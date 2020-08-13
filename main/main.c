#include <espnow.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "nvs.h"
#include "nvs_flash.h"

void app_main(void) {
	esp_err_t ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

	ESP_ERROR_CHECK(ret);

	xTaskCreate(espnow_run, "example_espnow_task", 4096, NULL, 4, NULL);

	while (1) {
		vTaskDelay(1500 / portTICK_RATE_MS);
	}
}
