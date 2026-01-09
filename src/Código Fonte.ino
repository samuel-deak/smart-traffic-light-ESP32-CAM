/* Edge Impulse Arduino examples
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


// These sketches are tested with 2.0.4 ESP32 Arduino Core
// https://github.com/espressif/arduino-esp32/releases/tag/2.0.4


/* Includes ---------------------------------------------------------------- */
#include <Sem_foro_Inteligente_inferencing.h> // Substitua pelo nome da sua biblioteca
#include "edge-impulse-sdk/dsp/image/image.hpp"


#include "esp_camera.h"


// Select camera model - find more camera models in camera_pins.h file here
// https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Camera/CameraWebServer/camera_pins.h


//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM


#if defined(CAMERA_MODEL_ESP_EYE)
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    4
#define SIOD_GPIO_NUM    18
#define SIOC_GPIO_NUM    23


#define Y9_GPIO_NUM      36
#define Y8_GPIO_NUM      37
#define Y7_GPIO_NUM      38
#define Y6_GPIO_NUM      39
#define Y5_GPIO_NUM      35
#define Y4_GPIO_NUM      14
#define Y3_GPIO_NUM      13
#define Y2_GPIO_NUM      34
#define VSYNC_GPIO_NUM   5
#define HREF_GPIO_NUM    27
#define PCLK_GPIO_NUM    25


#elif defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27


#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22


#else
#error "Camera model not selected"
#endif


/* Constant defines -------------------------------------------------------- */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS      320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS      240
#define EI_CAMERA_FRAME_BYTE_SIZE            3



// --- Pinos do Semáforo 1 (Avenida Principal) ---
const int S1_VERDE_PIN   = 2; // GPIO 2
const int S1_AMARELO_PIN = 4; // GPIO 4 (Pino do flash)
const int S1_VERMELHO_PIN = 14; // GPIO 14


// --- Pinos do Semáforo 2 (Rua Secundária) ---
const int S2_VERDE_PIN   = 12; // GPIO 12
const int S2_AMARELO_PIN = 13;  // GPIO 13 
const int S2_VERMELHO_PIN = 15;  // GPIO 15


// ==========================================================
// Variáveis da Máquina de Estados do Semáforo
// ==========================================================


// Define os estados possíveis do semáforo
enum EstadoSemaforo {
  PRINCIPAL_VERDE,      // S1 Verde, S2 Vermelho (Estado Padrão)
  PRINCIPAL_AMARELO,   // S1 Amarelo, S2 Vermelho
  TUDO_VERMELHO_1,    // S1 Vermelho, S2 Vermelho (Pausa de segurança)
  SECUNDARIA_VERDE,   // S1 Vermelho, S2 Verde
  SECUNDARIA_AMARELO, // S1 Vermelho, S2 Amarelo
  TUDO_VERMELHO_2     // S1 Vermelho, S2 Vermelho (Pausa de segurança)
};


// Variável para guardar o estado atual
EstadoSemaforo estadoAtual = PRINCIPAL_VERDE;


// Variável para guardar o tempo da última mudança de estado
unsigned long tempoTrocaEstado = 0;


// Constantes de tempo (em milissegundos)
const long TEMPO_AMARELO = 3000;         // 3 segundos
const long TEMPO_TUDO_VERMELHO = 2000;   // 2 segundos
const long TEMPO_VERDE_SECUNDARIA = 10000; // 10 segundos (tempo que a rua X fica aberta)


// Limiar de confiança para considerar "carro"
const float LIMIAR_DETECCAO = 0.70; // 70% de confiança


// Variável de "gatilho" que a IA irá ativar
bool carro_detectado_rua_x = false;
bool transicao_iniciada = false; // Flag para não disparar o gatilho múltiplas vezes



/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;
uint8_t *snapshot_buf; //points to the output of the capture


static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,


    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,


    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,


    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,   //QQVGA-UXGA Do not use sizes above QVGA when not JPEG


    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1,      //if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};


/* Function definitions (Prototypes) --------------------------------------- */
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) ;
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
void gerenciarSemaforos();
void setLuzes(int S1, int S2);


/**
* @brief     Arduino setup function
*/
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    //comment out the below line to start inference immediately after upload
    while (!Serial);
    Serial.println("Edge Impulse Inferencing Demo");
    if (ei_camera_init() == false) {
        ei_printf("Failed to initialize Camera!\r\n");
    }
    else {
        ei_printf("Camera initialized\r\n");
    }


    // ==========================================================
    // Configuração dos Pinos dos Semáforos
    // ==========================================================
    pinMode(S1_VERDE_PIN, OUTPUT);
    pinMode(S1_AMARELO_PIN, OUTPUT);
    pinMode(S1_VERMELHO_PIN, OUTPUT);
    pinMode(S2_VERDE_PIN, OUTPUT);
    pinMode(S2_AMARELO_PIN, OUTPUT);
    pinMode(S2_VERMELHO_PIN, OUTPUT);


    // Estado inicial (S1 Verde, S2 Vermelho)
    digitalWrite(S1_VERDE_PIN, HIGH);
    digitalWrite(S1_AMARELO_PIN, LOW);
    digitalWrite(S1_VERMELHO_PIN, LOW);
    
    digitalWrite(S2_VERDE_PIN, LOW);
    digitalWrite(S2_AMARELO_PIN, LOW);
    digitalWrite(S2_VERMELHO_PIN, HIGH); // Liga o S2 Vermelho (e o flash LED)


    // Define o tempo inicial
    tempoTrocaEstado = millis();
    // ==========================================================


    ei_printf("\nStarting continious inference in 2 seconds...\n");
    ei_sleep(2000);
}


/**
* @brief       Get data and run inferencing
*
* @param[in]   debug   Get debug info if true
*/
void loop()
{
    // instead of wait_ms, we'll wait on the signal, this allows threads to cancel us...
    if (ei_sleep(5) != EI_IMPULSE_OK) {
        return;
    }


    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);


    // check if allocation was successful
    if(snapshot_buf == nullptr) {
        ei_printf("ERR: Failed to allocate snapshot buffer!\n");
        return;
    }


    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;


    if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
        ei_printf("Failed to capture image\r\n");
        free(snapshot_buf);
        return;
    }


    // Run the classifier
    ei_impulse_result_t result = { 0 };


    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        return;
    }


    // print the predictions
    ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);


#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    
    float max_carro_confianca = 0.0; // Reseta a confian�a a cada ciclo


    // IMPRESS�O ORIGINAL (Solicitada por voc�)
    ei_printf("Object detection bounding boxes:\r\n");
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value == 0) {
            continue;
        }


        // Imprime a detec��o (como no original)
        ei_printf("   %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
                bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);


        // Verifica se a label é "carro" e se a confiança é a maior até agora
        if (strcmp(bb.label, "Carro") == 0) {
            if (bb.value > max_carro_confianca) {
                max_carro_confianca = bb.value;
            }
        }
    }


    // ==========================================================
    // Lógica de Gatilho da IA para o Semáforo
    // ==========================================================
    // Se um carro for detectado com confiançaa acima do limiar
    // E a transição ainda não tiver começado...
    if (max_carro_confianca > LIMIAR_DETECCAO && !transicao_iniciada) {
        carro_detectado_rua_x = true; // ATIVA O GATILHO
        ei_printf("GATILHO: Carro detectado! Iniciando transição...\n");
    }
    // ==========================================================


#else
    ei_printf("Predictions:\r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("   %s: ", ei_classifier_inferencing_categories[i]);
        ei_printf("%.5f\r\n", result.classification[i].value);
    }
#endif


    // Print anomaly result (if it exists)
#if EI_CLASSIFIER_HAS_ANOMALY
    ei_printf("Anomaly prediction: %.3f\r\n", result.anomaly);
#endif


#if EI_CLASSIFIER_HAS_VISUAL_ANOMALY
    ei_printf("Visual anomalies:\r\n");
    for (uint32_t i = 0; i < result.visual_ad_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.visual_ad_grid_cells[i];
        if (bb.value == 0) {
            continue;
        }
        ei_printf("   %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
                bb.label,
                bb.value,
                bb.x,
                bb.y,
                bb.width,
                bb.height);
    }
#endif


    free(snapshot_buf);


    // ==========================================================
    // Chamar o Gerenciador do Semáforo a CADA CICLO
    // ==========================================================
    gerenciarSemaforos();
}


// =================================================================
// FUNÇÕES ORIGINAIS DA CÂMERA (DO CÓDIGO DO EDGE IMPULSE)
// =================================================================


/**
 * @brief   Setup image sensor & start streaming
 *
 * @retval  false if initialisation failed
 */
bool ei_camera_init(void) {


    if (is_initialised) return true;


#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif


    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x\n", err);
      return false;
    }


    sensor_t * s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) {
      s->set_vflip(s, 1); // flip it back
      s->set_brightness(s, 1); // up the brightness just a bit
      s->set_saturation(s, 0); // lower the saturation
    }


#if defined(CAMERA_MODEL_M5STACK_WIDE)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
#elif defined(CAMERA_MODEL_ESP_EYE)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    s->set_awb_gain(s, 1);
#endif


    is_initialised = true;
    return true;
}


/**
 * @brief     Stop streaming of sensor data
 */
void ei_camera_deinit(void) {


    //deinitialize the camera
    esp_err_t err = esp_camera_deinit();


    if (err != ESP_OK)
    {
        ei_printf("Camera deinit failed\n");
        return;
    }


    is_initialised = false;
    return;
}



/**
 * @brief     Capture, rescale and crop image
 *
 * @param[in] img_width     width of output image
 * @param[in] img_height    height of output image
 * @param[in] out_buf       pointer to store output image, NULL may be used
 * if ei_camera_frame_buffer is to be used for capture and resize/cropping.
 *
 * @retval    false if not initialised, image captured, rescaled or cropped failed
 *
 */
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    bool do_resize = false;


    if (!is_initialised) {
        ei_printf("ERR: Camera is not initialized\r\n");
        return false;
    }


    camera_fb_t *fb = esp_camera_fb_get();


    if (!fb) {
        ei_printf("Camera capture failed\n");
        return false;
    }


   bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);


   esp_camera_fb_return(fb);


   if(!converted){
       ei_printf("Conversion failed\n");
       return false;
   }


    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
        || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
        do_resize = true;
    }


    if (do_resize) {
        ei::image::processing::crop_and_interpolate_rgb888(
        out_buf,
        EI_CAMERA_RAW_FRAME_BUFFER_COLS,
        EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
        out_buf,
        img_width,
        img_height);
    }



    return true;
}


static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
    // we already have a RGB888 buffer, so recalculate offset into pixel index
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;


    while (pixels_left != 0) {
        // Swap BGR to RGB here
        // due to https://github.com/espressif/esp32-camera/issues/379
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];


        // go to the next pixel
        out_ptr_ix++;
        pixel_ix+=3;
        pixels_left--;
    }
    // and done!
    return 0;
}


#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif


// ==========================================================
// NOVAS FUN��ES DO SEM�FORO
// ==========================================================


/**
 * @brief Função Auxiliar para Acender os LEDs
 * (S1 = Semáforo 1, S2 = Semáforo 2)
 * (1 = Verde, 2 = Amarelo, 3 = Vermelho)
 */
void setLuzes(int S1, int S2) {
    // --- Controle Semáforo 1 (Principal) ---
    digitalWrite(S1_VERDE_PIN,   (S1 == 1)); // HIGH se S1 == 1, senão LOW
    digitalWrite(S1_AMARELO_PIN, (S1 == 2)); // HIGH se S1 == 2, senão LOW
    digitalWrite(S1_VERMELHO_PIN, (S1 == 3)); // HIGH se S1 == 3, senão LOW


    // --- Controle Semáforo 2 (Secundária) ---
    digitalWrite(S2_VERDE_PIN,   (S2 == 1)); // HIGH se S2 == 1, senão LOW
    digitalWrite(S2_AMARELO_PIN, (S2 == 2)); // HIGH se S2 == 2, senão LOW
    digitalWrite(S2_VERMELHO_PIN, (S2 == 3)); // HIGH se S2 == 3, senão LOW
}


/**
 * @brief Função Principal da Máquina de Estados (Gerenciador)
 * Esta função é chamada a todo momento pelo loop()
 */
void gerenciarSemaforos() {
    
    // Calcula quanto tempo passou desde a última troca
    unsigned long tempoDecorrido = millis() - tempoTrocaEstado;


    // Use um "switch" para verificar o estado atual
    switch (estadoAtual) {
      
        case PRINCIPAL_VERDE:
            // Luzes: S1 Verde, S2 Vermelho
            setLuzes(1, 3);
            
            // CONDIÇÃO DE MUDANÇA:
            // Se o gatilho (IA) foi ativado E a transição não começou...
            if (carro_detectado_rua_x && !transicao_iniciada) {
                transicao_iniciada = true; // Marca que a transição começou
                estadoAtual = PRINCIPAL_AMARELO; // Muda para o próximo estado
                tempoTrocaEstado = millis();    // Reinicia o cronômetro
                ei_printf("Estado: PRINCIPAL_AMARELO\n");
            }
            break; // Fim do caso PRINCIPAL_VERDE


        case PRINCIPAL_AMARELO:
            // Luzes: S1 Amarelo, S2 Vermelho
            setLuzes(2, 3);
            
            // CONDIÇÃO DE MUDANÇA:
            // Se o tempo de amarelo já passou...
            if (tempoDecorrido >= TEMPO_AMARELO) {
                estadoAtual = TUDO_VERMELHO_1; // Muda para o próximo estado
                tempoTrocaEstado = millis();     // Reinicia o cronômetro
                ei_printf("Estado: TUDO_VERMELHO_1\n");
            }
            break; // Fim do caso PRINCIPAL_AMARELO


        case TUDO_VERMELHO_1:
            // Luzes: S1 Vermelho, S2 Vermelho
            setLuzes(3, 3);
            
            // CONDIÇÃO DE MUDANÇA:
            // Se o tempo de "tudo vermelho" já passou...
            if (tempoDecorrido >= TEMPO_TUDO_VERMELHO) {
                estadoAtual = SECUNDARIA_VERDE; // Muda para o próximo estado
                tempoTrocaEstado = millis();      // Reinicia o cronômetro
                ei_printf("Estado: SECUNDARIA_VERDE\n");
            }
            break; // Fim do caso TUDO_VERMELHO_1


        case SECUNDARIA_VERDE:
            // Luzes: S1 Vermelho, S2 Verde
            setLuzes(3, 1);
            
            // CONDIÇÃO DE MUDANÇAA:
            // Se o tempo que a Rua X fica aberta já passou...
            if (tempoDecorrido >= TEMPO_VERDE_SECUNDARIA) {
                estadoAtual = SECUNDARIA_AMARELO; // Muda para o próximo estado
                tempoTrocaEstado = millis();       // Reinicia o cronômetro
                ei_printf("Estado: SECUNDARIA_AMARELO\n");
            }
            break; // Fim do caso SECUNDARIA_VERDE


        case SECUNDARIA_AMARELO:
            // Luzes: S1 Vermelho, S2 Amarelo
            setLuzes(3, 2);
            
            // CONDIÇÃO DE MUDANÇAA:
            // Se o tempo de amarelo já passou...
            if (tempoDecorrido >= TEMPO_AMARELO) {
                estadoAtual = TUDO_VERMELHO_2; // Muda para o pr�ximo estado
                tempoTrocaEstado = millis();     // Reinicia o cron�metro
                ei_printf("Estado: TUDO_VERMELHO_2\n");
            }
            break; // Fim do caso SECUNDARIA_AMARELO


        case TUDO_VERMELHO_2:
            // Luzes: S1 Vermelho, S2 Vermelho
            setLuzes(3, 3);
            
            // CONDIÇÃO DE MUDANÇA:
            // Se o tempo de "tudo vermelho" já passou...
            if (tempoDecorrido >= TEMPO_TUDO_VERMELHO) {
                estadoAtual = PRINCIPAL_VERDE; // VOLTA AO ESTADO INICIAL
                tempoTrocaEstado = millis();     // Reinicia o cronômetro
                
                // Reinicia os gatilhos para o próximo ciclo
                carro_detectado_rua_x = false; 
                transicao_iniciada = false;
                ei_printf("Estado: PRINCIPAL_VERDE (Ciclo completo)\n");
            }
            break; // Fim do caso TUDO_VERMELHO_2
    }
}