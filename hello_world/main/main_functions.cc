/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "main_functions.h"
#include "model.h"
#include "constants.h"
#include "output_handler.h"

namespace {

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;

TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

int inference_count = 0;

constexpr int kTensorArenaSize = 2000;
uint8_t tensor_arena[kTensorArenaSize];

}  // namespace


void setup() {

  MicroPrintf("Iniciando TensorFlow Lite Micro - MODELO INT8");

  model = tflite::GetModel(g_model);

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf(
        "Model provided is schema version %d not equal to supported version %d.",
        model->version(),
        TFLITE_SCHEMA_VERSION
    );
    return;
  }

  static tflite::MicroMutableOpResolver<1> resolver;

  if (resolver.AddFullyConnected() != kTfLiteOk) {
    MicroPrintf("Erro ao adicionar FullyConnected");
    return;
  }

  static tflite::MicroInterpreter static_interpreter(
      model,
      resolver,
      tensor_arena,
      kTensorArenaSize
  );

  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();

  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  MicroPrintf(
      "INPUT type=%d scale=%f zero_point=%d",
      static_cast<int>(input->type),
      static_cast<double>(input->params.scale),
      input->params.zero_point
  );

  MicroPrintf(
      "OUTPUT type=%d scale=%f zero_point=%d",
      static_cast<int>(output->type),
      static_cast<double>(output->params.scale),
      output->params.zero_point
  );

  inference_count = 0;

  MicroPrintf("Setup INT8 concluido");
}


void loop() {

  // Mesmos valores usados no Colab
  static const float test_values[] = {
      0.0f,
      1.570796f,  // pi/2
      3.141593f,  // pi
      4.712389f,  // 3*pi/2
      6.283185f   // 2*pi
  };

static const int kTestCount = 5;

float x = test_values[inference_count];

// FLOAT -> INT8
int32_t x_quantized_temp =
      static_cast<int32_t>(
          x / input->params.scale
          + input->params.zero_point
      );

// Limita ao intervalo do int8
if (x_quantized_temp < -128) {
    x_quantized_temp = -128;
  }

if (x_quantized_temp > 127) {
    x_quantized_temp = 127;
  }

int8_t x_quantized =
      static_cast<int8_t>(x_quantized_temp);

input->data.int8[0] = x_quantized;

// Inferência
TfLiteStatus invoke_status = interpreter->Invoke();

if (invoke_status != kTfLiteOk) {
    MicroPrintf(
        "Invoke failed on x: %f",
        static_cast<double>(x)
    );
    return;
  }

// Saída INT8
int8_t y_quantized =
      output->data.int8[0];

// INT8 -> FLOAT
float y =
      (static_cast<int32_t>(y_quantized)
       - output->params.zero_point)
      * output->params.scale;

// Resultado para comparação com o Colab
MicroPrintf(
      "COMPARACAO -> x=%f x_q=%d y_q=%d y=%f",
      static_cast<double>(x),
      static_cast<int>(x_quantized),
      static_cast<int>(y_quantized),
      static_cast<double>(y)
  );

// Próximo valor de teste
  inference_count += 1;

if (inference_count >= kTestCount) {
    inference_count = 0;
  }
}  