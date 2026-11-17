// Testes da logica da fila (firmware/fila.h) no PC, sem placa.
// Rodar:  make test   (ou: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Ifirmware testes/teste_fila.cpp -o testes/teste_fila && ./testes/teste_fila)
#include <cmath>
#include <cstring>
#include <cstdio>
#include "fila.h"

static int total = 0, falhas = 0;
#define CHECAR(cond, desc) do { total++; if (cond) printf("  ok   %s\n", desc); \
    else { falhas++; printf("  FALHOU %s (linha %d)\n", desc, __LINE__); } } while (0)

static bool perto(float a, float b, float tol = 0.01f) { return std::fabs(a - b) <= tol; }

// O enunciado pede 4 testes minimos; cada bloco abaixo corresponde a um deles.
static Fila f;   // global: a struct tem ~2,1 KB, como no ESP32

static void teste1_chegadas() {
    printf("\n[teste minimo 1: chegada de novos cidadaos]\n");
    fila_iniciar(&f);
    CHECAR(f.aguardando == 0 && f.log_qtd == 0, "comeca vazia");
    CHECAR(fila_chegada(&f, 0, 1000), "1a chegada registrada");
    CHECAR(fila_chegada(&f, 1000, 1001), "2a chegada registrada");
    CHECAR(fila_chegada(&f, 2000, 1002), "3a chegada registrada");
    CHECAR(f.aguardando == 3 && f.chegadas == 3, "fila = 3, chegadas = 3");
    const Evento *u = fila_ultimo_evento(&f);
    CHECAR(u && u->tipo == EV_CHEGADA && u->fila == 3 && u->instante == 1002 && u->seq == 3,
           "evento com tipo, fila depois, horario e numero de sequencia");
    CHECAR(f.inicio_atual_ms == 0, "atendimento da 1a pessoa comecou na chegada dela (fila estava vazia)");
}

static void teste2_atendimentos() {
    printf("\n[teste minimo 2: conclusao de atendimentos]\n");
    // continua do teste 1: 3 pessoas; o 1o atendimento comecou em 0 ms
    CHECAR(fila_atendido(&f, 6000, 1006), "conclui atendimento (6 s de botao)");
    CHECAR(f.aguardando == 2 && f.concluidos == 1, "fila = 2, concluidos = 1");
    CHECAR(perto(f.ult_dur[0], 6.0f * FATOR_TEMPO / 60.0f), "duracao medida = 6 s x FATOR_TEMPO");
    CHECAR(f.inicio_atual_ms == 6000, "proxima pessoa comecou a ser atendida na hora");
    CHECAR(fila_atendido(&f, 10000, 1010), "conclui o 2o (4 s)");
    CHECAR(f.n_dur == 2 && perto(fila_media_ult5(&f), 5.0f * FATOR_TEMPO / 60.0f), "media dos ultimos = 5 min no modo demo");
}

static void teste3_fila_vazia() {
    printf("\n[teste minimo 3: saida com fila vazia]\n");
    fila_atendido(&f, 15000, 1015);   // conclui o ultimo
    CHECAR(f.aguardando == 0 && f.concluidos == 3, "fila zerou com 3 concluidos");
    uint32_t recusas = f.recusas;
    CHECAR(!fila_atendido(&f, 16000, 1016), "\"atendido\" com fila vazia e RECUSADO");
    CHECAR(f.aguardando == 0, "a contagem nao fica negativa (nem da a volta para 255)");
    CHECAR(f.concluidos == 3 && f.recusas == recusas + 1, "concluidos nao muda; recusa contada");
    const Evento *u = fila_ultimo_evento(&f);
    CHECAR(u && u->tipo == EV_ERRO_FILA_VAZIA && u->fila == 0, "tentativa invalida fica no registro de eventos");
    CHECAR(f.n_dur == 3, "tentativa invalida nao entra na media de atendimento");
}

static void teste4_estimativa() {
    printf("\n[teste minimo 4: atualizacao da estimativa de espera]\n");
    fila_iniciar(&f);
    CHECAR(fila_espera_convencional(&f) == 0.0f && fila_espera_ia(&f) == 0.0f, "fila vazia: espera 0 nos dois metodos");
    CHECAR(perto(fila_media_historica(&f), MODELO_MEDIA_HISTORICA_MIN), "sem medicoes: media historica = a do treino");

    fila_chegada(&f, 0, 1);
    float conv1 = fila_espera_convencional(&f), ia1 = fila_espera_ia(&f);
    fila_chegada(&f, 0, 2);
    fila_chegada(&f, 0, 3);
    float conv3 = fila_espera_convencional(&f), ia3 = fila_espera_ia(&f);
    CHECAR(conv3 > conv1 && ia3 > ia1, "mais gente na fila -> estimativa sobe nos dois");
    CHECAR(perto(conv3, 3 * MODELO_MEDIA_HISTORICA_MIN), "convencional = fila x media historica");
    float esperado_ia = MODELO_B0 + MODELO_B1 * 3 + MODELO_B2 * 3 * MODELO_MEDIA_HISTORICA_MIN;
    CHECAR(perto(ia3, esperado_ia), "IA = b0 + b1*fila + b2*fila*media_ult5 (coeficientes do treino)");

    // atendimentos lentos (12 s = 12 min no modo demo) devem subir a previsao da IA
    fila_atendido(&f, 12000, 4);
    fila_chegada(&f, 12000, 5);
    float ia_lento = fila_espera_ia(&f);
    float ia_sem_ritmo = MODELO_B0 + MODELO_B1 * 3 + MODELO_B2 * 3 * MODELO_MEDIA_HISTORICA_MIN;
    CHECAR(ia_lento > ia_sem_ritmo, "atendimento mais lento que a media -> IA aumenta a espera");
    CHECAR(fila_media_historica(&f) > MODELO_MEDIA_HISTORICA_MIN, "media historica e atualizada com a medicao");
    CHECAR(fila_espera_ia(&f) >= 0.0f, "previsao nunca negativa");
}

static void teste_limites() {
    printf("\n[limites e registro]\n");
    fila_iniciar(&f);
    for (int i = 0; i < FILA_MAXIMA; i++) fila_chegada(&f, 0, (uint32_t)i);
    CHECAR(f.aguardando == FILA_MAXIMA, "enche ate o limite");
    CHECAR(!fila_chegada(&f, 0, 999) && f.aguardando == FILA_MAXIMA, "chegada alem do limite e recusada");
    CHECAR(fila_ultimo_evento(&f)->tipo == EV_ERRO_FILA_CHEIA, "recusa por fila cheia registrada");

    fila_iniciar(&f);
    for (int i = 0; i < CAPACIDADE_LOG + 10; i++) {
        if (i % 2 == 0) fila_chegada(&f, (uint32_t)i * 1000, (uint32_t)i);
        else fila_atendido(&f, (uint32_t)i * 1000, (uint32_t)i);
    }
    CHECAR(f.log_qtd == CAPACIDADE_LOG, "buffer circular nao passa da capacidade");
    CHECAR(fila_evento(&f, 0)->seq == 11 && fila_ultimo_evento(&f)->seq == CAPACIDADE_LOG + 10,
           "os 10 mais antigos foram sobrescritos, a ordem continua certa");
    CHECAR(fila_evento(&f, CAPACIDADE_LOG) == nullptr, "indice fora do registro devolve nulo");

    fila_iniciar(&f);
    fila_chegada(&f, 0xFFFFF000u, 1);          // perto do fim do contador millis()
    fila_atendido(&f, 0x00001000u, 2);         // contador deu a volta
    CHECAR(perto(f.ult_dur[0], (8192.0f / 60000.0f) * FATOR_TEMPO), "duracao certa mesmo quando millis() da a volta");
}

static void teste_json() {
    char buf[600];
    printf("\n[mensagens para o painel]\n");
    fila_iniciar(&f);
    fila_chegada(&f, 0, 1731850000u);
    int n = fila_json_estado(&f, "posto-teste", ",\"heap_livre\":1234", buf, sizeof buf);
    CHECAR(n > 0 && n < (int)sizeof buf, "estado cabe no buffer");
    CHECAR(strstr(buf, "\"fila\":1,") && strstr(buf, "\"tipo\":\"chegada\"") && strstr(buf, "\"instante\":1731850000"),
           "estado tem fila, ultimo evento e horario");
    CHECAR(strstr(buf, ",\"heap_livre\":1234}") && buf[n - 1] == '}', "campos extras entram e o JSON fecha");
    printf("       %s\n", buf);
    n = fila_json_evento(fila_ultimo_evento(&f), "botao", buf, sizeof buf);
    CHECAR(n > 0 && strstr(buf, "\"origem\":\"botao\""), "evento em JSON com a origem");
    CHECAR(sizeof(Evento) == 8, "cada evento ocupa 8 bytes");
    printf("       sizeof(Evento) = %zu B, sizeof(Fila) = %zu B\n", sizeof(Evento), sizeof(Fila));
}

int main() {
    printf("Testes da fila virtual (firmware/fila.h)\n");
    teste1_chegadas();
    teste2_atendimentos();
    teste3_fila_vazia();
    teste4_estimativa();
    teste_limites();
    teste_json();
    printf("\n%d testes, %d falha(s).\n", total, falhas);
    return falhas ? 1 : 0;
}
