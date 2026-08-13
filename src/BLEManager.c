#include "BLEManager.h"
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "imu_app.h"

/* BLE Includes */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLEManager";
static uint16_t g_val_handle;
static uint8_t ble_addr_type;
static uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static bool g_is_connected = false;

static uint8_t g_raw_data[BLE_MAX_RAW_DATA_LEN];
static uint16_t g_raw_data_len = 0;

static int ble_gap_event(struct ble_gap_event *event, void *arg);
static int ble_gatt_svr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

// Define UUIDs
static const ble_uuid16_t gatt_svr_svc_uuid = BLE_UUID16_INIT(BLE_SERVICE_UUID);
static const ble_uuid16_t gatt_svr_chr_uuid = BLE_UUID16_INIT(BLE_CHARACTERISTIC_UUID);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = &gatt_svr_chr_uuid.u,
                .access_cb = ble_gatt_svr_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &g_val_handle,
            },
            {
                0, // No more characteristics
            }},
    },
    {
        0, // No more services
    },
};

static int ble_gatt_svr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len > sizeof(g_raw_data))
        {
            ESP_LOGW(TAG, "Received raw message too long: %d bytes (truncating to %d)", len, (int)sizeof(g_raw_data));
            len = sizeof(g_raw_data);
        }

        int rc = ble_hs_mbuf_to_flat(ctxt->om, g_raw_data, sizeof(g_raw_data), &len);
        if (rc != 0)
        {
            return BLE_ATT_ERR_UNLIKELY;
        }

        g_raw_data_len = len;

        ESP_LOGI(TAG, "Received %d bytes:", len);
        ESP_LOG_BUFFER_HEX(TAG, g_raw_data, len);

        return 0;
    }
    else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
    {
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static void ble_app_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name = "ESP32_NAV";
    int rc;

    memset(&fields, 0, sizeof(fields));

    // Discoverability flags
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // Transmit Power
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    // Device Name
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    // Advertised Services (16-bit UUID)
    fields.uuids16 = (ble_uuid16_t[]){
        BLE_UUID16_INIT(BLE_SERVICE_UUID)};
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Error setting advertisement data; rc=%d", rc);
        return;
    }

    // Set advertisement parameters
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Error starting advertisement; rc=%d", rc);
    }
    else
    {
        ESP_LOGI(TAG, "BLE advertising started successfully");
    }
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "BLE Connection %s; status=%d",
                 event->connect.status == 0 ? "established" : "failed",
                 event->connect.status);
        if (event->connect.status == 0)
        {
            g_conn_handle = event->connect.conn_handle;
            g_is_connected = true;
        }
        else
        {
            g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            g_is_connected = false;
            ble_app_advertise();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "BLE Disconnected; reason=%d", event->disconnect.reason);
        g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        g_is_connected = false;
        ble_app_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "BLE Advertising complete; reason=%d", event->adv_complete.reason);
        ble_app_advertise();
        break;
    }
    return 0;
}

static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &ble_addr_type);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Error determining address type; rc=%d", rc);
        return;
    }

    ble_app_advertise();
}

void BLE_Manager_Init(void)
{
    g_raw_data_len = 0;
    memset(g_raw_data, 0, sizeof(g_raw_data));
    g_is_connected = false;
    g_conn_handle = BLE_HS_CONN_HANDLE_NONE;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    int rc = nimble_port_init();
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Failed to initialize NimBLE; rc=%d", rc);
        return;
    }

    // Configure NimBLE
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.gatts_register_cb = NULL;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // Register GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Error counting GATT services; rc=%d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Error adding GATT services; rc=%d", rc);
        return;
    }

    // Set Device Name
    rc = ble_svc_gap_device_name_set("ESP32_NAV");
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Error setting GAP device name; rc=%d", rc);
        return;
    }

    nimble_port_freertos_init(ble_host_task);
}

void BLE_Manager_Deinit(void)
{
    nimble_port_stop();
    g_is_connected = false;
    g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    ESP_LOGI(TAG, "BLE Manager Deinitialized");
}

const uint8_t *BLE_Manager_GetRawData(uint16_t *len)
{
    if (len != NULL)
    {
        *len = g_raw_data_len;
    }
    return g_raw_data;
}

bool BLE_Manager_IsConnected(void)
{
    return g_is_connected;
}

esp_err_t BLE_Manager_SendData(const uint8_t *data, uint16_t len)
{
    if (!g_is_connected || g_conn_handle == BLE_HS_CONN_HANDLE_NONE)
    {
        ESP_LOGW(TAG, "Cannot send data: BLE master not connected");
        return ESP_FAIL;
    }

    if (data == NULL || len == 0)
    {
        ESP_LOGW(TAG, "Cannot send data: Invalid data buffer or length");
        return ESP_ERR_INVALID_ARG;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory buffer for BLE notification");
        return ESP_ERR_NO_MEM;
    }

    int rc = ble_gatts_notify_custom(g_conn_handle, g_val_handle, om);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Failed to send BLE notification; rc=%d", rc);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sent %d bytes to BLE master", len);
    return ESP_OK;
}

void BLE_Send_Task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (BLE_Manager_IsConnected())
        {
            float yaw = IMU_App_GetYaw();
            uint16_t nav_heading = (uint16_t)(int)yaw * 10;
            uint8_t payload[8];
            payload[0] = 0xAA;                          // Start frame
            payload[1] = 0xBD;                          // Reserved (Speed OBD)
            payload[2] = (uint8_t)(nav_heading >> 8);   // Heading MSB
            payload[3] = (uint8_t)(nav_heading & 0xFF); // Heading LSB
            payload[4] = 0x00;                          // Reserved
            payload[5] = 0x00;                          // Reserved
            payload[6] = 0x00;                          // Reserved
            payload[7] = 0x55;                          // End frame

            esp_err_t err = BLE_Manager_SendData(payload, sizeof(payload));
            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "BLE_Send_Task: Y=%d, OBD=",
                         (int)nav_heading);
            }
        }
    }
}
