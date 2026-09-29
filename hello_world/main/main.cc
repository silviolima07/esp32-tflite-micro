#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "main_functions.h"

extern "C" void app_main(void) {

  printf("\n\n*** APP_MAIN INICIOU ***\n");
  fflush(stdout);

  vTaskDelay(pdMS_TO_TICKS(1000));

  printf("*** INICIANDO TENSORFLOW LITE MICRO ***\n");
  fflush(stdout);

  setup();

  printf("*** SETUP TFLITE CONCLUIDO ***\n");
  fflush(stdout);

  while (true) {

    printf("*** EXECUTANDO INFERENCIA ***\n");
    fflush(stdout);

    loop();

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}