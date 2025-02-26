#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include "hardware/clocks.h"

#include "inc/ssd1306.h"
#include "inc/font.h"
#include "build/pio_matrix.pio.h" 

#define I2C_PORT   i2c1
#define I2C_SDA    14
#define I2C_SCL    15
#define OLED_ADDR  0x3C
#define WIDTH      128
#define HEIGHT     64

#define MIC_PIN    28   // GPIO28 => canal ADC2
#define N_SAMPLES  100  

static ssd1306_t ssd;

/**
 * Exibe texto no display.
 */
void atualiza_display_com_informacoes(const char *texto) {
    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
    ssd1306_draw_string(&ssd, texto, 10, 10);
    ssd1306_send_data(&ssd);
}

int main() {
    stdio_init_all();

    // Inicializa I2C e display
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // ADC p/ microfone
    adc_init();
    adc_gpio_init(MIC_PIN);
    atualiza_display_com_informacoes("Iniciando...");
    sleep_ms(1000);

    while (true) {
        // Seleciona canal do ADC (GPIO28 => ADC2)
        adc_select_input(2);

        // PEAK-TO-PEAK
        uint16_t minVal = 0xFFFF;
        uint16_t maxVal = 0;

        // Coletar N_SAMPLES
        for(int i=0; i<N_SAMPLES; i++){
            uint16_t raw = adc_read(); // 0..4095
            if(raw < minVal) minVal = raw;
            if(raw > maxVal) maxVal = raw;
        }

        // amplitude 0..1
        uint16_t peak2peak = maxVal - minVal;
        float amplitude = (float)peak2peak / 4095.0f;

        // dB = 20 * log10(amplitude)
        float db_val = 20.0f * log10f(amplitude + 1e-6f);

        // arredonda p/ inteiro
        int db_int = (int)roundf(db_val);

        // Exibe
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d dB", db_int);
        atualiza_display_com_informacoes(buffer);

        // Depuração no console
        printf("min=%u max=%u => p2p=%u amp=%.4f dB=%.2f => %d\n",
               minVal, maxVal, peak2peak, amplitude, db_val, db_int);

        sleep_ms(200);
    }

    return 0;
}
