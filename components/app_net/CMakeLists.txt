set(APP_NET_MQTT_CA_CERT "${CMAKE_CURRENT_LIST_DIR}/certs/ca.crt")

if(NOT EXISTS "${APP_NET_MQTT_CA_CERT}")
    message(WARNING "components/app_net/certs/ca.crt is missing. Run tools/node_red/generate_tls_certs.sh before using MQTTS telemetry.")
    file(WRITE "${APP_NET_MQTT_CA_CERT}" "Generate a local demo CA with tools/node_red/generate_tls_certs.sh before using MQTTS telemetry.\n")
endif()

idf_component_register(SRCS "app_net.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_wifi esp_event esp_netif nvs_flash mqtt mpu9250 lwip
                       EMBED_TXTFILES "${APP_NET_MQTT_CA_CERT}")
