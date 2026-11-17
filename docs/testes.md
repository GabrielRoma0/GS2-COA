# Testes e evidências

## 1. Os 4 testes mínimos do enunciado, na placa (Wokwi)

Executados em 17/11/2026 no projeto salvo (https://wokwi.com/projects/480979792844066817), com a simulação a ~60–99% da velocidade real e o painel publicado (https://gabrielroma0.github.io/GS2-COA/painel/) aberto ao lado. As operações foram feitas pelo Monitor Serial (`c`/`a`), que chamam as mesmas funções dos botões; os botões físicos e o virtual foram testados à parte (extras).

| # | Passo | Resultado esperado | Observado | Print |
|---|---|---|---|---|
| 1 | **Chegada de novos cidadãos:** 3 chegadas, ~2 s entre elas | fila 1 → 2 → 3; eventos com horário; estimativa aparece | eventos 1–3 `chegada` com hora real (NTP); OLED "3 na fila", IA 16 min × média 19 min | [03](prints/03-teste1-chegadas.jpg) |
| 2 | **Conclusão de atendimentos:** 2 atendimentos (~5 s cada) | fila 3 → 2 → 1; concluídos sobem; duração medida | eventos 4–5 `atendido`; OLED "1 na fila · 2 atendidos", IA 4 min × média 6 min | [04](prints/04-teste2-atendimentos.jpg) |
| 3 | **Saída com fila vazia:** atender até zerar e mais uma vez | a última é **recusada**: fila fica 0, LED vermelho, aviso, evento de erro | evento 6 zera a fila; evento 7 **`erro_fila_vazia`** com fila 0; OLED "Fila vazia!"; painel: "Bloqueado: fila vazia" em vermelho e 1 operação bloqueada | [05](prints/05-teste3-fila-vazia-bloqueada.jpg), [08](prints/08-painel-registro-e-botao-virtual.jpg) |
| 4 | **Atualização da estimativa:** 4 chegadas; 2 atendimentos **lentos** (~15 s e ~11 s); nova chegada | estimativas sobem com a fila; depois dos atendimentos lentos, para o **mesmo** tamanho de fila a previsão da IA é maior | com 3 na fila: IA **16 min** antes × **19 min** depois dos atendimentos lentos (média histórica: 19 → 22 min); "média dos últimos 5" subiu para 11,07 min | [06](prints/06-teste4-estimativa-sobe-com-atendimento-lento.jpg), [07](prints/07-painel-online-com-a-placa.jpg) |
| extra | Botão virtual "Registrar chegada" no painel | a placa recebe, valida e o registro mostra origem "painel" | evento 15 `chegada`, origem **painel**, fila 3 → 4 | [08](prints/08-painel-registro-e-botao-virtual.jpg) |
| extra | Pausar a simulação por ~20 s (simula queda de rede) | painel avisa "Posto sem enviar dados há N s" | "Posto sem enviar dados há 22 s"; ao continuar, volta para "Posto online" | [09](prints/09-painel-posto-sem-dados.jpg) |
| extra | Botão físico CHEGADA (GPIO 18) segurado 8 s | **um** evento só (debounce) | evento com origem **botao**, fila +1 | [10](prints/10-botao-fisico-chegada.png) |

Outros prints: [01](prints/01-boot-relatorio-hardware.jpg) relatório de hardware no boot ([zoom](prints/01b-relatorio-hardware-zoom.png)), [02](prints/02-wifi-mqtt-conectado.jpg) Wi-Fi e MQTT conectados, [11](prints/11-painel-com-simulador-plano-b.jpg) painel com o simulador Python (plano B).

### Primeira execução no Wokwi (17/11/2026, projeto ainda não salvo)

O firmware foi colado num projeto novo do Wokwi, compilou sem erro e rodou. Um assinante MQTT no PC registrou o que a placa publicou no broker real:

| Ação na placa | Publicado pela placa (resumo) |
|---|---|
| Boot | Wi-Fi `10.10.0.2`, relatório de hardware, `[mqtt] conectando a broker.hivemq.com ... ok`, estado retido com fila 0 |
| 3 × `c` no Serial (teste 1) | eventos 1–3 `chegada`, fila 1 → 3; estado: IA 16,3 min × média 18,7 min |
| 4 × `a` no Serial (testes 2 e 3) | eventos 4–6 `atendido`, fila 3 → 0; evento 7 **`erro_fila_vazia`** com fila 0; estado: concluídos 3, recusas 1 |
| Comando `chegada` enviado pelo broker (botão virtual) | evento 8 com origem `painel`, já com a hora real do NTP |
| Botão físico CHEGADA segurado 8 s | **um** evento (9, origem `botao`): o debounce não conta repetido |
| Botão físico ATENDIDO | evento 10, origem `botao`, fila 2 → 1 (teste 4: estimativa recalculada a cada evento) |

Achados e correções feitos na hora: (1) a simulação rodava a ~3% da velocidade real, e o OLED sendo redesenhado 10×/s pesava, então agora só redesenha quando algo muda (subiu para ~24%); (2) com a simulação lenta, o keep-alive padrão de 15 s do MQTT derrubava a conexão, que passou para 60 s. Os eventos com `instante` 4 e 5 são anteriores à sincronização do NTP (segundos desde o boot), como previsto.

## 2. Testes automáticos da lógica (feitos em 17/11/2026)

`make test` compila [`testes/teste_fila.cpp`](../testes/teste_fila.cpp) com o **mesmo `firmware/fila.h` da placa** e com AddressSanitizer + UndefinedBehaviorSanitizer.

Resultado: **39 testes, 0 falha(s)**.

| Grupo | Verifica |
|---|---|
| Teste mínimo 1: chegadas | contagem, evento com tipo, fila, horário e número de sequência; início do atendimento |
| Teste mínimo 2: conclusões | contagem, duração medida × `FATOR_TEMPO`, próxima pessoa chamada na hora, média dos últimos |
| Teste mínimo 3: fila vazia | "atendido" com fila 0 recusado; contador continua 0 (não vira 255); recusa registrada; não entra na média |
| Teste mínimo 4: estimativa | 0 com fila vazia; sobe com a fila; fórmula convencional e da IA batem com os coeficientes; atendimento lento aumenta a IA; nunca negativa |
| Limites | fila cheia (99) recusa chegada; registro circular sobrescreve os mais antigos na ordem certa; `millis()` dando a volta |
| Mensagens | JSON de estado e de evento válidos e dentro do buffer; `sizeof(Evento) = 8 B`, `sizeof(Fila) = 2.108 B` |

## 3. Coerência entre firmware e simulador (feito em 17/11/2026)

`make coerencia` roda a mesma sequência de 14 operações (incluindo duas saídas com a fila vazia) na lógica em C++ do ESP32 e no simulador em Python (plano B), e compara fila e as duas estimativas a cada passo: **14/14 iguais**. Assim o plano B mostra exatamente os mesmos números que a placa.

## 4. Comunicação ponta a ponta (feito em 17/11/2026)

1. Simulador publicando no broker público `broker.hivemq.com` (posto de teste `teste-claude-e2e`, 100 s de chegadas e atendimentos automáticos).
2. Painel aberto no navegador, assinando o mesmo tópico por WebSocket/TLS.

| Verificado | Resultado |
|---|---|
| Painel conecta e mostra "Posto online" | ✅ |
| Fila, espera (IA × média), concluídos, chegadas e bloqueadas atualizando sozinhos | ✅ (ex.: fila 5, IA 29 min, média 31 min, 2 concluídos, 7 chegadas, 1 bloqueada) |
| Registro de eventos com horário e origem | ✅ |
| Saída com fila vazia chegando como "Bloqueado: fila vazia" | ✅ (primeira operação do simulador) |
| Botão virtual "Registrar chegada" → comando pela rede → posto valida → painel atualiza | ✅ (evento com origem "painel") |

## 5. Previsão (feito em 17/11/2026)

`make treinar` gera os dados simulados e treina o modelo; o resultado é sempre o mesmo (semente fixa). Teste em 8 dias nunca vistos: erro médio de **1,99 min (IA) × 2,48 min (média histórica)**. Tabela completa em [`ia/resultados.md`](../ia/resultados.md).
