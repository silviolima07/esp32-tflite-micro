#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

#include "model.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

// ============================================================
// DHT22
// ============================================================

#define DHT_GPIO GPIO_NUM_17


// ============================================================
// STANDARD SCALER
//
// SUBSTITUA pelos valores obtidos no Colab:
//
// print(scaler.mean_)
// print(scaler.scale_)
// ============================================================

constexpr float TEMP_MEAN = 29.51f;   
constexpr float TEMP_STD  = 3.11;   

constexpr float HUM_MEAN  = 71.20f;   
constexpr float HUM_STD   = 14.85f;  


// ============================================================
// TENSOR ARENA
// ============================================================

constexpr int TENSOR_ARENA_SIZE = 12 * 1024;

alignas(16) static uint8_t tensor_arena[TENSOR_ARENA_SIZE];


// ============================================================
// DHT22 - FUNCOES AUXILIARES
// ============================================================

static int wait_for_level(int level, int timeout_us)
{
    int64_t start = esp_timer_get_time();

    while (gpio_get_level(DHT_GPIO) != level)
    {
        if ((esp_timer_get_time() - start) > timeout_us)
        {
            return -1;
        }
    }

    return 0;
}


static int measure_level(int level, int timeout_us)
{
    int64_t start = esp_timer_get_time();

    while (gpio_get_level(DHT_GPIO) == level)
    {
        if ((esp_timer_get_time() - start) > timeout_us)
        {
            return -1;
        }
    }

    return (int)(esp_timer_get_time() - start);
}


// ============================================================
// INICIALIZACAO DHT22
// ============================================================

static void dht22_init()
{
    gpio_config_t io_conf = {};

    io_conf.pin_bit_mask = (1ULL << DHT_GPIO);
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
}


// ============================================================
// LEITURA DHT22
// ============================================================

static int dht22_read(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};

    // sinal inicial
    gpio_set_level(DHT_GPIO, 0);

    esp_rom_delay_us(2000);

    // libera linha
    gpio_set_level(DHT_GPIO, 1);

    esp_rom_delay_us(30);

    // resposta do DHT22
    if (wait_for_level(0, 200) < 0)
        return 1;

    if (wait_for_level(1, 200) < 0)
        return 2;

    if (wait_for_level(0, 200) < 0)
        return 3;


    // leitura dos 40 bits
    for (int i = 0; i < 40; i++)
    {
        if (wait_for_level(1, 150) < 0)
        {
            return 10 + i;
        }

        int high_time = measure_level(1, 150);

        if (high_time < 0)
        {
            return 60 + i;
        }

        data[i / 8] <<= 1;

        if (high_time > 50)
        {
            data[i / 8] |= 1;
        }
    }


    // checksum
    uint8_t checksum =
        data[0] +
        data[1] +
        data[2] +
        data[3];

    if (checksum != data[4])
    {
        return 100;
    }


    // umidade
    uint16_t raw_humidity =
        ((uint16_t)data[0] << 8) |
        data[1];


    // temperatura
    uint16_t raw_temperature =
        ((uint16_t)(data[2] & 0x7F) << 8) |
        data[3];


    *humidity =
        raw_humidity / 10.0f;

    *temperature =
        raw_temperature / 10.0f;


    if (data[2] & 0x80)
    {
        *temperature *= -1.0f;
    }


    return 0;
}


// ============================================================
// LIMITA INT8
// ============================================================

static int8_t quantize_int8(
    float value,
    float scale,
    int zero_point
)
{
    int32_t quantized =
        (int32_t)roundf(value / scale)
        + zero_point;


    if (quantized > 127)
        quantized = 127;

    if (quantized < -128)
        quantized = -128;


    return (int8_t)quantized;
}


// ============================================================
// APP MAIN
// ============================================================

extern "C" void app_main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DHT22 + TFLITE MICRO - ALERTA CLIMATICO\n");
    printf("========================================\n\n");


    // --------------------------------------------------------
    // inicializa DHT22
    // --------------------------------------------------------

    dht22_init();


    // --------------------------------------------------------
    // carrega modelo TFLite
    // --------------------------------------------------------

    const tflite::Model *model =
        tflite::GetModel(
            modelo_climatico_int8_tflite
        );



    // --------------------------------------------------------
    // operadores usados pela rede
    //
    // Dense -> FullyConnected
    // Softmax -> Softmax
    // --------------------------------------------------------

    static tflite::MicroMutableOpResolver<2> resolver;

    resolver.AddFullyConnected();
    resolver.AddSoftmax();


    // --------------------------------------------------------
    // cria interpreter
    // --------------------------------------------------------

    static tflite::MicroInterpreter interpreter(
        model,
        resolver,
        tensor_arena,
        TENSOR_ARENA_SIZE
    );


    // --------------------------------------------------------
    // aloca tensores
    // --------------------------------------------------------

    if (interpreter.AllocateTensors() != kTfLiteOk)
    {
        printf("Erro ao alocar tensores.\n");
        return;
    }


    TfLiteTensor *input =
        interpreter.input(0);

    TfLiteTensor *output =
        interpreter.output(0);


    printf("Modelo carregado.\n");

    printf(
        "Modelo: %u bytes\n",
        modelo_climatico_int8_tflite_len
    );


    // --------------------------------------------------------
    // mostra parametros INT8 do proprio modelo
    // --------------------------------------------------------

    float input_scale =
        input->params.scale;

    int input_zero_point =
        input->params.zero_point;


    float output_scale =
        output->params.scale;

    int output_zero_point =
        output->params.zero_point;


    printf("\nPARAMETROS DE QUANTIZACAO\n");

    printf(
        "Input scale      : %.8f\n",
        input_scale
    );

    printf(
        "Input zero point : %d\n",
        input_zero_point
    );

    printf(
        "Output scale     : %.8f\n",
        output_scale
    );

    printf(
        "Output zero point: %d\n\n",
        output_zero_point
    );


    const char *classes[] =
    {
        "ADEQUADO",
        "ATENCAO",
        "ALERTA"
    };


    // ========================================================
    // LOOP
    // ========================================================

    while (true)
    {
        float temperature = 0.0f;
        float humidity = 0.0f;


        int status =
            dht22_read(
                &temperature,
                &humidity
            );


        if (status != 0)
        {
            printf(
                "Falha DHT22 - codigo: %d\n",
                status
            );

            vTaskDelay(
                pdMS_TO_TICKS(2000)
            );

            continue;
        }


        // ----------------------------------------------------
        // valores originais
        // ----------------------------------------------------

        printf("\n------------------------------\n");

        printf(
            "Temperatura: %.1f C\n",
            temperature
        );

        printf(
            "Umidade    : %.1f %%\n",
            humidity
        );


        // ----------------------------------------------------
        // STANDARD SCALER
        //
        // mesma transformacao usada no Colab
        // ----------------------------------------------------

        float temp_scaled =
            (temperature - TEMP_MEAN)
            / TEMP_STD;


        float hum_scaled =
            (humidity - HUM_MEAN)
            / HUM_STD;


        printf(
            "Normalizado: temp=%.4f hum=%.4f\n",
            temp_scaled,
            hum_scaled
        );


        // ----------------------------------------------------
        // FLOAT -> INT8
        // ----------------------------------------------------

        int8_t temp_int8 =
            quantize_int8(
                temp_scaled,
                input_scale,
                input_zero_point
            );


        int8_t hum_int8 =
            quantize_int8(
                hum_scaled,
                input_scale,
                input_zero_point
            );


        input->data.int8[0] =
            temp_int8;

        input->data.int8[1] =
            hum_int8;


        printf(
            "Entrada INT8: [%d, %d]\n",
            temp_int8,
            hum_int8
        );


        // ----------------------------------------------------
        // INFERENCIA
        // ----------------------------------------------------

        if (interpreter.Invoke() != kTfLiteOk)
        {
            printf(
                "Erro durante inferencia.\n"
            );

            vTaskDelay(
                pdMS_TO_TICKS(2000)
            );

            continue;
        }


        // ----------------------------------------------------
        // resultados INT8
        // ----------------------------------------------------

        int8_t raw0 =
            output->data.int8[0];

        int8_t raw1 =
            output->data.int8[1];

        int8_t raw2 =
            output->data.int8[2];


        // ----------------------------------------------------
        // dequantizacao das probabilidades
        // ----------------------------------------------------

        float prob0 =
            (raw0 - output_zero_point)
            * output_scale;

        float prob1 =
            (raw1 - output_zero_point)
            * output_scale;

        float prob2 =
            (raw2 - output_zero_point)
            * output_scale;


        // ----------------------------------------------------
        // ARGMAX
        // ----------------------------------------------------

        int classe = 0;

        int8_t maior = raw0;


        if (raw1 > maior)
        {
            maior = raw1;
            classe = 1;
        }


        if (raw2 > maior)
        {
            maior = raw2;
            classe = 2;
        }


        // ----------------------------------------------------
        // imprime resultado
        // ----------------------------------------------------

        printf("\nProbabilidades:\n");

        printf(
            "ADEQUADO : %.3f\n",
            prob0
        );

        printf(
            "ATENCAO  : %.3f\n",
            prob1
        );

        printf(
            "ALERTA   : %.3f\n",
            prob2
        );


        printf(
            "\n>>> CLASSIFICACAO: %s <<<\n",
            classes[classe]
        );


        vTaskDelay(
            pdMS_TO_TICKS(2000)
        );
    }
}