// =====================================================================
// Sistema Inteligente de Gerenciamento de Filas Virtuais - Mapiii
// GS 2026.2 · Arquitetura e Organizacao de Computadores · 1CC-FEV
//
// ESP32 (simulado no Wokwi) com:
//   ENTRADA : botao CHEGADA (GPIO 18), botao ATENDIDO (GPIO 19),
//             comandos do painel web (MQTT) e do Monitor Serial
//   PROCESSO: logica da fila + previsao de espera (firmware/fila.h)
//   SAIDA   : display OLED SSD1306 (I2C, GPIO 21/22), LEDs verde (GPIO 25)
//             e vermelho (GPIO 26), Monitor Serial
//   REDE    : Wi-Fi -> MQTT (broker publico) -> painel no navegador
//             NTP para a hora real dos eventos
// =====================================================================
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include "fila.h"

void relatorioHardware();   // definida mais abaixo

// ---- Pinos -----------------------------------------------------------
const uint8_t PINO_CHEGADA  = 18;
const uint8_t PINO_ATENDIDO = 19;
const uint8_t PINO_LED_OK   = 25;   // verde: operacao aceita
const uint8_t PINO_LED_ERRO = 26;   // vermelho: operacao recusada

// ---- Rede ------------------------------------------------------------
const char *WIFI_SSID  = "Wokwi-GUEST";   // rede do simulador (sem senha)
const char *WIFI_SENHA = "";
const char *MQTT_HOST  = "broker.hivemq.com";
const uint16_t MQTT_PORTA = 1883;
// Troque o final se outra equipe usar o mesmo nome (o painel usa o mesmo).
const char *POSTO = "mapiii-posto-pcd-570589";
const char *TOPICO_BASE = "fiap-gs2026/mapiii-fila/";

// ---- Objetos ---------------------------------------------------------
Adafruit_SSD1306 tela(128, 64, &Wire, -1);
WiFiClient wifiCliente;
PubSubClient mqtt(wifiCliente);
Fila fila;                         // ~2,1 KB em RAM estatica (.bss)

char topicoEstado[96], topicoEvento[96], topicoComando[96];
char json[640];

// ---- Estado de E/S ---------------------------------------------------
struct Botao {
    uint8_t  pino;
    bool     estavel;      // nivel ja confirmado (HIGH = solto, por causa do pull-up)
    bool     lido;         // ultima leitura crua
    uint32_t mudou_ms;     // quando a leitura crua mudou
};
Botao botaoChegada  = { PINO_CHEGADA,  HIGH, HIGH, 0 };
Botao botaoAtendido = { PINO_ATENDIDO, HIGH, HIGH, 0 };
const uint32_t DEBOUNCE_MS = 40;

uint32_t ledDesligaEm = 0;
uint32_t mensagemAte = 0;
char mensagem[22] = "";
uint32_t ultimaTentativaRede = 0;
uint32_t ultimoEstadoPublicado = 0;
bool estadoMudou = true;
bool telaSuja = true;              // so redesenha o OLED quando algo muda

// ---------------------------------------------------------------------
uint32_t instanteAgora() {
    time_t t = time(nullptr);
    // Antes do NTP responder, time() devolve segundos desde 1970 ~ 0:
    // usa os segundos desde o boot (o painel mostra como "sem hora real").
    return (t > 1700000000) ? (uint32_t)t : millis() / 1000;
}

void piscar(uint8_t pino) {
    digitalWrite(PINO_LED_OK, pino == PINO_LED_OK);
    digitalWrite(PINO_LED_ERRO, pino == PINO_LED_ERRO);
    ledDesligaEm = millis() + 300;
}

void avisar(const char *texto) {
    strncpy(mensagem, texto, sizeof mensagem - 1);
    mensagem[sizeof mensagem - 1] = '\0';
    mensagemAte = millis() + 1500;
    telaSuja = true;
}

void publicarEvento(const char *origem) {
    const Evento *e = fila_ultimo_evento(&fila);
    if (!e) return;
    fila_json_evento(e, origem, json, sizeof json);
    Serial.printf("[evento] %s\n", json);
    if (mqtt.connected()) mqtt.publish(topicoEvento, json);
    estadoMudou = true;
    telaSuja = true;
}

void publicarEstado() {
    char extra[200];
    snprintf(extra, sizeof extra,
             ",\"heap_livre\":%lu,\"heap_min\":%lu,\"rssi\":%d,\"uptime_s\":%lu,\"ntp\":%s",
             (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(),
             WiFi.isConnected() ? WiFi.RSSI() : 0, (unsigned long)(millis() / 1000),
             time(nullptr) > 1700000000 ? "true" : "false");
    fila_json_estado(&fila, POSTO, extra, json, sizeof json);
    if (mqtt.connected()) {
        mqtt.publish(topicoEstado, json, true);   // retido: quem abre o painel ja recebe
    }
    ultimoEstadoPublicado = millis();
    estadoMudou = false;
}

// ---- As duas operacoes da fila (botao, painel ou serial) -------------
void registrarChegada(const char *origem) {
    if (fila_chegada(&fila, millis(), instanteAgora())) {
        piscar(PINO_LED_OK);
        avisar("Chegada registrada");
    } else {
        piscar(PINO_LED_ERRO);
        avisar("Fila cheia!");
    }
    publicarEvento(origem);
}

void registrarAtendido(const char *origem) {
    if (fila_atendido(&fila, millis(), instanteAgora())) {
        piscar(PINO_LED_OK);
        avisar("Atendimento concluido");
    } else {
        piscar(PINO_LED_ERRO);
        avisar("Fila vazia!");     // bloqueado: a contagem nao fica negativa
    }
    publicarEvento(origem);
}

// ---- Entrada: botoes com debounce por varredura (polling) -------------
// Retorna true no instante em que o botao e APERTADO (borda HIGH -> LOW).
bool apertou(Botao &b) {
    bool leitura = digitalRead(b.pino);
    uint32_t agora = millis();
    if (leitura != b.lido) {           // mudou: reinicia a contagem do debounce
        b.lido = leitura;
        b.mudou_ms = agora;
    }
    if (agora - b.mudou_ms >= DEBOUNCE_MS && leitura != b.estavel) {
        b.estavel = leitura;           // ficou estavel por 40 ms: aceita
        return b.estavel == LOW;
    }
    return false;
}

// ---- Entrada: comandos do painel (MQTT) ------------------------------
void aoReceberMqtt(char *topico, byte *dados, unsigned int tam) {
    (void)topico;                      // so assinamos um topico: o de comando
    char cmd[16];
    unsigned int n = tam < sizeof cmd - 1 ? tam : sizeof cmd - 1;
    memcpy(cmd, dados, n);
    cmd[n] = '\0';
    if (strcmp(cmd, "chegada") == 0)       registrarChegada("painel");
    else if (strcmp(cmd, "atendido") == 0) registrarAtendido("painel");
    else Serial.printf("[mqtt] comando ignorado: %s\n", cmd);
}

// ---- Entrada: Monitor Serial -----------------------------------------
void lerSerial() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == 'c') registrarChegada("serial");
        else if (c == 'a') registrarAtendido("serial");
        else if (c == 'l') {
            Serial.printf("--- registro de eventos (%u de %u) ---\n", fila.log_qtd, CAPACIDADE_LOG);
            for (uint16_t i = 0; i < fila.log_qtd; i++) {
                const Evento *e = fila_evento(&fila, i);
                time_t t = e->instante;
                struct tm hora;
                localtime_r(&t, &hora);
                Serial.printf("#%u %02d:%02d:%02d %-16s fila=%u\n", e->seq, hora.tm_hour,
                              hora.tm_min, hora.tm_sec, fila_nome_evento(e->tipo), e->fila);
            }
        } else if (c == 'm') {
            relatorioHardware();
        }
    }
}

// ---- Saida: display OLED ---------------------------------------------
void desenharTela() {
    tela.clearDisplay();
    tela.setTextColor(SSD1306_WHITE);
    tela.setTextSize(1);
    tela.setCursor(0, 0);
    tela.print("FILA VIRTUAL  ");
    tela.print(WiFi.isConnected() ? (mqtt.connected() ? "online" : "s/mqtt") : "offline");

    tela.setTextSize(3);
    tela.setCursor(0, 12);
    tela.print(fila.aguardando);
    tela.setTextSize(1);
    tela.setCursor(52, 14);
    tela.print("na fila");
    tela.setCursor(52, 24);
    tela.printf("%lu atendidos", (unsigned long)fila.concluidos);

    tela.setCursor(0, 40);
    tela.printf("Espera IA : %4.0f min", fila_espera_ia(&fila));
    tela.setCursor(0, 49);
    tela.printf("Pela media: %4.0f min", fila_espera_convencional(&fila));

    if (millis() < mensagemAte) {         // faixa de aviso na ultima linha
        tela.fillRect(0, 57, 128, 7, SSD1306_WHITE);
        tela.setTextColor(SSD1306_BLACK);
        tela.setCursor(2, 57);
        tela.print(mensagem);
    }
    tela.display();
}

// ---- Rede: (re)conexao sem travar os botoes ----------------------------
void cuidarDaRede() {
    if (millis() - ultimaTentativaRede < 5000) return;
    ultimaTentativaRede = millis();
    if (!WiFi.isConnected()) {
        Serial.println("[rede] Wi-Fi caiu, reconectando...");
        WiFi.reconnect();
        return;
    }
    if (!mqtt.connected()) {
        char id[40];
        snprintf(id, sizeof id, "esp32-%s-%04lx", POSTO, (unsigned long)(esp_random() & 0xFFFF));
        Serial.printf("[mqtt] conectando a %s ... ", MQTT_HOST);
        if (mqtt.connect(id)) {
            Serial.println("ok");
            mqtt.subscribe(topicoComando);
            estadoMudou = true;
        } else {
            Serial.printf("falhou (estado %d), nova tentativa em 5 s\n", mqtt.state());
        }
    }
}

// ---- Relatorio de hardware (Etapa 5: CPU e memoria medidas na placa) --
void relatorioHardware() {
    Serial.println("========== HARDWARE ==========");
    Serial.printf("Chip: %s rev %d, %d nucleo(s), %lu MHz\n", ESP.getChipModel(),
                  ESP.getChipRevision(), ESP.getChipCores(), (unsigned long)ESP.getCpuFreqMHz());
    Serial.printf("Flash: %lu KB | programa: %lu KB | livre p/ programa: %lu KB\n",
                  (unsigned long)(ESP.getFlashChipSize() / 1024), (unsigned long)(ESP.getSketchSize() / 1024),
                  (unsigned long)(ESP.getFreeSketchSpace() / 1024));
    Serial.printf("Heap: total %lu B | livre %lu B | menor livre %lu B | maior bloco %lu B\n",
                  (unsigned long)ESP.getHeapSize(), (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)ESP.getMinFreeHeap(), (unsigned long)ESP.getMaxAllocHeap());
    Serial.printf("Pilha da loop(): sobra minima %lu B\n",
                  (unsigned long)uxTaskGetStackHighWaterMark(NULL));
    Serial.printf("Estruturas: Fila = %u B, Evento = %u B, registro = %u eventos (%u B)\n",
                  (unsigned)sizeof(Fila), (unsigned)sizeof(Evento), CAPACIDADE_LOG,
                  (unsigned)(CAPACIDADE_LOG * sizeof(Evento)));
    Serial.printf("Buffers: JSON %u B, MQTT %u B, tela 128x64/8 = 1024 B\n",
                  (unsigned)sizeof json, (unsigned)mqtt.getBufferSize());
    Serial.println("==============================");
}

// =====================================================================
void setup() {
    Serial.begin(115200);
    pinMode(PINO_CHEGADA, INPUT_PULLUP);    // solto = HIGH, apertado = LOW (liga no GND)
    pinMode(PINO_ATENDIDO, INPUT_PULLUP);
    pinMode(PINO_LED_OK, OUTPUT);
    pinMode(PINO_LED_ERRO, OUTPUT);

    Wire.begin(21, 22);                     // I2C: SDA 21, SCL 22
    if (!tela.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("[tela] SSD1306 nao encontrado");
    }
    tela.clearDisplay();
    tela.setTextColor(SSD1306_WHITE);
    tela.setCursor(0, 0);
    tela.println("Mapiii - fila virtual");
    tela.println("Conectando ao Wi-Fi...");
    tela.display();

    fila_iniciar(&fila);
    snprintf(topicoEstado,  sizeof topicoEstado,  "%s%s/estado",  TOPICO_BASE, POSTO);
    snprintf(topicoEvento,  sizeof topicoEvento,  "%s%s/evento",  TOPICO_BASE, POSTO);
    snprintf(topicoComando, sizeof topicoComando, "%s%s/comando", TOPICO_BASE, POSTO);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_SENHA, 6);   // canal 6 deixa a conexao no Wokwi mais rapida
    uint32_t inicio = millis();
    while (!WiFi.isConnected() && millis() - inicio < 15000) delay(100);
    Serial.printf("[rede] Wi-Fi %s, IP %s\n", WiFi.isConnected() ? "conectado" : "SEM CONEXAO",
                  WiFi.localIP().toString().c_str());

    configTime(-3 * 3600, 0, "pool.ntp.org", "time.google.com");   // horario de Brasilia
    mqtt.setServer(MQTT_HOST, MQTT_PORTA);
    mqtt.setCallback(aoReceberMqtt);
    mqtt.setBufferSize(sizeof json + 128);
    mqtt.setKeepAlive(60);                  // tolera simulacao lenta / rede instavel
    ultimaTentativaRede = millis() - 5000;   // tenta o MQTT ja no primeiro loop

    relatorioHardware();
    Serial.printf("Topicos: %s | %s | %s\n", topicoEstado, topicoEvento, topicoComando);
    Serial.println("Serial: c = chegada, a = atendido, l = registro, m = memoria");
}

void loop() {
    // ENTRADA
    if (apertou(botaoChegada))  registrarChegada("botao");
    if (apertou(botaoAtendido)) registrarAtendido("botao");
    lerSerial();

    // REDE
    cuidarDaRede();
    mqtt.loop();                       // recebe comandos do painel

    // SAIDA
    if (estadoMudou || millis() - ultimoEstadoPublicado > 5000) publicarEstado();
    if (ledDesligaEm && millis() > ledDesligaEm) {
        digitalWrite(PINO_LED_OK, LOW);
        digitalWrite(PINO_LED_ERRO, LOW);
        ledDesligaEm = 0;
    }
    // O OLED so e redesenhado quando algo muda (evento, aviso que expira ou
    // mudanca de conexao): cada quadro sao 1.024 B pelo I2C (~25 ms de CPU).
    static bool conexaoAntes = false, avisoAntes = false;
    bool conexaoAgora = WiFi.isConnected() && mqtt.connected();
    bool avisoAgora = millis() < mensagemAte;
    if (conexaoAgora != conexaoAntes || avisoAgora != avisoAntes) telaSuja = true;
    conexaoAntes = conexaoAgora;
    avisoAntes = avisoAgora;
    if (telaSuja) {
        desenharTela();
        telaSuja = false;
    }
    delay(5);                          // deixa a CPU respirar (tarefas de Wi-Fi)
}
