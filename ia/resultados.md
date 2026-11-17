# Resultados da previsao de espera

> Gerado por `ia/treinar_regressao.py`. **Dados simulados** (`dados/historico_simulado.csv`, semente fixa), nao de um posto real.

- Treino: dias 1 a 22 (1201 chegadas). Teste: dias 23 a 30 (426 chegadas), nunca vistos no treino.
- Media historica de atendimento (treino): **6.24 min**.
- Modelo de IA: `espera = -2.008 + 4.929 x fila + 0.188 x (fila x media_ult5)`

## Erro no conjunto de teste (minutos)

| Grupo | n | MAE conv. | MAE IA | RMSE conv. | RMSE IA | R2 conv. | R2 IA |
|---|---:|---:|---:|---:|---:|---:|---:|
| todos | 426 | 2.48 | 1.99 | 4.09 | 3.49 | 0.950 | 0.964 |
| fila >= 3 | 119 | 4.77 | 4.34 | 6.54 | 5.86 | 0.885 | 0.908 |

MAE = erro medio absoluto (em media, quantos minutos a previsao erra). RMSE pune mais os erros grandes. R2 = quanto da variacao da espera o metodo explica (1 = perfeito).

## Exemplos de previsao (min)

| Fila | Media dos ultimos 5 (min) | Convencional | IA |
|---:|---:|---:|---:|
| 1 | 4.5 | 6.2 | 3.8 |
| 1 | 7.5 | 6.2 | 4.3 |
| 3 | 4.5 | 18.7 | 15.3 |
| 3 | 7.5 | 18.7 | 17.0 |
| 5 | 4.5 | 31.2 | 26.9 |
| 5 | 7.5 | 31.2 | 29.7 |
| 8 | 4.5 | 49.9 | 44.2 |
| 8 | 7.5 | 49.9 | 48.7 |

## Leitura dos resultados

- A convencional **superestima**: conta a pessoa que ja esta sendo atendida como se ainda faltasse o atendimento inteiro dela. A regressao aprendeu essa correcao sozinha (b0 = -2.01 min e b1 = 4.93 min por pessoa, menor que a media de 6.24).
- O termo do ritmo recente (b2) e pequeno mas positivo: com o posto lento (pico) a IA aumenta a previsao; a convencional da sempre o mesmo numero para a mesma fila.
- O ganho e moderado (cerca de 20% menos erro medio) porque a fila simulada e simples (um atendente, sem faltas nem prioridades). Com dados reais, o modelo teria de ser retreinado e comparado de novo.
