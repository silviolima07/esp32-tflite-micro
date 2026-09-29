# Template para novos projetos TinyML

## Estrutura sugerida

```text
meu_projeto/
├── main/
│   ├── main.cc
│   ├── main_functions.cc
│   ├── main_functions.h
│   ├── model.cc
│   └── model.h
├── models/
│   ├── modelo_float.tflite
│   └── modelo_int8.tflite
├── components/
│   ├── esp-tflite-micro/
│   └── esp-nn/
├── docs/
├── diagram.json
├── wokwi.toml
└── README.md
```

## Fluxo padrão

```text
Dataset
   ↓
Treinamento FLOAT32
   ↓
Quantização INT8
   ↓
Validação no host
   ↓
modelo_int8.tflite
   ↓
model.cc
   ↓
TFLite Micro
   ↓
ESP32-S3
```

## Checklist

- [ ] Modelo validado no host.
- [ ] `model.cc` corresponde ao `.tflite`.
- [ ] `dtype` de entrada e saída conferidos.
- [ ] `scale` e `zero_point` conhecidos.
- [ ] Entrada quantizada antes de `Invoke()`.
- [ ] Saída desquantizada corretamente.
- [ ] Pelo menos cinco entradas comparadas com a referência.
- [ ] ANSI C usado como baseline.
- [ ] ESP-NN otimizado testado somente depois da baseline.
- [ ] Problemas e decisões registrados no README.
