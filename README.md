# ESP32-S3 + TensorFlow Lite Micro + Wokwi — Hello World INT8

Projeto da **Atividade Avaliativa Prática 4/6**, reproduzindo o exemplo *Hello World* do TensorFlow Lite Micro no ESP32-S3 com ESP-IDF e Wokwi.

## Objetivo

Executar um modelo quantizado INT8 no ESP32-S3 e comparar sua inferência com o mesmo modelo previamente validado no Google Colab.

```text
Treinamento FLOAT32 no Colab
        ↓
Quantização INT8
        ↓
hello_world_int8.tflite
        ↓
Conversão para model.cc
        ↓
TensorFlow Lite Micro
        ↓
ESP32-S3 / Wokwi
        ↓
Inferência INT8
        ↓
Desquantização da saída
```

## Principais problemas encontrados

### 1. `model.cc` original produzia saída inadequada

Nos primeiros testes, a entrada quantizada variava, mas a saída permanecia praticamente constante.

```text
x_q variando
y_q = -128
y ≈ -1.118
```

Para eliminar a dúvida sobre qual modelo estava realmente embarcado, o `model.cc` original foi substituído por um novo arquivo gerado a partir do:

```text
hello_world_int8.tflite
```

Esse modelo INT8 foi treinado/quantizado e validado previamente no Colab.

O `.tflite` e os bytes presentes no novo `model.cc` foram comparados por tamanho e SHA-256 para confirmar que o firmware estava usando exatamente o mesmo modelo validado.

### 2. Resultado do ESP32 diferente do Colab

Mesmo com o `model.cc` correto, ainda havia divergências importantes entre o Colab e o ESP32-S3.

Exemplo para `x = π/2`:

```text
Colab
x_q = -64
y_q = 127
y ≈ 1.035

ESP-NN otimizado
x_q = -64
y_q = -99
y ≈ -0.765
```

Como a entrada quantizada era a mesma, a divergência não estava na quantização de `x`.

## Solução: trocar ESP-NN otimizado por ANSI C

No `menuconfig`, a configuração:

```text
ESP-NN Optimization for nn functions
```

foi alterada de:

```text
Optimized versions
```

para:

```text
ANSI C
```

O modelo não foi alterado nessa etapa.

O mesmo `model.cc`, os mesmos pesos e os mesmos parâmetros de quantização continuaram sendo utilizados. Mudou apenas a implementação usada para executar as operações da rede.

Depois da alteração para ANSI C, os resultados do ESP32-S3 ficaram muito próximos dos valores obtidos no Colab.

## Lógica de inferência INT8

```text
x FLOAT
   ↓
quantização usando scale e zero_point
   ↓
x_q INT8
   ↓
interpreter->Invoke()
   ↓
y_q INT8
   ↓
desquantização
   ↓
y FLOAT
```

Trecho simplificado:

```cpp
int32_t x_quantized_temp =
    static_cast<int32_t>(
        x / input->params.scale +
        input->params.zero_point
    );

if (x_quantized_temp < -128) x_quantized_temp = -128;
if (x_quantized_temp > 127)  x_quantized_temp = 127;

input->data.int8[0] =
    static_cast<int8_t>(x_quantized_temp);

interpreter->Invoke();

int8_t y_quantized = output->data.int8[0];

float y =
    (static_cast<int32_t>(y_quantized)
     - output->params.zero_point)
    * output->params.scale;
```

## Resultado final

```text
Modelo FLOAT32
        ↓
Quantização INT8
        ↓
Validação no Colab
        ↓
hello_world_int8.tflite
        ↓
model.cc
        ↓
TensorFlow Lite Micro
        ↓
ESP-NN em ANSI C
        ↓
ESP32-S3 / Wokwi
        ↓
Resultado próximo ao Colab
```

## Principais aprendizados

- O `model.cc` precisa corresponder exatamente ao `.tflite` validado.
- Quantizar a entrada não é quantizar o modelo.
- `scale` e `zero_point` fazem parte do contrato do modelo INT8.
- O modelo deve ser validado no host antes de ser embarcado.
- Imprimir `x`, `x_q`, `y_q` e `y` ajuda a localizar divergências.
- Se o mesmo `x_q` produz `y_q` muito diferente em duas plataformas, a investigação deve avançar para kernels e bibliotecas de execução.
- Otimizações de hardware precisam ser validadas numericamente.
- Neste experimento, a implementação ANSI C reproduziu melhor o comportamento do Colab.

## Documentação

- [`docs/guia_tflite_micro.md`](docs/guia_tflite_micro.md)
- [`docs/template_novos_projetos.md`](docs/template_novos_projetos.md)
