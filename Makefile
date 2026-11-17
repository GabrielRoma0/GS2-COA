# Fila virtual (COA, GS 2026.2)
# make test       testes da logica da fila (firmware/fila.h) no PC, com AddressSanitizer
# make coerencia  confere se o simulador Python calcula igual ao firmware
# make treinar    gera o historico simulado e treina a regressao (atualiza firmware/modelo.h)
# make painel     serve o painel em http://localhost:8765
# make simulador  posto simulado (plano B) gerando eventos por 120 s

PY = .venv/bin/python

test:
	g++ -std=c++17 -Wall -Wextra -g -fsanitize=address,undefined -Ifirmware testes/teste_fila.cpp -o testes/teste_fila
	./testes/teste_fila

$(PY):
	python3 -m venv .venv && $(PY) -m pip install -q paho-mqtt==2.1.0

coerencia: $(PY)
	$(PY) testes/coerencia.py

treinar:
	python3 ia/gerar_historico.py
	python3 ia/treinar_regressao.py

painel:
	cd painel && python3 -m http.server 8765

simulador: $(PY)
	$(PY) simulador/posto_simulado.py --auto 120

.PHONY: test coerencia treinar painel simulador
