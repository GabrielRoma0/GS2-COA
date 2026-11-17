# Testes e evidências

## 1. Os 4 testes mínimos do enunciado, na placa (Wokwi)

**A dupla executa no Wokwi com o painel aberto ao lado, tira um print de cada passo (OLED + painel + Monitor Serial) e salva em `docs/prints/`.** Antes, abrir o Monitor Serial e esperar aparecer `[mqtt] conectando a broker.hivemq.com ... ok`.

| # | Passo | Resultado esperado | Observado / print |
|---|---|---|---|
| 1 | **Chegada de novos cidadãos:** apertar CHEGADA 3 vezes, com ~5 s entre elas | OLED e painel: fila 1 → 2 → 3; LED verde pisca a cada vez; 3 linhas "Chegada" no registro do painel, com horário; Serial `[evento] {"tipo":"chegada"...}` | **[PREENCHER]** |
| 2 | **Conclusão de atendimentos:** esperar ~6 s e apertar ATENDIDO 2 vezes (com ~6 s entre elas) | fila 3 → 2 → 1; "atendidos" 0 → 1 → 2; "Atendimentos medidos pela placa" no painel sobe; "Média dos últimos 5" fica perto de 6 min (modo demo: 6 s = 6 min) | **[PREENCHER]** |
| 3 | **Saída com fila vazia:** apertar ATENDIDO até zerar e mais uma vez | a última tentativa é **recusada**: fila continua 0 (não fica −1), LED **vermelho**, OLED "Fila vazia!", painel mostra "Bloqueado: fila vazia" em vermelho no registro e "operações bloqueadas" sobe para 1 | **[PREENCHER]** |
| 4 | **Atualização da estimativa:** com a fila em 0, apertar CHEGADA 4 vezes; depois fazer 2 atendimentos **lentos** (~12 s cada) e comparar | a cada chegada as duas estimativas sobem (IA e média); depois dos atendimentos lentos, a "Média dos últimos 5" sobe e a previsão da IA aumenta para o mesmo tamanho de fila | **[PREENCHER]** |
| extra | Botão virtual "Registrar chegada" no painel | a placa recebe, valida e o OLED muda; o registro mostra origem "painel" | **[PREENCHER]** |
| extra | Desligar o Wi-Fi não é possível no Wokwi; em vez disso, pausar a simulação por 20 s | painel mostra "Posto sem enviar dados há N s"; ao continuar volta para "Posto online" | **[PREENCHER]** |

Dica: tecla `l` no Monitor Serial lista o registro de eventos guardado na RAM; tecla `m` mostra o relatório de CPU e memória (vai para o relatório técnico, seção 6).

### Execução de verificação no Wokwi (feita em 17/11/2026, antes dos prints)

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
