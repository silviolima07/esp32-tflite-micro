# DHT22 + ESP32-S3 + TinyML INT8 — Problemas Encontrados e Lições Aprendidas

Durante o desenvolvimento do projeto `dht22_alerta_climatico`, vários problemas não estavam relacionados diretamente ao modelo de Machine Learning, mas ao ambiente ESP-IDF, dependências, cache, simulador Wokwi e integração do sensor.

Esta documentação registra os principais problemas encontrados e as respectivas soluções para evitar que sejam repetidos em projetos futuros.

---

## 1. Reaproveitamento do projeto Hello World

Inicialmente o projeto foi criado copiando o exemplo anterior de TensorFlow Lite Micro `hello_world`.

Isso acabou trazendo vários arquivos e configurações que não pertenciam ao novo projeto, como:

- `model.cc`
- `model.h`
- `constants.cc`
- `constants.h`
- `main_functions.cc`
- `main_functions.h`
- `output_handler.cc`
- `output_handler.h`
- modelos `.tflite` antigos
- referências antigas no `CMakeLists.txt`
- nome antigo `hello_world`
- configurações antigas no `sdkconfig`

### Problema

O ESP-IDF continuava tentando compilar arquivos ou usar configurações do projeto anterior.

### Solução

Para um novo projeto, aproveitar apenas a estrutura necessária.

Estrutura final:

```text
dht22_alerta_climatico/
├── CMakeLists.txt
├── sdkconfig
├── diagram.json
├── wokwi.toml
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── main.cc
    ├── model.cc
    └── model.h
```

O `CMakeLists.txt` principal deve conter:

```cmake
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(dht22_alerta_climatico)
```

E o `main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS
        "main.cc"
        "model.cc"
    INCLUDE_DIRS "."
)
```

### Lição

Para um projeto novo, é mais seguro começar com uma estrutura ESP-IDF mínima e copiar apenas aquilo que realmente será reutilizado.

---

## 2. Problema com caminhos contendo espaços

O ESP-IDF inicialmente utilizava ferramentas localizadas em:

```text
C:\Users\Silvio Cesar\.espressif
```

O espaço existente em `Silvio Cesar` causou problemas em algumas chamadas das ferramentas e complicou o diagnóstico do ambiente.

### Solução adotada

Foi utilizado:

```text
IDF_TOOLS_PATH=C:\Espressif
```

O ambiente Python passou a ficar em:

```text
C:\Espressif\python_env\idf6.1_py3.12_env\
```

O comando confiável para executar o ESP-IDF ficou:

```powershell
& "C:\Espressif\python_env\idf6.1_py3.12_env\Scripts\python.exe" `
  "C:\esp\v6.1\esp-idf\tools\idf.py" build
```

### Lição

Sempre que possível, utilizar caminhos simples para ferramentas de compilação:

```text
C:\Espressif
C:\esp
D:\EMBARCADOS
```

Evitar espaços e caracteres especiais.

---

## 3. Existiam dois `idf.py`

O comando:

```powershell
where.exe idf.py
```

mostrou mais de uma instalação.

Por isso:

```powershell
idf.py --version
```

não necessariamente executava o ESP-IDF esperado.

### Solução

Executar explicitamente o Python e o `idf.py` da instalação correta.

### Lição

Antes de investigar erros estranhos de compilação, verificar:

```powershell
where.exe idf.py
where.exe python
```

---

## 4. Extensão do VS Code modificando arquivos README

Foi encontrado um problema particularmente difícil de diagnosticar.

Uma extensão do VS Code (`dreamlight.aline`) modificava automaticamente arquivos Markdown adicionando conteúdo semelhante a:

```yaml
---
noteId: "..."
tags: []
---
```

Isso também ocorreu dentro de:

```text
managed_components/
```

### Consequência

O ESP-IDF Component Manager verifica SHA256 dos componentes baixados.

Como o `README.md` do componente era alterado pela extensão, o hash deixava de corresponder ao arquivo original.

Isso gerava erros de integridade em componentes como:

```text
esp-tflite-micro
esp-nn
dht
```

### Solução

Desabilitar/remover a extensão responsável por alterar automaticamente Markdown.

Também foi considerada problemática a extensão:

```text
eighthundreds.notebook
```

Após desabilitar as extensões, os componentes precisaram ser baixados novamente.

### Lição

Nunca permitir que extensões do editor modifiquem arquivos automaticamente dentro de:

```text
managed_components/
```

Essa pasta deve ser considerada gerenciada exclusivamente pelo ESP-IDF.

---

## 5. Cache impedindo builds corretos

Diversas alterações de dependências e configurações não apareciam imediatamente porque arquivos antigos permaneciam no cache.

Os principais locais envolvidos foram:

```text
build/
managed_components/
dependencies.lock
```

Em determinados momentos, o projeto continuava usando configurações de builds anteriores.

### Procedimento de limpeza

Quando houver mudança significativa de:

- componente;
- target;
- modelo;
- CMake;
- dependências;
- configuração do ESP-IDF;

considerar limpar:

```powershell
Remove-Item -Recurse -Force build
```

Quando necessário, também regenerar:

```text
managed_components/
dependencies.lock
```

com cuidado, permitindo que o Component Manager baixe novamente as dependências.

### Lição

Antes de investigar um comportamento aparentemente impossível, eliminar a possibilidade de estar compilando artefatos antigos.

---

## 6. Build executado dentro da pasta `main`

Em determinado momento foi executado:

```text
dht22_alerta_climatico\main> idf.py build
```

O ESP-IDF passou a interpretar `main/CMakeLists.txt` como se fosse o `CMakeLists.txt` principal.

Isso produziu erros como:

```text
No project() command is present
No cmake_minimum_required command is present
```

e até erros secundários relacionados ao compilador.

### Forma correta

Sempre executar:

```text
dht22_alerta_climatico> idf.py build
```

Nunca:

```text
dht22_alerta_climatico\main> idf.py build
```

### Lição

O `idf.py` deve ser executado na raiz do projeto ESP-IDF.

---

## 7. Configuração antiga da tabela de partições

O `sdkconfig` copiado do Hello World ainda indicava uma tabela personalizada:

```text
partitions.csv
```

mas esse arquivo não existia no novo projeto.

### Solução

Executar:

```powershell
idf.py menuconfig
```

e selecionar:

```text
Partition Table
    → Single factory app, no OTA
```

### Lição

Ao reutilizar um `sdkconfig`, verificar especialmente:

- target;
- tabela de partições;
- flash;
- PSRAM;
- configurações específicas do projeto anterior.

---

# Problemas relacionados ao DHT22

## 8. GPIO incorreto

A ligação inicial do sensor passou por diferentes GPIOs.

A configuração que funcionou foi:

```text
DHT22 SDA → GPIO17
DHT22 VCC → 3V3
DHT22 GND → GND
```

No firmware:

```cpp
#define DHT_GPIO GPIO_NUM_17
```

### Lição

O mesmo GPIO deve aparecer de forma consistente em:

```text
diagram.json
main.cc
```

---

## 9. Necessidade de resistor pull-up

O DHT22 utiliza uma linha digital bidirecional.

A comunicação só se tornou confiável após inserir um resistor pull-up de:

```text
10 kΩ
```

entre 3V3 e a linha SDA.

Representação simplificada:

```text
3V3
 |
10k
 |
 +------ SDA DHT22
 |
GPIO17
```

### Lição

Não tratar a linha de dados do DHT22 como um sinal digital comum.

Ela depende de uma linha que possa ser liberada e assumida pelo sensor.

---

## 10. Driver DHT não funcionou corretamente no Wokwi

Inicialmente foi utilizado um componente externo para realizar:

```cpp
dht_read_float_data(...)
```

No Wokwi, a execução chegava antes da função, mas não retornava.

O diagnóstico mostrou:

```text
ANTES DA LEITURA
```

mas nunca:

```text
DEPOIS DA LEITURA
```

Portanto o firmware estava bloqueando dentro da função de leitura.

### Solução

Foi implementado diretamente o protocolo do DHT22 no `main.cc`.

Foram utilizadas funções para:

- esperar nível LOW/HIGH;
- medir duração dos pulsos;
- receber os 40 bits;
- validar checksum;
- reconstruir temperatura;
- reconstruir umidade.

### Lição

Uma biblioteca que funciona no hardware real não necessariamente se comportará da mesma forma no simulador.

---

## 11. Diagnóstico por códigos de erro

Inicialmente o programa apenas mostrava:

```text
Falha na leitura do DHT22
```

Isso não dizia em que ponto a comunicação estava falhando.

A leitura foi alterada para retornar códigos como:

```text
1      falha esperando resposta LOW
2      falha esperando resposta HIGH
3      falha na próxima transição LOW
10-49  falha durante leitura dos bits
60-99  falha medindo o pulso
100    checksum incorreto
```

Foi observado:

```text
Falha DHT22 - codigo: 1
```

Isso mostrou que o sensor nem estava assumindo corretamente a linha de dados.

### Lição

Em sistemas embarcados, diagnosticar cada fase do protocolo é muito mais eficiente do que retornar apenas verdadeiro/falso.

---

## 12. GPIO precisou operar em open-drain

O funcionamento correto ocorreu configurando GPIO17 como:

```cpp
GPIO_MODE_INPUT_OUTPUT_OD
```

com pull-up.

O ESP32 faz:

```text
LOW
↓
mantém a linha baixa

HIGH
↓
libera a linha

DHT22
↓
assume a linha e envia sua resposta
```

Isso finalmente permitiu ler corretamente os valores alterados nos sliders do Wokwi.

---

# TensorFlow Lite Micro

## 13. Modelo antigo do Hello World não deveria ser reutilizado

O `model.cc` do exemplo Hello World correspondia a outro modelo.

Para este projeto foi realizado:

```text
novo dataset
↓
novo treinamento
↓
novo modelo Float32
↓
quantização INT8
↓
modelo_climatico_int8.tflite
↓
novo model.cc
```

### Lição

`model.cc` não é código genérico do TensorFlow Lite.

Ele contém os bytes de um modelo específico.

Sempre que o modelo muda, `model.cc` precisa ser regenerado.

---

## 14. `xxd` não estava instalado no Windows

Foi tentado:

```powershell
xxd -i modelo_climatico_int8.tflite > model.cc
```

mas o Windows informou que `xxd` não era reconhecido.

### Solução

O próprio Colab foi utilizado para ler o `.tflite` e gerar:

```text
model.cc
model.h
```

### Lição

Não é necessário instalar `xxd` somente para essa conversão. Python consegue gerar diretamente o array C.

---

## 15. Header `tensorflow/lite/version.h` inexistente

Na integração com:

```text
espressif/esp-tflite-micro 1.4.1
```

o build apresentou:

```text
fatal error:
tensorflow/lite/version.h:
No such file or directory
```

### Solução

Remover:

```cpp
#include "tensorflow/lite/version.h"
```

e a verificação que dependia de:

```cpp
TFLITE_SCHEMA_VERSION
```

### Lição

A API e a organização dos headers podem mudar entre versões do TensorFlow Lite Micro.

Não assumir que um exemplo escrito para outra versão terá exatamente os mesmos headers.

---

# Checklist para o próximo projeto TinyML

Antes de iniciar:

- [ ] Criar projeto ESP-IDF limpo.
- [ ] Não copiar `build/`.
- [ ] Não copiar `managed_components/`.
- [ ] Não reutilizar `model.cc` de outro modelo.
- [ ] Conferir o `CMakeLists.txt` principal.
- [ ] Conferir `main/CMakeLists.txt`.
- [ ] Conferir o target (`esp32s3`).
- [ ] Conferir tabela de partições.
- [ ] Usar caminhos sem espaços sempre que possível.
- [ ] Conferir qual `idf.py` está sendo executado.
- [ ] Desabilitar extensões que alterem arquivos automaticamente.
- [ ] Executar `idf.py build` somente na raiz.
- [ ] Limpar `build/` após mudanças estruturais.
- [ ] Confirmar GPIO no código e no `diagram.json`.
- [ ] Verificar necessidade de pull-up do sensor.
- [ ] Testar primeiro somente o sensor.
- [ ] Depois integrar o modelo.
- [ ] Validar Float32 no Colab.
- [ ] Validar INT8 no Colab.
- [ ] Somente depois embarcar o `.tflite`.

---

# Estratégia recomendada para futuros projetos

A ordem que mostrou ser mais segura é:

```text
1. ESP-IDF compila projeto mínimo
       ↓
2. Wokwi inicia ESP32
       ↓
3. Sensor funciona sozinho
       ↓
4. Dataset é validado
       ↓
5. Modelo Float32 é treinado
       ↓
6. Modelo Float32 é avaliado
       ↓
7. Modelo é quantizado
       ↓
8. Modelo INT8 é avaliado no Colab
       ↓
9. .tflite é convertido para model.cc
       ↓
10. TFLite Micro é integrado
       ↓
11. Sensor alimenta o modelo
       ↓
12. Inferência é validada no Wokwi
```

Essa separação é importante porque impede que um problema no sensor seja confundido com um problema no modelo, ou que um problema no TensorFlow Lite seja confundido com um problema no ESP-IDF.

---

# Resultado final

O pipeline final implementado foi:

```text
DHT22
→ ESP32-S3
→ leitura de temperatura e umidade
→ StandardScaler
→ conversão da entrada para INT8
→ inferência com TensorFlow Lite Micro
→ classificação
→ ADEQUADO / ATENCAO / ALERTA
```

O projeto também mostrou, na prática, que o trabalho em TinyML envolve muito mais do que apenas treinar e quantizar um modelo: ambiente, toolchain, gerenciamento de componentes, integração de sensor, pré-processamento, compatibilidade de bibliotecas e validação embarcada são parte essencial do processo.
