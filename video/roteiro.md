# Roteiro do vídeo (até 5 min)

Gravar a tela com o Wokwi à esquerda e o painel à direita (navegador lado a lado). Monitor Serial visível embaixo. Ensaiar uma vez antes. Começar a gravação **com a simulação já conectada** (`[mqtt] ... ok`), para não perder tempo esperando o Wi-Fi.

| Tempo | Quem | Tela | Fala (resumo) |
|---|---|---|---|
| 0:00–0:30 | Gabriel | diagrama `docs/arquitetura.png` | Problema: no posto de atendimento à pessoa com deficiência, ninguém sabe quanto vai esperar, e para quem depende de transporte adaptado isso pesa. Nossa solução: fila virtual com previsão de espera, parte do Citizen Assist AI / Mapiii. |
| 0:30–1:20 | Thor | diagrama | Arquitetura: ESP32 (Xtensa LX6, 2 núcleos, 240 MHz, 520 KB de SRAM, 4 MB de flash); entradas (2 botões em GPIO com pull-up e debounce); saídas (OLED por I2C, LEDs); Wi-Fi → MQTT → painel; NTP para a hora. Fluxo de uma chegada (legenda do diagrama). |
| 1:20–2:20 | Gabriel | Wokwi + painel | **Teste 1:** 3 chegadas (fila sobe no OLED e no painel, LED verde, eventos com horário). **Teste 2:** 2 atendimentos (concluídos sobem, média medida). **Teste 3:** atendido com fila vazia → bloqueado, LED vermelho, "Fila vazia!", evento de erro no registro. |
| 2:20–3:20 | Thor | painel + `ia/resultados.md` | **Teste 4 e IA:** chegadas fazem a estimativa subir; atendimentos lentos fazem a IA subir mais que a média. Explicar: média histórica × regressão linear treinada com dados **simulados** (dizer isso em voz alta); no teste, erro médio 1,99 min × 2,48 min; por quê (desconto de quem já está sendo atendido + ritmo recente). |
| 3:20–3:50 | Gabriel | painel | Botão virtual (comando vai pela rede, a placa valida), gráfico, baixar CSV. Pausar a simulação e mostrar "Posto sem enviar dados". |
| 3:50–4:40 | Thor | Monitor Serial (tecla `m`) + relatório seção 6 | Análise: memória medida (heap livre, programa na flash), nosso programa usa ~5 KB (~1% da SRAM); ciclo entrada → processamento → saída; o mais caro é o I2C da tela; protocolos (GPIO, I2C, UART, Wi-Fi, TCP/IP, MQTT, WebSocket/TLS, NTP). |
| 4:40–5:00 | Gabriel | diagrama | Limitações (RAM volátil, broker público sem senha, QoS 0, dados simulados) e o que melhoraria. Encerrar. |

Checklist antes de gravar:

- [ ] Projeto do Wokwi salvo e com link público
- [ ] Painel aberto com o mesmo nome de posto do firmware
- [ ] Fila zerada (reiniciar a simulação)
- [ ] Monitor Serial limpo
- [ ] Vídeo com no máximo 5:00
