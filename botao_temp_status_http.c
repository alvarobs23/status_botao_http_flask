#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "example_http_client_util.h"

// Configurações
#define HOST        "192.168.18.8"
#define PORT        5000
#define INTERVALO_MS 500
#define BUTTON_A    5
#define LED_RED     13

// Inicializa o ADC e habilita o sensor interno
void adc_setup() {
    adc_init();
    adc_set_temp_sensor_enabled(true);
}

// Lê temperatura do sensor interno (canal 4)
float read_temperature() {
    adc_select_input(4);
    uint16_t raw = adc_read();
    float voltage = raw * 3.3f / (1 << 12);  // 12 bits
    // Fórmula para Pico:
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

// Média de N amostras
float read_temperature_average(int samples) {
    float sum = 0.0f;
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

    // Wi-Fi
    if (cyw43_arch_init()) return 1;
    cyw43_arch_enable_sta_mode();
    cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                       CYW43_AUTH_WPA2_AES_PSK, 30000);

    printf("Cliente HTTP iniciado — IP: %s\n",
           ip4addr_ntoa(netif_ip4_addr(netif_list)));

    int last_button = 1;

    while (1) {
        int btn = gpio_get(BUTTON_A);
        float temp = read_temperature_average(5);
        char path[128];

        // Define caminho incluindo status e temperatura
        snprintf(path, sizeof(path),
                 "/update?btn=%d&temp=%.2f", btn, temp);

        // LED reflete o botão
        gpio_put(LED_RED, btn == 0);

        // Configura e envia requisição
        EXAMPLE_HTTP_REQUEST_T req = {
            .hostname   = HOST,
            .url        = path,
            .port       = PORT,
            .headers_fn = http_client_header_print_fn,
            .recv_fn    = http_client_receive_print_fn
        };

        printf("GET %s\n", path);
        int res = http_client_request_sync(cyw43_arch_async_context(), &req);
        printf(res == 0 ? "Sucesso\n" : "Erro %d\n", res);

        sleep_ms(INTERVALO_MS);
    }

    return 0;
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
