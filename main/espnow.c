/*
 * esp_now.c
 *
 *  Created on: 12 Aug 2020
 *      Author: mauricio
 */

#include <espnow.h>
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"

#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

//static xQueueHandle espnow_queue;
static const char *TAG = "espnow";
static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF };

typedef struct {
	uint8_t mac_addr[ESP_NOW_ETH_ALEN];
	esp_now_send_status_t status;
} espnow_event_send_t;

typedef struct {
	uint8_t mac_addr[ESP_NOW_ETH_ALEN];
	uint8_t *data;
	int data_len;
} espnow_event_recv_t;

int wifi_init() {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
	ESP_ERROR_CHECK(esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif

	return 0;
}

int espnow_init() {

	wifi_init();

	ESP_ERROR_CHECK(esp_now_init());
//	esp_now_send(broadcast_mac, data, 3);

	return 0;
}

void espnow_run() {
	uint8_t data[3] = { 0x01, 0x02, 0x03 };

	espnow_init();
	ESP_LOGI(TAG, "init");

	while (1) {
		vTaskDelay(1000 / portTICK_RATE_MS);
		esp_now_send(broadcast_mac, data, 3);
		ESP_LOGI(TAG, "enviado");
	}
}
