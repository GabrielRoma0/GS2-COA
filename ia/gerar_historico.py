"""
Gera o historico SIMULADO de um posto de atendimento a pessoa com deficiencia.

ATENCAO: os dados sao SIMULADOS (o enunciado permite, desde que identificados).
Nao representam nenhum posto real. Servem para treinar e comparar os metodos
de previsao do tempo de espera.

Modelo da simulacao (fila M/G/1, um atendente):
  - posto aberto das 8h as 17h, 30 dias uteis;
  - chegadas aleatorias (processo de Poisson) com taxa maior no pico da manha
    e no inicio da tarde;
  - tempo de atendimento aleatorio (lognormal), mais demorado no pico
    (casos com mais documentos) e com um custo fixo de chamada/deslocamento.

Para cada pessoa que chega, grava:
  dia, hora_chegada (min desde 0h), fila_na_chegada (pessoas no posto, contando
  quem esta sendo atendido), media_ult5_min (media dos ultimos 5 atendimentos
  concluidos ate aquele momento), espera_real_min (ate ser chamada).

Uso:  python3 ia/gerar_historico.py   ->  dados/historico_simulado.csv
So usa a biblioteca padrao do Python.
"""
import csv
import math
import os
import random

SEMENTE = 2026          # semente fixa: o mesmo CSV sai em qualquer computador
DIAS = 30
ABRE, FECHA = 8 * 60, 17 * 60   # minutos desde a meia-noite

PASTA = os.path.dirname(os.path.abspath(__file__))
SAIDA = os.path.join(PASTA, "..", "dados", "historico_simulado.csv")


def taxa_chegada(minuto):
    """Pessoas por minuto, conforme a hora do dia."""
    h = minuto / 60
    if 9 <= h < 11.5:
        return 1 / 7.5      # pico da manha: 1 pessoa a cada 7,5 min
    if 13.5 <= h < 15:
        return 1 / 8.5      # pico da tarde
    return 1 / 13.0


def duracao_atendimento(minuto, rng):
    """Duracao do atendimento em minutos (lognormal com media por horario)."""
    h = minuto / 60
    media = 6.5 if (9 <= h < 11.5 or 13.5 <= h < 15) else 4.5
    sigma = 0.35
    mu = math.log(media) - sigma ** 2 / 2   # faz a media da lognormal ser 'media'
    return 0.8 + rng.lognormvariate(mu, sigma)   # 0,8 min: chamada + deslocamento


def simular_dia(dia, rng):
    # 1) gera os horarios de chegada (Poisson com taxa variavel, por "thinning")
    chegadas, t = [], ABRE
    taxa_max = 1 / 7.5
    while True:
        t += rng.expovariate(taxa_max)
        if t >= FECHA:
            break
        if rng.random() < taxa_chegada(t) / taxa_max:
            chegadas.append(t)

    # 2) atende na ordem de chegada (um atendente)
    linhas = []
    livre_em = ABRE               # quando o atendente fica livre
    inicios, fins, duracoes = [], [], []
    for i, cheg in enumerate(chegadas):
        # quantas pessoas ainda estao no posto quando esta chega
        fila = sum(1 for j in range(i) if fins[j] > cheg)
        # media dos ultimos 5 atendimentos ja concluidos (o que o ESP32 sabe)
        concluidos = [duracoes[j] for j in range(i) if fins[j] <= cheg][-5:]
        media5 = sum(concluidos) / len(concluidos) if concluidos else float("nan")

        inicio = max(cheg, livre_em)
        dur = duracao_atendimento(inicio, rng)
        fim = inicio + dur
        livre_em = fim
        inicios.append(inicio); fins.append(fim); duracoes.append(dur)

        linhas.append({
            "dia": dia,
            "hora_chegada": round(cheg, 2),
            "fila_na_chegada": fila,
            "media_ult5_min": "" if math.isnan(media5) else round(media5, 3),
            "duracao_atendimento_min": round(dur, 3),
            "espera_real_min": round(inicio - cheg, 3),
        })
    return linhas


def main():
    rng = random.Random(SEMENTE)
    todas = []
    for dia in range(1, DIAS + 1):
        todas.extend(simular_dia(dia, rng))
    os.makedirs(os.path.dirname(SAIDA), exist_ok=True)
    with open(SAIDA, "w", newline="", encoding="utf-8") as f:
        f.write("# DADOS SIMULADOS - gerados por ia/gerar_historico.py (semente %d). "
                "Nao sao dados de um posto real.\n" % SEMENTE)
        w = csv.DictWriter(f, fieldnames=list(todas[0].keys()))
        w.writeheader()
        w.writerows(todas)
    filas = [l["fila_na_chegada"] for l in todas]
    esperas = [l["espera_real_min"] for l in todas]
    print(f"{len(todas)} chegadas simuladas em {DIAS} dias -> {os.path.relpath(SAIDA)}")
    print(f"fila na chegada: media {sum(filas)/len(filas):.1f}, maxima {max(filas)}")
    print(f"espera real: media {sum(esperas)/len(esperas):.1f} min, maxima {max(esperas):.1f} min")


if __name__ == "__main__":
    main()
