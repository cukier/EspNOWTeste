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
#include "freertos/task.h"

#include <unistd.h>
#include <string.h>

#define ESPNOW_QUEUE_SIZE			8
#define CONFIG_ESPNOW_CHANNEL 		1
#define CONFIG_ESPNOW_PMK 			"pmk1234567890123"

//typedef enum {
//	CMD_STP, CMD_UP, CMD_DOWN, CMD_EM
//} espnow_cmd_t;
//
//typedef struct {
//	espnow_cmd_t cmd;
//	uint64_t pos;
//} espnow_pkg_t;

//typedef struct {
//	uint8_t mac_addr[ESP_NOW_ETH_ALEN];
//	esp_now_send_status_t status;
//} espnow_event_send_t;

typedef struct {
	uint8_t mac_addr[ESP_NOW_ETH_ALEN];
	uint8_t *data;
	int data_len;
} espnow_event_recv_t;

static xQueueHandle espnow_queue;
static const char *TAG = "espnow_cuki";
//static espnow_pkg_t *pkg = NULL;
static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF };

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data,
		int len) {
	espnow_event_recv_t evt;

	if (mac_addr == NULL || data == NULL || len <= 0) {
		ESP_LOGE(TAG, "Receive cb arg error");
		return;
	}

	evt.data = (uint8_t*) malloc(sizeof(uint8_t) * len);

	if (evt.data == NULL) {
		ESP_LOGE(TAG, "Malloc receive data fail");
		return;
	}

	memcpy(evt.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
	memcpy(evt.data, data, len);
	evt.data_len = len;

	if (xQueueSend(espnow_queue, &evt, portMAX_DELAY) != pdTRUE) {
		ESP_LOGW(TAG, "Send receive queue fail");
		free(evt.data);
	}
}

int wifi_init() {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	return 0;
}

esp_err_t espnow_init() {

	wifi_init();

	espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(uint8_t));

	if (espnow_queue == NULL) {
		ESP_LOGE(TAG, "Create mutex fail");
		return ESP_FAIL;
	}

	ESP_ERROR_CHECK(esp_now_init());
//	ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb));
	ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
	ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t* )CONFIG_ESPNOW_PMK));

	esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));

	if (peer == NULL) {
		ESP_LOGE(TAG, "Malloc peer information fail");
		vSemaphoreDelete(espnow_queue);
		esp_now_deinit();
		return ESP_FAIL;
	}

	memset(peer, 0, sizeof(esp_now_peer_info_t));
	peer->channel = CONFIG_ESPNOW_CHANNEL;
	peer->ifidx = ESP_IF_WIFI_STA;
	peer->encrypt = false;
	memcpy(peer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
	ESP_ERROR_CHECK(esp_now_add_peer(peer));
	free(peer);

	return ESP_OK;
}

void espnow_run() {
	int cont;
	uint8_t data[ESPNOW_QUEUE_SIZE] = { 0 };
	espnow_event_recv_t evt;

	ESP_ERROR_CHECK(espnow_init());
	ESP_LOGI(TAG, "init ok");

	while (1) {
		vTaskDelay(1500 / portTICK_RATE_MS);

		for (cont = 0; cont < ESPNOW_QUEUE_SIZE; ++cont) {
			data[cont] = cont + 1;
		}

//		esp_err_t err = esp_now_send(broadcast_mac, data, 3);
//		ESP_LOGI(TAG, "enviado: 0x%x", err);

		if (esp_now_send(broadcast_mac, data, ESPNOW_QUEUE_SIZE) != ESP_OK) {
			ESP_LOGE(TAG, "Send error");
		} else {
			ESP_LOGI(TAG, "send data ok");
		}

		for (cont = 0; cont < ESPNOW_QUEUE_SIZE; ++cont) {
			data[cont] = 0;
		}

		if (xQueueReceive(espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
			ESP_LOGI(TAG, "Recebido: %u de "MACSTR"", evt.data_len,
					MAC2STR(evt.mac_addr));

			for (cont = 0; cont < evt.data_len; ++cont) {
				ESP_LOGI(TAG, "%5d-0x%x ", cont, evt.data[cont]);
			}

			if (evt.data_len > 0)
				free(evt.data);
		}
	}
}
