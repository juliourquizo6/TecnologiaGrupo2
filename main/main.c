#include <esp_event.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <protocol_examples_common.h>
#include <tb.h>

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    esp_event_loop_args_t event_loop_cfg = {
        .queue_size = 10,
        .task_stack_size = 2048,
    };
    esp_event_loop_handle_t event_loop = NULL;
    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_cfg, &event_loop));

    thingsboard tb = {0};
    ESP_ERROR_CHECK(thingsboard_init(&tb, event_loop, NULL, 0, NULL, NULL));
    ESP_ERROR_CHECK(tb_connect(tb));
    ESP_ERROR_CHECK(tb_send_telemetry(tb, "{\"temperature\": 25.0}"));

    tb_destroy(tb);
}
