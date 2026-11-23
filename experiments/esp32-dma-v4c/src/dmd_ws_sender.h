#pragma once
#include <atomic>
#include <WiFi.h>
#include "esp_timer.h"
#include "esp_websocket_client.h"
#include "dmd_ws_protocol.h"

#if __has_include("dmd_ws_local.h")
#include "dmd_ws_local.h"
#endif
#ifndef DMD_WS_WIFI_SSID
#define DMD_WS_WIFI_SSID "Loki"
#endif
#ifndef DMD_WS_URL
#define DMD_WS_URL "ws://192.168.0.125:8765/dmd"
#endif
#ifdef DMD_WS_WIFI_PASSWORD
static constexpr bool wsConfigured = true;
#else
static constexpr bool wsConfigured = false;
#define DMD_WS_WIFI_PASSWORD ""
#endif

static constexpr unsigned wsQueueDepth = 2;
static constexpr uint64_t wsMaxAgeUs = 150000;
struct WsFrame {
    uint64_t acquiredUs;
    uint32_t connection;
    uint8_t bytes[dmd_ws::frameBytes];
};

static QueueHandle_t wsQueue;
static std::atomic<uint32_t> wsReady{0}, wsOfflineDrops{0}, wsQueueDrops{0};

static void wsOffer(const uint8_t *pixels, uint32_t capturedUs) {
    const uint32_t connection = wsReady.load();
    if (!connection) {
        ++wsOfflineDrops;
        return;
    }
    WsFrame frame;
    frame.connection = connection;
    frame.acquiredUs = dmd_ws::acquisitionTime(esp_timer_get_time(), capturedUs);
    dmd_ws::gray2(frame.bytes, pixels, uint32_t(frame.acquiredUs / 1000));
    if (xQueueSend(wsQueue, &frame, 0) != pdTRUE) ++wsQueueDrops;
}

enum class WsLink { Connecting, Connected, Closed };
static std::atomic<WsLink> wsLink{WsLink::Closed};

static void wsEvent(void *, esp_event_base_t, int32_t event, void *) {
    if (event == WEBSOCKET_EVENT_CONNECTED) wsLink.store(WsLink::Connected);
    if (event == WEBSOCKET_EVENT_DISCONNECTED || event == WEBSOCKET_EVENT_CLOSED) {
        wsReady.store(0);
        wsLink.store(WsLink::Closed);
    }
}

static bool wsSend(esp_websocket_client_handle_t client, const uint8_t *data, int length) {
    return esp_websocket_client_send_bin(client, reinterpret_cast<const char *>(data), length,
                                         pdMS_TO_TICKS(100)) == length;
}

static void wsTask(void *) {
    static WsFrame frame;
    uint8_t dimensions[dmd_ws::dimensionsBytes];
    dmd_ws::dimensions(dimensions);
    esp_websocket_client_config_t config{};
    config.uri = DMD_WS_URL;
    config.disable_auto_reconnect = true;
    config.buffer_size = 2048;
    config.task_prio = 1;
    config.task_stack = 4096;

    if (wsConfigured) {
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.begin(DMD_WS_WIFI_SSID, DMD_WS_WIFI_PASSWORD);
    } else {
        Serial.println("WS CONFIG missing DMD_WS_WIFI_PASSWORD; sender offline");
    }

    esp_websocket_client_handle_t client = nullptr;
    uint32_t attempts = 0, connections = 0, sent = 0, staleDrops = 0;
    uint32_t sendDrops = 0, dimensionErrors = 0, startErrors = 0;
    uint32_t lastAttempt = millis() - 2000, lastWifi = millis(), lastReport = millis();
    for (;;) {
        const uint32_t now = millis();
        const bool wifi = wsConfigured && WiFi.status() == WL_CONNECTED;
        if (client && (!wifi || wsLink.load() == WsLink::Closed ||
                       (!wsReady.load() && now - lastAttempt >= 12000))) {
            wsReady.store(0);
            esp_websocket_client_stop(client);
            esp_websocket_client_destroy(client);
            client = nullptr;
            while (xQueueReceive(wsQueue, &frame, 0) == pdTRUE) ++staleDrops;
            lastAttempt = millis();
        }
        if (!wifi && wsConfigured && now - lastWifi >= 5000) {
            lastWifi = now;
            WiFi.reconnect();
        }
        if (wifi && !client && uint32_t(millis() - lastAttempt) >= 2000) {
            lastAttempt = millis();
            ++attempts;
            wsLink.store(WsLink::Connecting);
            client = esp_websocket_client_init(&config);
            if (!client ||
                esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, wsEvent, nullptr) != ESP_OK ||
                esp_websocket_client_start(client) != ESP_OK) {
                ++startErrors;
                if (client) esp_websocket_client_destroy(client);
                client = nullptr;
                wsLink.store(WsLink::Closed);
            }
        }
        if (client && wsLink.load() == WsLink::Connected && !wsReady.load()) {
            if (wsSend(client, dimensions, sizeof(dimensions)) && wsLink.load() == WsLink::Connected) {
                wsReady.store(++connections);
            } else {
                ++dimensionErrors;
                wsLink.store(WsLink::Closed);
            }
        }
        const uint32_t ready = wsReady.load();
        if (ready && xQueueReceive(wsQueue, &frame, 0) == pdTRUE) {
            if (frame.connection != ready || esp_timer_get_time() - frame.acquiredUs > wsMaxAgeUs) {
                ++staleDrops;
            } else if (!wsSend(client, frame.bytes, sizeof(frame.bytes))) {
                ++sendDrops;
                wsReady.store(0);
                wsLink.store(WsLink::Closed);
            } else {
                ++sent;
            }
        }
        if (uint32_t(millis() - lastReport) >= 5000) {
            lastReport = millis();
            Serial.printf("WS STATUS wifi=%u ready=%u attempts=%lu connections=%lu reconnects=%lu sent=%lu "
                          "offlineDrops=%lu queueDrops=%lu staleDrops=%lu sendDrops=%lu dimensionErrors=%lu startErrors=%lu\n",
                          unsigned(wifi), unsigned(wsReady.load() != 0), (unsigned long)attempts,
                          (unsigned long)connections, (unsigned long)(connections ? connections - 1 : 0),
                          (unsigned long)sent, (unsigned long)wsOfflineDrops.load(),
                          (unsigned long)wsQueueDrops.load(), (unsigned long)staleDrops,
                          (unsigned long)sendDrops, (unsigned long)dimensionErrors, (unsigned long)startErrors);
        }
        vTaskDelay(1);
    }
}

static void wsInit() {
    wsQueue = xQueueCreate(wsQueueDepth, sizeof(WsFrame));
    if (!wsQueue || xTaskCreatePinnedToCore(wsTask, "dmd-websocket", 5120, nullptr, 1, nullptr, 0) != pdPASS)
        fatal("WebSocket queue/task allocation");
    Serial.printf("WS CONFIG url=%s ssid=%s queue=%u maxAgeMs=%u\n",
                  DMD_WS_URL, DMD_WS_WIFI_SSID, wsQueueDepth, unsigned(wsMaxAgeUs / 1000));
}
