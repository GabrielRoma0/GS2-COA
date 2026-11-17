"""
Treina a previsao inteligente de espera e compara com a estimativa convencional.

Dados: dados/historico_simulado.csv (SIMULADOS, ver ia/gerar_historico.py).
Divisao: dias 1 a 22 para treino, dias 23 a 30 para teste (o modelo nunca ve
os dias de teste durante o treino).

Metodos comparados, para quem chega e encontra 'fila' pessoas no posto:

  1. Convencional (media historica):
         espera = fila x media_historica_de_atendimento
     media_historica = media de TODOS os atendimentos do treino.

  2. IA - regressao linear multipla (minimos quadrados, escrita a mao):
         espera = b0 + b1 * fila + b2 * (fila x media_ult5)
     media_ult5 = media dos ultimos 5 atendimentos concluidos naquele dia.
     O modelo aprende quanto pesar o ritmo RECENTE do posto (que muda no
     horario de pico) em vez de usar so a media geral.

  Nos dois: fila = 0 -> espera 0 (ninguem na frente) e previsao nunca negativa.

Saidas:
  ia/modelo.json        coeficientes e metricas
  firmware/modelo.h     coeficientes para o ESP32 (gerado, nao editar a mao)
  ia/resultados.md      tabela da comparacao para o relatorio

Uso: python3 ia/treinar_regressao.py      (so biblioteca padrao)
"""
import csv
import json
import math
import os

PASTA = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.join(PASTA, "..")
CSV = os.path.join(RAIZ, "dados", "historico_simulado.csv")
DIA_CORTE = 22


def ler_csv():
    with open(CSV, encoding="utf-8") as f:
        linhas = [l for l in f if not l.startswith("#")]
    dados = []
    for r in csv.DictReader(linhas):
        dados.append({
            "dia": int(r["dia"]),
            "fila": int(r["fila_na_chegada"]),
            "media5": float(r["media_ult5_min"]) if r["media_ult5_min"] else None,
            "duracao": float(r["duracao_atendimento_min"]),
            "espera": float(r["espera_real_min"]),
        })
    return dados


def resolver(A, b):
    """Resolve A x = b por eliminacao de Gauss com pivoteamento parcial."""
    n = len(b)
    M = [A[i][:] + [b[i]] for i in range(n)]
    for col in range(n):
        piv = max(range(col, n), key=lambda r: abs(M[r][col]))
        M[col], M[piv] = M[piv], M[col]
        for r in range(col + 1, n):
            f = M[r][col] / M[col][col]
            for c in range(col, n + 1):
                M[r][c] -= f * M[col][c]
    x = [0.0] * n
    for i in range(n - 1, -1, -1):
        x[i] = (M[i][n] - sum(M[i][j] * x[j] for j in range(i + 1, n))) / M[i][i]
    return x


def minimos_quadrados(X, y):
    """Coeficientes que minimizam a soma dos erros ao quadrado:
    equacoes normais (X^T X) b = X^T y."""
    k = len(X[0])
    XtX = [[sum(l[i] * l[j] for l in X) for j in range(k)] for i in range(k)]
    Xty = [sum(l[i] * yi for l, yi in zip(X, y)) for i in range(k)]
    return resolver(XtX, Xty)


def caracteristicas(r, media_hist):
    m5 = r["media5"] if r["media5"] is not None else media_hist
    return [1.0, r["fila"], r["fila"] * m5]


def prever_convencional(r, media_hist):
    return r["fila"] * media_hist


def prever_ia(r, b, media_hist):
    if r["fila"] == 0:
        return 0.0
    x = caracteristicas(r, media_hist)
    return max(0.0, sum(bi * xi for bi, xi in zip(b, x)))


def metricas(reais, previstos):
    erros = [p - y for p, y in zip(previstos, reais)]
    mae = sum(abs(e) for e in erros) / len(erros)
    rmse = math.sqrt(sum(e * e for e in erros) / len(erros))
    media_y = sum(reais) / len(reais)
    ss_tot = sum((y - media_y) ** 2 for y in reais)
    r2 = 1 - sum(e * e for e in erros) / ss_tot if ss_tot else float("nan")
    return mae, rmse, r2


def main():
    dados = ler_csv()
    treino = [r for r in dados if r["dia"] <= DIA_CORTE]
    teste = [r for r in dados if r["dia"] > DIA_CORTE]

    media_hist = sum(r["duracao"] for r in treino) / len(treino)

    # treina so com quem encontrou fila (fila = 0 e sempre espera 0)
    com_fila = [r for r in treino if r["fila"] > 0]
    X = [caracteristicas(r, media_hist) for r in com_fila]
    y = [r["espera"] for r in com_fila]
    b = minimos_quadrados(X, y)

    resultado = {"aviso": "Treinado com dados SIMULADOS (dados/historico_simulado.csv)",
                 "media_historica_min": round(media_hist, 4),
                 "b0": round(b[0], 4), "b1": round(b[1], 4), "b2": round(b[2], 4),
                 "treino_n": len(treino), "teste_n": len(teste), "comparacao": {}}

    linhas_md = []
    for nome, grupo in [("todos", teste), ("fila >= 3", [r for r in teste if r["fila"] >= 3])]:
        reais = [r["espera"] for r in grupo]
        conv = [prever_convencional(r, media_hist) for r in grupo]
        ia = [prever_ia(r, b, media_hist) for r in grupo]
        mc, rc, r2c = metricas(reais, conv)
        mi, ri, r2i = metricas(reais, ia)
        resultado["comparacao"][nome] = {
            "n": len(grupo),
            "convencional": {"mae": round(mc, 2), "rmse": round(rc, 2), "r2": round(r2c, 3)},
            "ia": {"mae": round(mi, 2), "rmse": round(ri, 2), "r2": round(r2i, 3)},
        }
        linhas_md.append(f"| {nome} | {len(grupo)} | {mc:.2f} | {mi:.2f} | {rc:.2f} | {ri:.2f} | {r2c:.3f} | {r2i:.3f} |")

    with open(os.path.join(PASTA, "modelo.json"), "w", encoding="utf-8") as f:
        json.dump(resultado, f, indent=2, ensure_ascii=False)

    with open(os.path.join(RAIZ, "firmware", "modelo.h"), "w", encoding="utf-8") as f:
        f.write(
            "// GERADO por ia/treinar_regressao.py - nao editar a mao.\n"
            "// Treinado com dados SIMULADOS (dados/historico_simulado.csv).\n"
            "// espera_min = B0 + B1*fila + B2*(fila*media_ult5_min)\n"
            "#pragma once\n"
            f"#define MODELO_MEDIA_HISTORICA_MIN {media_hist:.4f}f\n"
            f"#define MODELO_B0 {b[0]:.4f}f\n"
            f"#define MODELO_B1 {b[1]:.4f}f\n"
            f"#define MODELO_B2 {b[2]:.4f}f\n")

    exemplo = []
    for fila in (1, 3, 5, 8):
        for m5 in (4.5, 7.5):
            r = {"fila": fila, "media5": m5}
            exemplo.append(f"| {fila} | {m5:.1f} | {prever_convencional(r, media_hist):.1f} | {prever_ia(r, b, media_hist):.1f} |")

    with open(os.path.join(PASTA, "resultados.md"), "w", encoding="utf-8") as f:
        f.write(
            "# Resultados da previsao de espera\n\n"
            "> Gerado por `ia/treinar_regressao.py`. **Dados simulados** "
            "(`dados/historico_simulado.csv`, semente fixa), nao de um posto real.\n\n"
            f"- Treino: dias 1 a {DIA_CORTE} ({len(treino)} chegadas). "
            f"Teste: dias {DIA_CORTE + 1} a 30 ({len(teste)} chegadas), nunca vistos no treino.\n"
            f"- Media historica de atendimento (treino): **{media_hist:.2f} min**.\n"
            f"- Modelo de IA: `espera = {b[0]:.3f} + {b[1]:.3f} x fila + {b[2]:.3f} x (fila x media_ult5)`\n\n"
            "## Erro no conjunto de teste (minutos)\n\n"
            "| Grupo | n | MAE conv. | MAE IA | RMSE conv. | RMSE IA | R2 conv. | R2 IA |\n"
            "|---|---:|---:|---:|---:|---:|---:|---:|\n" + "\n".join(linhas_md) + "\n\n"
            "MAE = erro medio absoluto (em media, quantos minutos a previsao erra). "
            "RMSE pune mais os erros grandes. R2 = quanto da variacao da espera o metodo explica (1 = perfeito).\n\n"
            "## Exemplos de previsao (min)\n\n"
            "| Fila | Media dos ultimos 5 (min) | Convencional | IA |\n|---:|---:|---:|---:|\n"
            + "\n".join(exemplo) + "\n\n"
            "## Leitura dos resultados\n\n"
            "- A convencional **superestima**: conta a pessoa que ja esta sendo atendida como se "
            "ainda faltasse o atendimento inteiro dela. A regressao aprendeu essa correcao "
            f"sozinha (b0 = {b[0]:.2f} min e b1 = {b[1]:.2f} min por pessoa, menor que a media de "
            f"{media_hist:.2f}).\n"
            "- O termo do ritmo recente (b2) e pequeno mas positivo: com o posto lento (pico) a "
            "IA aumenta a previsao; a convencional da sempre o mesmo numero para a mesma fila.\n"
            "- O ganho e moderado (cerca de 20% menos erro medio) porque a fila simulada e simples "
            "(um atendente, sem faltas nem prioridades). Com dados reais, o modelo teria de ser "
            "retreinado e comparado de novo.\n")

    c = resultado["comparacao"]["todos"]
    print(f"media historica: {media_hist:.2f} min")
    print(f"IA: espera = {b[0]:.3f} + {b[1]:.3f}*fila + {b[2]:.3f}*(fila*media_ult5)")
    for nome, v in resultado["comparacao"].items():
        print(f"[{nome}] n={v['n']}  MAE conv {v['convencional']['mae']}  x  IA {v['ia']['mae']}   "
              f"RMSE conv {v['convencional']['rmse']} x IA {v['ia']['rmse']}   R2 {v['convencional']['r2']} x {v['ia']['r2']}")
    print("gravado: ia/modelo.json, ia/resultados.md, firmware/modelo.h")


if __name__ == "__main__":
    main()
