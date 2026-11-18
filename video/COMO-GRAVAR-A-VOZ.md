# Como gravar a voz por cima do vídeo

O vídeo [`coa-demonstracao-legendada.mp4`](coa-demonstracao-legendada.mp4) já está pronto e legendado, mas **sem som**. As falas, com o tempo de cada uma e quem fala, estão em [`falas.md`](falas.md). A legenda mostra exatamente o texto que deve ser lido.

O vídeo foi montado a partir de uma execução **real** do programa: o lado esquerdo é a gravação do projeto salvo no Wokwi rodando, e o lado direito é o painel publicado recebendo os dados pela internet. Os botões foram apertados no tempo de cada fala.

## Jeito mais fácil: Clipchamp (já vem no Windows 11) ou CapCut

1. Abrir o Clipchamp (ou o CapCut) → **Criar vídeo** → importar o `.mp4`.
2. Arrastar o vídeo para a linha do tempo.
3. **Gravar e criar → Áudio** (no CapCut, o ícone de microfone, "Narração"): dar play e ler cada legenda quando ela aparecer.
   - Dá para gravar por partes (pausar e continuar), se ficar mais fácil.
4. Ajustar o áudio na linha do tempo se alguma fala ficar adiantada ou atrasada.
5. **Exportar** em 1080p.

## Jeito alternativo: gravar só o áudio e juntar com um comando

1. Abrir o vídeo em tela cheia e, ao mesmo tempo, gravar a voz no celular ou no Audacity, lendo as legendas.
2. Salvar como `voz.mp3` na pasta `video/`.
3. Juntar o vídeo e a voz:

```bash
ffmpeg -i coa-demonstracao-legendada.mp4 -i voz.mp3 -c:v copy -c:a aac -shortest coa-final.mp4
```

## Depois

- Subir no YouTube como **não listado** (studio.youtube.com → Criar → Enviar vídeos → Visibilidade: Não listado).
- O arquivo `.srt` serve se quiserem legendas "de verdade" no YouTube (Legendas → Enviar arquivo), mas não é necessário: a legenda já está gravada na imagem.
- Colar o link no README e entregar no Portal (ver `PENDENCIAS.md`).

## Dicas

- Falar num ritmo calmo: cada legenda fica na tela o tempo suficiente para lê-la sem correr.
- Se uma fala ficar comprida demais para vocês, podem resumir; o importante é explicar o que está na tela.
- Todas as falas são do Gabriel; a abertura cita o Thor como integrante da dupla.
- O vídeo tem 3:48, dentro do limite de 5 minutos.
