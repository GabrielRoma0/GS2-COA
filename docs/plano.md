# GS 2026.2 · Computer Organization and Architecture (COA) · Plano de trabalho

Contribuição da disciplina ao Mapiii (Citizen Assist AI). Escrito em 17/11/2026 a partir do enunciado `GS2026_2_Arquitetura_Organizacao_Computadores.pdf` (turma 1CC-FEV, vale 100 pontos): **Sistema Inteligente de Gerenciamento de Filas Virtuais**.

> **Regra desta pasta:** nada daqui vai para o repositório oficial (`mapiii-assistente` / `GS-2SEM-2026`) e nada é mexido lá. O Mapiii serve só de **base** (público, tom, regra de dado identificado). A entrega de COA é uma pasta/repositório **separado**.

---

> **Status 17/11/2026:** firmware (ESP32/Wokwi), lógica da fila com 39 testes, IA (regressão: erro médio 1,99 min × 2,48 min da média), painel web testado de ponta a ponta pelo broker público, simulador Python (plano B, coerente com o firmware), diagrama, relatório técnico, testes e roteiro do vídeo prontos. **Verificado no Wokwi (17/11):** compila, conecta, publica e responde a botões, serial e comandos remotos. **Falta da dupla:** montar o projeto no Wokwi e salvar o link, rodar os 4 testes com prints, preencher os [PREENCHER] (valores da tecla `m`, links, conclusão), publicar o painel no GitHub Pages, gravar o vídeo, entregar.

## 0. O que o professor vai cobrar (resumo do enunciado)

| Pontos | Critério | O que precisa aparecer |
|---|---|---|
| **20** | Arquitetura e diagrama | CPU/microcontrolador, RAM e armazenamento, entradas (botões), saídas (display/dashboard), interface de rede, e o **fluxo** entre eles explicado. |
| **25** | Fila virtual e E/S | Botão "chegou", botão "atendido", contagem automática, **nunca negativa**, total exibido, **log de eventos com horário**. |
| **15** | Previsão + IA | Estimativa convencional (média histórica) **comparada** com um modelo simples (regressão linear). Dados simulados podem, **identificados**. |
| **15** | Rede e visualização | Página no navegador com: na fila agora, tempo estimado, atendimentos concluídos, atualizando sozinha. |
| **15** | Análise de CPU, memória, interfaces | Arquitetura do processador, organização e **estimativa de uso** de memória, E/P/S, protocolos, limitações. |
| **10** | Documentação, testes, apresentação | Repositório organizado, relatório técnico, vídeo ≤ 5 min. |

Testes mínimos a demonstrar: (1) chegada; (2) conclusão de atendimento; (3) saída com fila vazia (bloqueada); (4) estimativa sendo atualizada.

Entregáveis: protótipo · diagrama · código com instruções · relatório técnico · vídeo ≤ 5 min.


## Cronograma (entrega 18/11)

| Quando | O quê |
|---|---|
| 17/11 noite | Passos 2 a 5: diagrama, firmware no Wokwi, IA e painel (Claude); dupla abre o link do Wokwi e testa |
| 18/11 manhã | Passos 6 e 7: análise da arquitetura e os 4 testes com prints |
| 18/11 tarde | Passos 8 e 9: relatório, vídeo ≤ 5 min, push no GitHub, entrega no Portal |

---

## 1. A ideia (recomendação)

**Cenário (ligado ao Mapiii):** fila de um **posto de atendimento à pessoa com deficiência** (ex.: atendimento presencial de cadastro do Bilhete Único Especial ou de um balcão da Prefeitura). A pessoa ou a família vê pelo celular quanto tempo falta antes de sair de casa, o que importa muito para quem depende de transporte adaptado.

**Arquitetura recomendada: ESP32 simulado no Wokwi + painel web.**

```
[Botão CHEGADA] ─┐                         ┌─> [Display OLED SSD1306 (I2C)]
[Botão ATENDIDO]─┼─> GPIO ─> ESP32 ───────┤
                 │  (interrupção/debounce) │   RAM: fila, contadores, buffer de eventos
                 │                         │   Flash: firmware + histórico (NVS/LittleFS)
                 │                         └─> Wi-Fi ─> MQTT (broker público, WebSocket)
                 │                                          │
                 │                                          v
                 │                           [Painel web no navegador]
                 │                           fila · espera estimada · concluídos · log
```

- **Por que ESP32:** é o exemplo citado no enunciado, tem ficha técnica concreta para a Etapa 5 (Xtensa LX6 dual-core até 240 MHz, ~520 KB de SRAM, flash externa de 4 MB, Wi-Fi 802.11 b/g/n, GPIO, I2C) e o Wokwi simula tudo de graça, sem comprar placa.
- **Por que MQTT:** o Wokwi deixa o ESP32 sair para a internet; o painel web assina o mesmo tópico via WebSocket, então "atualização pela comunicação com o sistema" fica real e fácil de mostrar no vídeo.
- **Previsão (Etapa 3):**
  - convencional: `espera = pessoas_na_fila × média_histórica_de_atendimento`;
  - IA: **regressão linear** treinada sobre um histórico **simulado e identificado** (`dados/historico_simulado.csv`, ex.: tempo de espera real × pessoas na fila, horário). Treino em Python (mínimos quadrados escrito à mão ou `numpy`), coeficientes `a` e `b` gravados no firmware/painel; comparação com **MAE** das duas abordagens num conjunto de teste.
- **Painel:** HTML + JS puro (sem framework), acessível (contraste, texto grande, `aria-live` no número da fila), no estilo do Mapiii.

**Plano B (se o Wokwi/MQTT der problema):** aplicação Python que **representa** a mesma arquitetura (o enunciado aceita "uma aplicação que represente a arquitetura proposta"): servidor HTTP local com botões virtuais e painel, mesmo log e mesma previsão. O trabalho COA da GS anterior da dupla foi em Python (`~/GS-COA/GS-COA-1SEM.py`).

---

## 2. Como organizar a pasta

```
COA - GS2 - 2026/
├── GS2026_2_Arquitetura_Organizacao_Computadores.pdf   enunciado
├── README.md                     integrantes, cenário, como rodar (link do Wokwi), vídeo
├── firmware/
│   ├── sketch.ino                ESP32: botões, fila, OLED, log, MQTT, previsão
│   ├── diagram.json              circuito do Wokwi
│   └── wokwi.toml / libraries.txt
├── painel/index.html             dashboard web (MQTT via WebSocket)
├── ia/
│   ├── gerar_historico.py        gera o histórico SIMULADO (semente fixa)
│   ├── treinar_regressao.py      regressão linear + comparação com a média (MAE)
│   └── resultados.md             coeficientes e erro das duas abordagens
├── dados/historico_simulado.csv  marcado como simulado
├── docs/
│   ├── plano.md                  este arquivo
│   ├── arquitetura.png/.svg      diagrama (entregável)
│   ├── relatorio-tecnico.md      → exportar para PDF/DOCX (entregável)
│   └── testes.md                 4 testes mínimos com prints
└── video/roteiro.md              roteiro de até 5 min
```

---

## 3. Passo a passo (em ordem)

Legenda: **C** = Claude escreve, **D** = dupla faz/decide.

### Passo 1 · Decidir (D, 10 min)
Hardware real ou Wokwi, cenário, prazo (seção 7).

### Passo 2 · Diagrama de arquitetura (C desenha, D revisa)
SVG com os 5 blocos exigidos + setas do fluxo (entrada → CPU → RAM/flash → saída/rede) e texto curto explicando o papel de cada um.

### Passo 3 · Fila no ESP32 (C)
Dois botões com **debounce**, contador protegido (`if (fila > 0)`; saída com fila vazia gera evento `ERRO_FILA_VAZIA` e mensagem no OLED), contador de concluídos, **buffer circular** de eventos `{tipo, timestamp}` (horário via NTP), OLED com fila e espera.

### Passo 4 · Previsão (C escreve, D entende para explicar)
Gerar histórico simulado → treinar regressão → comparar com média (MAE, tabela) → embutir `a`, `b` no firmware. A estimativa recalcula a cada evento (teste 4).

### Passo 5 · Rede e painel (C)
ESP32 publica JSON `{fila, concluidos, espera_media, espera_ia, ultimo_evento}` em um tópico MQTT; painel assina e atualiza sozinho; mostra também o log.

### Passo 6 · Análise da arquitetura (C escreve, D revisa)
CPU (ISA, núcleos, clock), mapa de memória e **estimativa de uso** (tamanho da struct de evento × capacidade do buffer, pilha, heap do Wi-Fi/MQTT), ciclo E/P/S, protocolos (GPIO, I2C, Wi-Fi, TCP/IP, MQTT, NTP), limitações (RAM finita do buffer, perda de Wi-Fi, latência do broker público, debounce).

### Passo 7 · Testes (D roda, C prepara o roteiro)
Os 4 testes mínimos com print/gravação + casos extras (cliques rápidos, Wi-Fi caído).

### Passo 8 · Relatório técnico e vídeo (C rascunha, D grava)
Relatório: arquitetura, componentes, funcionamento, previsão, testes, resultados. Vídeo ≤ 5 min: problema → diagrama → demo dos 4 testes → IA × média → limitações.

### Passo 9 · Entrega (D)
Conferir no Portal formato e prazo; integrantes e RM no README.

---

## 4. Checklist final

- [ ] Diagrama com CPU, RAM/armazenamento, entrada, saída e rede, e fluxo explicado
- [ ] Fila nunca fica negativa; tentativa inválida aparece no log
- [ ] Log com horário de cada evento
- [ ] Histórico marcado como **simulado** em todo lugar que aparece
- [ ] Comparação média × regressão com número (MAE) e conclusão
- [ ] Painel no navegador atualizando sozinho
- [ ] Estimativa de memória com conta, não só "o ESP32 tem 520 KB"
- [ ] Os 4 testes mínimos documentados
- [ ] Vídeo ≤ 5 min
- [ ] Coerência: o que o diagrama diz = o que o código faz = o que o relatório conta

---

## 5. Riscos conhecidos

- Broker MQTT público pode cair ou ficar lento: ter o plano B (Python local) pronto ou gravar o vídeo cedo.
- Wokwi gratuito tem fila de compilação; não deixar a gravação para a última hora.
- Botão do Wokwi também "quica": sem debounce, um clique conta duas pessoas.

---

## 6. Divisão sugerida

| Quem | O quê |
|---|---|
| Claude | firmware, painel, scripts de IA, diagrama, rascunho do relatório |
| Dupla | decidir, rodar no Wokwi, conferir os números, entender para explicar, prints, vídeo, entrega |

---

## 7. Decisões da dupla (preencher)

| Pergunta | Recomendação | Decisão |
|---|---|---|
| Plataforma | ESP32 no Wokwi + MQTT + painel web | **ESP32 no Wokwi** (17/11) |
| Cenário da fila | Posto de atendimento à PcD | |
| Modelo de IA | Regressão linear sobre histórico simulado | |
| Prazo e formato de entrega | conferir no Portal | **18/11** (formato: conferir no Portal) |
| Repositório no GitHub | público | **Sim, público** (dupla cria o repo e dá push) |
