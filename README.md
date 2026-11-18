# Fila virtual inteligente · COA (GS 2026.2)

**Sistema Inteligente de Gerenciamento de Filas Virtuais** para um posto de atendimento à pessoa com deficiência. Disciplina **Computer Organization and Architecture**, turma 1CCR, projeto integrador Citizen Assist AI (nosso assistente: **Mapiii**).

| Nome | RM |
|---|---|
| Gabriel Botelho Romão | 570589 |
| Thor Ferreira Camargo | 569543 |

- **Projeto no Wokwi:** https://wokwi.com/projects/480979792844066817
- **Painel online:** https://gabrielroma0.github.io/GS2-COA/painel/ (ou abra `painel/index.html` no navegador)
- **Vídeo (até 5 min):** https://youtu.be/AqaBxOGu3d0
- **Relatório técnico:** [docs/relatorio-tecnico.md](docs/relatorio-tecnico.md) · **Diagrama:** [docs/arquitetura.png](docs/arquitetura.png)

> Protótipo de estudo. A previsão foi treinada com **dados simulados** (`dados/historico_simulado.csv`), identificados como tal em todo o projeto.

## O que faz

- Botão **CHEGADA** e botão **ATENDIDO** no ESP32; também há botões virtuais no painel e teclas no Monitor Serial.
- A contagem **nunca fica negativa**: "atendido" com a fila vazia é bloqueado, acende o LED vermelho e fica registrado como erro.
- Registro de eventos com horário (NTP) num buffer circular na RAM, enviado ao painel, que exporta em CSV.
- **Duas previsões de espera** lado a lado: pela média histórica e por **regressão linear** (no teste com dados simulados: 1,99 min de erro médio × 2,48 min).
- Display OLED e LEDs no posto; **painel no navegador** atualizado por MQTT.

## Como rodar no Wokwi (a placa)

1. Abra o projeto pronto: https://wokwi.com/projects/480979792844066817 (já tem os arquivos de [`firmware/`](firmware/) e as bibliotecas).
2. Clique em ▶ (Start). O Monitor Serial mostra o relatório de hardware e `[mqtt] conectando a broker.hivemq.com ... ok`.
3. Abra o painel (abaixo) e aperte os botões verde (chegada) e azul (atendido) no Wokwi.

Monitor Serial: `c` = chegada, `a` = atendido, `l` = lista o registro de eventos, `m` = relatório de CPU e memória.

## Como abrir o painel

Basta abrir [`painel/index.html`](painel/index.html) no navegador (precisa de internet para falar com o broker). Para servir localmente:

```bash
make painel
```

e acesse http://localhost:8765. Outro posto: `index.html?posto=nome-do-posto`, com o mesmo nome em `POSTO` no `sketch.ino`.

Publicado no GitHub Pages: https://gabrielroma0.github.io/GS2-COA/painel/

## Plano B: posto simulado em Python

Se o Wokwi estiver fora do ar, o simulador faz o papel da placa: mesmas regras, mesmos coeficientes, mesmo tópico. Os números são comprovadamente iguais (`make coerencia`).

```bash
make simulador
```

## Testes e IA

```bash
make test
```

```bash
make coerencia
```

```bash
make treinar
```

- `make test`: 39 testes da lógica da fila (o mesmo `fila.h` da placa), com AddressSanitizer.
- `make coerencia`: firmware × simulador.
- `make treinar`: regenera o histórico simulado e o modelo (atualiza `firmware/modelo.h`).

Evidências em [docs/testes.md](docs/testes.md) e prints em [docs/prints/](docs/prints/).

## Estrutura

```
firmware/   sketch.ino, fila.h (lógica), modelo.h (coeficientes gerados), diagram.json, libraries.txt
painel/     index.html (dashboard, MQTT por WebSocket)
ia/         gerar_historico.py, treinar_regressao.py, modelo.json, resultados.md
dados/      historico_simulado.csv (SIMULADO)
simulador/  posto_simulado.py (plano B)
testes/     teste_fila.cpp, sequencia.cpp, coerencia.py
docs/       arquitetura.svg/.png, relatorio-tecnico.md, testes.md, plano.md
video/      roteiro.md
```
