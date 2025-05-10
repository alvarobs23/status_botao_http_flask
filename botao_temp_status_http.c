#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "example_http_client_util.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"

// --- Configurações da rede e ThingSpeak ---
#define THINGSPEAK_API_KEY  "5Y283PJ0JHGWHVTS"
#define HOST                "api.thingspeak.com"
#define PORT                80
#define INTERVALO_MS        500

// --- Pinos ---
#define BUTTON_A            5
#define LED_RED             13

// Inicializa o ADC e habilita o sensor interno
void adc_setup() {
    adc_init();
    adc_set_temp_sensor_enabled(true);
}

// Lê temperatura do sensor interno (canal 4)
float read_temperature() {
    adc_select_input(4);
    uint16_t raw = adc_read();
    float voltage = raw * 3.3f / (1 << 12);
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

// Média de N amostras
double read_temperature_average(int samples) {
    double sum = 0.0;
    for (int i = 0; i < samples; i++) {
        sum += read_temperature();
        sleep_ms(50);
    }
    return sum / samples;
}

int main() {
    stdio_init_all();
    adc_setup();

    // Configura botão e LED
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    // Inicializa Wi-Fi
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar o Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                           CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha na conexão Wi-Fi\n");
        return 1;
    }

    // Exibe IP obtido
    struct netif *netif = netif_list;
    printf("Conectado! IP: %s\n", ip4addr_ntoa(&netif->ip_addr));

    while (1) {
        int btn = gpio_get(BUTTON_A);
        double temp = read_temperature_average(5);
        int field1 = (btn == 0) ? 1 : 0;

        // Monta path para ThingSpeak
        char path[128];
        snprintf(path, sizeof(path),
                 "/update?api_key=%s&field1=%d&field2=%.2f",
                 THINGSPEAK_API_KEY, field1, temp);

        // LED reflete o estado do botão
        gpio_put(LED_RED, btn == 0);

        EXAMPLE_HTTP_REQUEST_T req = {
            .hostname   = HOST,
            .url        = path,
            .port       = PORT,
            .headers_fn = http_client_header_print_fn,
            .recv_fn    = http_client_receive_print_fn
        };

        // Envia requisição HTTP
        int res = http_client_request_sync(cyw43_arch_async_context(), &req);
        printf("GET http://%s%s -> %s\n", HOST, path,
               res == 0 ? "Sucesso" : "Erro");

        sleep_ms(INTERVALO_MS);
    }
    return 0;
}
