# Falas do vídeo de COA

Leia cada fala no tempo indicado (o mesmo texto aparece na legenda do vídeo).

| Início | Quem | Fala |
|---|---|---|
| 0:00 | Gabriel | Olá! Eu sou o Gabriel e, junto com o Thor Ferreira Camargo, desenvolvi este Sistema Inteligente de Gerenciamento de Filas Virtuais, parte do Citizen Assist AI. |
| 0:10 | Gabriel | Ele mostra a fila de um posto de atendimento à pessoa com deficiência e estima quanto tempo falta, para a família decidir a hora de sair de casa. |
| 0:20 | Gabriel | Na arquitetura, o processador é um ESP32: núcleo Xtensa de 32 bits, dois núcleos a 240 megahertz, 520 kilobytes de RAM e 4 megabytes de flash. |
| 0:34 | Gabriel | As entradas são dois botões, nos pinos 18 e 19, com debounce. As saídas são o display OLED, pelo barramento I2C, e dois LEDs. |
| 0:45 | Gabriel | A placa se conecta ao Wi-Fi e publica os dados por MQTT. O painel no navegador recebe tudo em tempo real, e o NTP dá a hora certa de cada evento. |
| 0:58 | Gabriel | Agora o protótipo funcionando: à esquerda, o ESP32 no Wokwi; à direita, o painel publicado na internet. Os dois já estão conectados. |
| 1:08 | Gabriel | Teste um: chegada de cidadãos. Cada aperto no botão verde soma uma pessoa, pisca o LED verde e aparece no painel na hora, com o horário. |
| 1:18 | Gabriel | Teste dois: conclusão de atendimentos. O botão azul tira uma pessoa da fila, e a placa mede quanto durou cada atendimento. |
| 1:32 | Gabriel | Teste três: com a fila vazia, o atendimento é bloqueado. A contagem não fica negativa, acende o LED vermelho e o erro fica registrado. |
| 1:45 | Gabriel | Teste quatro: a estimativa de espera. A cada chegada, a placa recalcula o tempo pela média histórica e pela previsão da IA. |
| 2:00 | Gabriel | Agora dois atendimentos mais lentos. A média dos últimos atendimentos sobe, e a previsão da IA aumenta para o mesmo tamanho de fila. |
| 2:19 | Gabriel | O painel também tem botões virtuais: o comando vai pela internet até a placa, que valida e responde. No registro, a origem aparece como painel. |
| 2:32 | Gabriel | A previsão compara dois métodos: a média histórica e uma regressão linear, treinada com um histórico simulado de trinta dias, identificado como simulado. |
| 2:42 | Gabriel | Nos dias de teste, que o modelo não viu, a regressão errou em média um vírgula noventa e nove minuto, contra dois vírgula quarenta e oito da média: cerca de vinte por cento menos erro. |
| 2:54 | Gabriel | Na análise da arquitetura, medimos na própria placa: cerca de 320 mil bytes de heap, e o nosso programa usa só uns 5 mil, perto de um por cento da RAM. O programa ocupa 934 kilobytes da flash. |
| 3:11 | Gabriel | Também vimos na prática que redesenhar o display pelo I2C custa caro para a CPU, então ele só é redesenhado quando algo muda. |
| 3:21 | Gabriel | As limitações: o registro fica na RAM e some se a placa reiniciar, o broker público não tem senha e o MQTT não garante cada mensagem. Se o Wi-Fi cair, a fila continua funcionando no posto. |
| 3:36 | Gabriel | Tudo está no repositório GS2-COA, com o relatório técnico, os testes e o link do Wokwi. Obrigado! |

**Duração total:** 3:47

