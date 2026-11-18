# ✅ O que falta para tirar 10 em COA (entrega 18/11)

O firmware no Wokwi, o painel publicado, a IA, os testes com prints, o diagrama e o relatório já estão prontos. Faltam 4 coisas, todas da dupla:

## 1. [ ] Escrever a conclusão do relatório (10 min)

1. Abrir [`docs/relatorio-tecnico.md`](docs/relatorio-tecnico.md) (no VS Code, ou no GitHub com o lápis ✏️).
2. No **último parágrafo**, trocar `**[PREENCHER: 2–3 frases da dupla sobre o que aprenderam.]**` por 2–3 frases de vocês. Ideias do que dizer:
   - o que mais surpreendeu: por exemplo, redesenhar o display pelo I2C deixava a simulação lenta;
   - como a teoria (CPU, memória, E/S, protocolos) apareceu no projeto;
   - o que fariam com mais tempo: placa real, broker com senha, dados reais de um posto.
3. Apagar a linha 10, que começa com `> **Antes de entregar:**`.
4. Se o Portal pedir PDF: abrir o arquivo no GitHub → `Ctrl+P` → *Salvar como PDF*.

**Pronto quando:** não aparece mais "PREENCHER" no relatório.

## 2. [ ] Subir o vídeo (até 5 min)

> **Vídeo pronto, com a sua voz:** `video/coa-demonstracao-com-voz.mp4` (3:47), com a gravação real do Wokwi e do painel, legendas e as 18 falas encaixadas no tempo de cada uma. Só assistir uma vez, subir no YouTube como **não listado** e colar o link no README (item 3). Os `.mp4` e os áudios não vão para o GitHub.

**Se preferirem gravar a tela vocês mesmos** (em vez de usar o vídeo pronto):

1. Abrir duas janelas lado a lado:
   - o Wokwi: https://wokwi.com/projects/480979792844066817
   - o painel: https://gabrielroma0.github.io/GS2-COA/painel/
2. Clicar em ▶ no Wokwi **antes de gravar** e esperar no Monitor Serial a linha `[mqtt] conectando a broker.hivemq.com ... ok`. O painel passa a mostrar "Posto online".
3. Seguir o roteiro [`video/roteiro.md`](video/roteiro.md): problema → arquitetura → testes 1 a 4 → IA × média → painel e botão virtual → memória (tecla `m`) → limitações.
4. Dicas:
   - deixar a aba do Wokwi em primeiro plano, senão a simulação fica lenta;
   - nos botões do Wokwi, segurar o clique por meio segundo;
   - se um aperto não contar, usar as teclas `c` (chegada) e `a` (atendido) no Monitor Serial;
   - para o teste 3, atender até zerar e apertar ATENDIDO mais uma vez (aparece o LED vermelho e "Fila vazia!").
5. Gravar com o **OBS Studio** (grátis, obsproject.com).
6. Subir no YouTube como **não listado**: studio.youtube.com → *Criar → Enviar vídeos* → em *Visibilidade*, escolher **Não listado**.

**Pronto quando:** o vídeo tem até 5:00 e o link abre numa aba anônima.

## 3. [ ] Colar o link do vídeo no README

No GitHub: abrir [`README.md`](README.md) → lápis ✏️ → trocar a linha `- **Vídeo (até 5 min):** _a gravar..._` por `- **Vídeo (até 5 min):** <link do YouTube>` → *Commit changes*.

## 4. [ ] (1 min, opcional) Corrigir a turma no Wokwi

No projeto do Wokwi (logado), no `sketch.ino`, linha 3: trocar `1CC-FEV` por `1CCR` e clicar em **Save**. É só um comentário, então não muda o funcionamento; o arquivo no GitHub já está certo.

## 5. [ ] Entregar no Portal

1. Conferir no Portal o formato pedido: link do repositório, ZIP, PDF do relatório ou `.txt`.
2. Mandar:
   - o nome completo e o RM dos dois;
   - o repositório: https://github.com/GabrielRoma0/GS2-COA
   - o vídeo;
   - o Wokwi e o painel, se houver campo.
   - Se for ZIP: no GitHub, *Code → Download ZIP*.

**Pronto quando:** o Portal mostra a entrega como enviada.
