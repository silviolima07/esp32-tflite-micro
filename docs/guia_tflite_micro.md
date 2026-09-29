# Guia ESP32-S3 + TensorFlow Lite Micro + Wokwi

## Objetivo

Registrar o processo completo usado para colocar o Hello World do TensorFlow Lite Micro em funcionamento no ESP32-S3 com ESP-IDF 6.1 e Wokwi.

## Fluxo validado

```text
Modelo FLOAT32 no Colab
        ↓
Quantização INT8
        ↓
Validação do .tflite
        ↓
Conversão para model.cc
        ↓
TFLite Micro no ESP32-S3
        ↓
Entrada FLOAT → INT8
        ↓
Inferência
        ↓
Saída INT8 → FLOAT
```

## Problema 1 — model.cc

A saída ficou constante mesmo com a entrada variando.

A correção foi gerar um novo `model.cc` a partir do `hello_world_int8.tflite` validado no Colab.

## Problema 2 — ESP-NN otimizado

Mesmo com o modelo correto, os resultados do ESP32 divergiam do Colab.

A configuração do ESP-NN foi alterada de:

```text
Optimized versions
```

para:

```text
ANSI C
```

Após a mudança, os resultados ficaram muito próximos dos valores de referência.

## Validação recomendada

1. Validar o `.tflite` no Colab.
2. Gerar `model.cc`.
3. Comparar tamanho e SHA-256.
4. Conferir `dtype`, `scale` e `zero_point`.
5. Imprimir `x`, `x_q`, `y_q` e `y`.
6. Comparar resultados Colab x ESP32.
7. Testar ANSI C antes de validar kernels otimizados.

## Wokwi

```toml
[wokwi]
version = 1
firmware = "build/flasher_args.json"
elf = "build/hello_world.elf"
```

## Conclusão

Foram identificados dois problemas independentes:

1. o `model.cc` inicialmente utilizado não correspondia adequadamente ao modelo INT8 usado como referência;
2. a implementação otimizada do ESP-NN produzia resultados numericamente inadequados para este modelo.

A configuração final validada foi:

```text
modelo INT8 validado
+
model.cc correspondente
+
ESP-NN em ANSI C
```
