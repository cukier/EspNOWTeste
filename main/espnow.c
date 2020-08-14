/*
 * esp_now.c
 *
 *  Created on: 12 Aug 2020
 *      Author: mauricio
 */

#include "espnow.h"

#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"

#include <unistd.h>
#include <string.h>

#define ESPNOW_QUEUE_SIZE			3
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

typedef enum {
	ESPNOW_SEND_CB, ESPNOW_RECV_CB
} espnow_event_id_t;

typedef struct {
	uint8_t mac_addr[ESP_NOW_ETH_ALEN];
	esp_now_send_status_t status;
} espnow_event_send_t;

typedef struct {
	uint8_t mac_addr[ESP_NOW_ETH_ALEN];
	uint8_t *data;
	int data_len;
} espnow_event_recv_t;

typedef union {
	espnow_event_recv_t recv_cb;
	espnow_event_send_t send_cb;
} espnow_info_t;

typedef struct {
	espnow_event_id_t id;
	espnow_info_t info;
} espnow_event_info_t;

static xQueueHandle espnow_queue;
static const char *TAG = "espnow";

static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF };

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data,
		int len) {
	if (mac_addr == NULL || data == NULL || len <= 0) {
		ESP_LOGE(TAG, "Receive cb arg error");
		return;
	}

//	memcpy(recv_ev->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
//	free(recv_ev->data);
//	recv_ev->data = (uint8_t*) malloc(sizeof(uint8_t) * len);
//	memcpy(recv_ev->data, data, len);
//	recv_ev->data_len = len;
//	ESP_LOGI(TAG, "Recebido");
}

static void espnow_send_cb(const uint8_t *mac_addr,
		esp_now_send_status_t status) {
	espnow_event_info_t evt;
	espnow_event_send_t *send_cb = &evt.info.send_cb;

	if (mac_addr == NULL) {
		ESP_LOGE(TAG, "Send cb arg error");
		return;
	}

	evt.id = ESPNOW_SEND_CB;
	memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);

//	memcpy(send_ev->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
//	ESP_LOGI(TAG, "Envidado");
}

static int wifi_init() {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif

	return 0;
}

esp_err_t espnow_init() {
	ESP_LOGI(TAG, "init begin...");

	wifi_init();

	espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(uint8_t));

	if (espnow_queue == NULL) {
		ESP_LOGE(TAG, "Create mutex fail");
		return ESP_FAIL;
	}

	ESP_ERROR_CHECK(esp_now_init());
	ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
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

	ESP_ERROR_CHECK(espnow_init());
	ESP_LOGI(TAG, "init ok");

	while (1) {
		vTaskDelay(1500 / portTICK_RATE_MS);

		for (cont = 0; cont < ESPNOW_QUEUE_SIZE; ++cont) {
			data[cont] = cont + 1;
		}

		esp_err_t err = esp_now_send(broadcast_mac, data, 3);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Send error 0x%x", err);
		} else {
			ESP_LOGI(TAG, "enviado");
		}

		for (cont = 0; cont < ESPNOW_QUEUE_SIZE; ++cont) {
			data[cont] = 0;
		}

		ESP_LOGI(TAG, "1");

		while (xQueueReceive(espnow_queue, &data, portMAX_DELAY) == pdTRUE) {
			ESP_LOGI(TAG, "Rec: %u %u %u", data[0], data[1], data[2]);
		}

		ESP_LOGI(TAG, "2");
	}
}

