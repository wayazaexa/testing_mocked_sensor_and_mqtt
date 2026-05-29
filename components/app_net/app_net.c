#include "app_net.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

static const char *TAG = "app_net";

extern const uint8_t mqtt_ca_crt_start[] asm("_binary_ca_crt_start");

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MQTT_CONNECTED_BIT BIT2

#ifndef CONFIG_EXAMPLE_MQTT_CONNECT_TIMEOUT_MS
#define CONFIG_EXAMPLE_MQTT_CONNECT_TIMEOUT_MS 10000
#endif

static EventGroupHandle_t s_wifi_event_group;
static esp_mqtt_client_handle_t s_mqtt_client;
static int s_retry_num;

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_EXAMPLE_WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retrying WiFi connection");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_start(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to initialize NVS");

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize netif");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Failed to create event loop");
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed to initialize WiFi");

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_any_id),
                        TAG,
                        "Failed to register WiFi handler");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_got_ip),
                        TAG,
                        "Failed to register IP handler");

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_EXAMPLE_WIFI_SSID,
            .password = CONFIG_EXAMPLE_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to set WiFi mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to set WiFi config");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start WiFi");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(CONFIG_EXAMPLE_WIFI_CONNECT_TIMEOUT_MS));

    if ((bits & WIFI_CONNECTED_BIT) != 0) {
        ESP_LOGI(TAG, "Connected to WiFi SSID %s", CONFIG_EXAMPLE_WIFI_SSID);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to connect to WiFi SSID %s", CONFIG_EXAMPLE_WIFI_SSID);
    return ESP_ERR_TIMEOUT;
}

static esp_err_t time_sync_start(void)
{
    time_t now = 0;
    struct tm timeinfo = {0};
    time(&now);
    gmtime_r(&now, &timeinfo);
    if (timeinfo.tm_year >= (2024 - 1900)) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Synchronizing time with SNTP server %s", CONFIG_EXAMPLE_SNTP_SERVER);
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_EXAMPLE_SNTP_SERVER);
    esp_err_t ret = esp_netif_sntp_init(&config);
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "SNTP already initialized, waiting for time sync");
    } else {
        ESP_RETURN_ON_ERROR(ret, TAG, "Failed to initialize SNTP");
    }

    ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(CONFIG_EXAMPLE_SNTP_SYNC_TIMEOUT_MS));
    ESP_RETURN_ON_ERROR(ret, TAG, "SNTP time sync failed");

    time(&now);
    gmtime_r(&now, &timeinfo);
    ESP_LOGI(TAG,
             "Time synchronized: %04d-%02d-%02d %02d:%02d:%02d UTC",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        xEventGroupSetBits(s_wifi_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        xEventGroupClearBits(s_wifi_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_ERROR:
        if (event != NULL && event->error_handle != NULL) {
            ESP_LOGE(TAG,
                     "MQTT error type=%d esp_tls_err=0x%x stack_err=0x%x cert_flags=0x%x sock_errno=%d",
                     event->error_handle->error_type,
                     event->error_handle->esp_tls_last_esp_err,
                     event->error_handle->esp_tls_stack_err,
                     event->error_handle->esp_tls_cert_verify_flags,
                     event->error_handle->esp_transport_sock_errno);
        } else {
            ESP_LOGE(TAG, "MQTT error");
        }
        break;
    default:
        ESP_LOGD(TAG, "MQTT event %ld, msg_id=%d", event_id, event != NULL ? event->msg_id : -1);
        break;
    }
}

esp_err_t app_net_start(void)
{
    ESP_RETURN_ON_ERROR(wifi_start(), TAG, "WiFi startup failed");
    ESP_RETURN_ON_ERROR(time_sync_start(), TAG, "Time sync failed; MQTTS certificate validation needs a valid clock");

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = CONFIG_EXAMPLE_MQTT_BROKER_URI,
            .verification.certificate = (const char *)mqtt_ca_crt_start,
        },
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_mqtt_client_register_event(s_mqtt_client,
                                                       ESP_EVENT_ANY_ID,
                                                       mqtt_event_handler,
                                                       NULL),
                        TAG,
                        "Failed to register MQTT handler");
    ESP_RETURN_ON_ERROR(esp_mqtt_client_start(s_mqtt_client), TAG, "Failed to start MQTT client");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           MQTT_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(CONFIG_EXAMPLE_MQTT_CONNECT_TIMEOUT_MS));
    if ((bits & MQTT_CONNECTED_BIT) == 0) {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t app_net_publish_mpu9250_probe(uint32_t seq, const mpu9250_probe_result_t *result)
{
    if (s_mqtt_client == NULL || result == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    char topic[96];
    char payload[192];
    int topic_len = snprintf(topic,
                             sizeof(topic),
                             "iot25/%s/sensor/mpu9250",
                             CONFIG_EXAMPLE_DEVICE_ID);
    int payload_len = snprintf(payload,
                               sizeof(payload),
                               "{\"device_id\":\"%s\",\"seq\":%" PRIu32 ",\"who_am_i\":%u,\"status\":\"%s\"}",
                               CONFIG_EXAMPLE_DEVICE_ID,
                               seq,
                               result->who_am_i,
                               mpu9250_status_to_string(result->status));

    if (topic_len < 0 || topic_len >= (int)sizeof(topic) ||
        payload_len < 0 || payload_len >= (int)sizeof(payload)) {
        return ESP_ERR_INVALID_SIZE;
    }

    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, payload, 0, 1, 0);
    if (msg_id < 0) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Published MQTT message id=%d topic=%s payload=%s", msg_id, topic, payload);
    return ESP_OK;
}
