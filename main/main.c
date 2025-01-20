#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "tb.h"

static void tb_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static thingsboard tb = {0};

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    ESP_ERROR_CHECK(tb_create(&tb, NULL, NULL, NULL, tb_event_handler, NULL));
}

void tb_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    thingsboard *tb = (thingsboard *)event_data;
    (void)tb;

    switch (event_id)
    {
    case TB_EVENT_ATTRIBUTES:
    {
        break;
    }

    default:
    {
        break;
    }
    }
}
