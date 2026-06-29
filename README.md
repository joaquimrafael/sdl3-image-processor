# Image Processor (C + SDL3)

Processador de imagens em C usando a biblioteca **SDL3** (Simple DirectMedia Layer). Carrega uma imagem, exibe seu histograma, aplica equalização para melhorar o contraste, converte para tons de cinza e classifica a imagem por brilho e contraste a partir de estatísticas da própria distribuição de pixels.

## Índice

- [Descrição geral do projeto](#descrição-geral-do-projeto)
- [Funcionamento do projeto](#funcionamento-do-projeto)
- [Explicação detalhada](#explicação-detalhada)
  - [Histograma](#histograma)
  - [Classificação de imagem (brilho e contraste)](#classificação-de-imagem-brilho-e-contraste)
  - [Equalização de histograma](#equalização-de-histograma)
- [Compilação e execução](#compilação-e-execução)
  - [MacOS](#macos)
  - [Windows](#windows)
- [Contexto acadêmico](#contexto-acadêmico)

## Descrição geral do projeto

Este projeto tem como propósito desenvolver um software de processamento de imagens em linguagem C, usando a biblioteca SDL (Simple DirectMedia Layer) em sua versão 3.

## Funcionamento do projeto

### 1. Entrada do programa

O programa recebe o nome da imagem como argumento na linha de comando.

### 2. Janelas de exibição

**Janela principal**: exibe a imagem carregada, podendo ser a versão **original** ou **equalizada**.

**Janela secundária**: exibe:
- Botão de alternância entre os modos "Original" e "Equalizado".
- Histograma com a distribuição de intensidades da imagem.
- Análise de brilho e contraste obtida a partir de estatísticas da imagem.

### 3. Funcionalidades extras

- Conversão automática de imagens coloridas para tons de cinza.
- Salvamento da imagem exibida pressionando a tecla **S**, gerando o arquivo `output.png`.
- Interatividade por botão e mudança de cursor ao passar sobre elementos interativos.

## Explicação detalhada

### Histograma

O histograma foi implementado para mostrar a distribuição de intensidades de cinza da imagem:

- **Contagem de intensidades**: cada pixel é convertido para escala de cinza (quando necessário) e seu valor (0 a 255) é contabilizado.
- **Normalização para proporção (%)**: a frequência é convertida em proporção relativa para permitir comparações entre imagens diferentes:

```
proporcao[i] = (contagem[i] / total_pixels) * 100
```

**Escalonamento no gráfico**:

- Cada intensidade (0 a 255) é representada por uma barra.
- A altura da barra é proporcional à frequência normalizada, limitada a 200px de altura.
- Isso garante que a barra mais alta ocupe todo o espaço disponível e as demais fiquem proporcionais.

**Renderização**:

- Eixo X: valores de intensidade (0 a 255).
- Eixo Y: frequência relativa.
- O gráfico é centralizado na janela secundária e desenhado em preto, com eixos destacados em rosa.

### Classificação de imagem (brilho e contraste)

A classificação de uma imagem em termos de brilho e contraste se dá pela estatística da distribuição de intensidade de cada pixel da imagem.

A métrica do brilho é a média, então calculamos a média da intensidade dos pixels e classificamos de acordo com o resultado:

```
media = soma(intensidade) / total_pixels
```

- **Imagem escura**: média < 85
- **Imagem média**: média entre 85 e 170
- **Imagem clara**: média > 170

A métrica do contraste é o desvio padrão, então calculamos e classificamos de acordo com o resultado:

```
variancia = (soma(intensidade²) / total_pixels) - (media²)
desvio = sqrt(variancia)
```

- **Contraste baixo**: desvio < 50
- **Contraste médio**: desvio entre 50 e 100
- **Contraste alto**: desvio > 100

As duas métricas de classificação são exibidas no topo da janela secundária, facilitando uma análise rápida da imagem.

### Equalização de histograma

No projeto, a equalização foi usada para melhorar o contraste da imagem, de acordo com o seguinte processo:

- **Construção do histograma original**: contagem da frequência de cada intensidade.
- **Cálculo da CDF (função de distribuição acumulada)**:

```
cdf[i] = cdf[i-1] + hist[i]
```

- **Normalização da CDF (gerando uma tabela de transformação LUT)**:

```
lut[i] = (cdf[i] * 255) / total_pixels
```

A transformação serve para redistribuir os pixels no histograma para que fiquem mais espalhados ao longo de toda a faixa.

- **Aplicação da LUT**: cada pixel é substituído pelo valor mapeado em `lut[intensidade]`.
- **Resultado**: depois da equalização, o contraste da imagem se apresenta melhor, e o histograma mostra uma distribuição mais uniforme.

## Compilação e execução

### MacOS

```bash
# Compilar
gcc -o image_processor main.c $(pkgconf --cflags --libs sdl3 sdl3-image sdl3-ttf)

# Executar
./image_processor <nome_do_arquivo>
```

### Windows

1. Baixar as bibliotecas SDL3, SDL3_image e SDL3_ttf e adicionar as respectivas `.dll` na pasta do projeto.
2. Compilar:

```bash
gcc main.c -I<path_para_includes> -L<path_para_libs> -lSDL3 -lSDL3_image -lSDL3_ttf -o image_processor
```

3. Executar:

```bash
./image_processor img/<imagem_escolhida>
```

## Contexto acadêmico

Projeto desenvolvido na disciplina de **Computação Visual**, Ciência da Computação, Universidade Presbiteriana Mackenzie (Turma 07N, 2025.2).

### Grupo

- Antonio Carlos Sciamarelli Neto - 10409160
- Gustavo Matta - 10410154
- Joaquim Rafael Mariano Prieto Pereira - 10408805
- Lucas Trebacchetti Eiras - 10401973

### Contribuições individuais

**Antonio**
- Documentação
- Criação do histograma
- Escalas de cinza
- Configuração do botão
- Refinamento

**Gustavo**
- Documentação
- Consulta teórica
- Carregamento da imagem
- Salvamento da imagem output
- Code review

**Joaquim**
- Arquitetura do projeto
- Criação do repositório e organização dos arquivos
- Carregamento da imagem e adaptação do tamanho da janela
- Criação da janela secundária
- Cálculo do histograma e equalização
- Salvamento da imagem output
- Interação com o botão
- Exibição das características da imagem
- Code review e refinamentos

**Lucas**
- Conversão e detecção de tons de cinza
- Geração da janela secundária
- Criação do histograma
- Equalização
- Configuração do botão
- Configuração dos eventos
- Code review e refinamentos
