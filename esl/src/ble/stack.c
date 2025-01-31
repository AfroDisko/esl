#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#include "stack.h"
#include "service.h"
#include "queue.h"

#define DEVICE_NAME                     "Aleksei Chernyshov"
#define MANUFACTURER_NAME               "NordicSemiconductor"
#define APP_ADV_INTERVAL                300

#define APP_ADV_DURATION                18000
#define APP_BLE_OBSERVER_PRIO           3
#define APP_BLE_CONN_CFG_TAG            1

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)
#define SLAVE_LATENCY                   0
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)
#define MAX_CONN_PARAMS_UPDATE_COUNT    3

#define DEAD_BEEF                       0xDEADBEEF

#define SCAN_RESP_DATA                  "Aleksei Chernyshov"

#define SEC_PARAM_BOND                  0
#define SEC_PARAM_MITM                  0
#define SEC_PARAM_LESC                  0
#define SEC_PARAM_KEYPRESS              0
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE
#define SEC_PARAM_OOB                   0
#define SEC_PARAM_MIN_KEY_SIZE          7
#define SEC_PARAM_MAX_KEY_SIZE          16    

NRF_BLE_GATT_DEF(m_gatt);
NRF_BLE_QWR_DEF(m_qwr);
BLE_ADVERTISING_DEF(m_advertising);

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

static ble_uuid_t m_adv_uuids[] =
{
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE},
    {UUID_BLE_SERVICE_SHRT, BLE_UUID_TYPE_BLE}
};

static ble_advdata_manuf_data_t gScanRespData =
{
    .company_identifier = 0,
    .data               =
    {
        .size   = sizeof(SCAN_RESP_DATA),
        .p_data = (uint8_t*)SCAN_RESP_DATA
    }
};

static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

static void on_conn_params_evt(ble_conn_params_evt_t* p_evt)
{
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
        sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
}

static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch(ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising");
            break;

        default:
            break;
    }
}

static void onEventWrite(ble_evt_t const* p_ble_evt, void* p_context)
{
    NRF_LOG_INFO("Handle %u", (p_ble_evt->evt).gatts_evt.params.write.handle);
    if((p_ble_evt->evt).gatts_evt.params.write.handle != bleServiceAttrHSVGetHandle())
        return;

    Event event =
    {
        .type     = EventChangeColorHSV,
        .data.hsv =
        {
            .h = (p_ble_evt->evt).gatts_evt.params.write.data[0],
            .s = (p_ble_evt->evt).gatts_evt.params.write.data[1],
            .v = (p_ble_evt->evt).gatts_evt.params.write.data[2]
        }
    };
    queueEventEnqueue(event);
    NRF_LOG_INFO("Queued color change by BLE");
}

static void ble_evt_handler(ble_evt_t const* p_ble_evt, void* p_context)
{
    switch(p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            break;

        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
            NRF_LOG_DEBUG("PHY update request");
            const ble_gap_phys_t phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            NRF_LOG_DEBUG("GATT Client Timeout");
            sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            break;

        case BLE_GATTS_EVT_WRITE:
            NRF_LOG_DEBUG("GATT Server Write");
            onEventWrite(p_ble_evt, p_context);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            NRF_LOG_DEBUG("GATT Server Timeout");
            sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            break;

        default:
            break;
    }
}

static void pm_handler(const pm_evt_t* p_pm_evt)
{
    pm_handler_on_pm_evt(p_pm_evt);
    pm_handler_disconnect_on_sec_failure(p_pm_evt);
    pm_handler_flash_clean(p_pm_evt);

    switch(p_pm_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
            break;

        default:
            break;
    }
}

void bleStackSetup(void)
{
    nrf_sdh_enable_request();
    uint32_t ram_start = 0;
    nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    nrf_sdh_ble_enable(&ram_start);
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    sd_ble_gap_device_name_set(&sec_mode, (uint8_t const*)DEVICE_NAME, strlen(DEVICE_NAME));
    sd_ble_gap_appearance_set(BLE_APPEARANCE_UNKNOWN);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;
    sd_ble_gap_ppcp_set(&gap_conn_params);

    nrf_ble_gatt_init(&m_gatt, NULL);

    nrf_ble_qwr_init_t qwr_init = {0};
    qwr_init.error_handler = nrf_qwr_error_handler;
    nrf_ble_qwr_init(&m_qwr, &qwr_init);

    pm_init();
    ble_gap_sec_params_t sec_param;
    memset(&sec_param, 0, sizeof(sec_param));
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 0;
    sec_param.kdist_own.id   = 0;
    sec_param.kdist_peer.enc = 0;
    sec_param.kdist_peer.id  = 0;
    pm_sec_params_set(&sec_param);
    pm_register(pm_handler);
    pm_peers_delete();

    ble_advertising_init_t init;
    memset(&init, 0, sizeof(init));
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = false;
    init.advdata.include_ble_device_addr = false;
    init.config.ble_adv_fast_enabled     = true;
    init.config.ble_adv_fast_interval    = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout     = APP_ADV_DURATION;
    init.srdata.uuids_complete.uuid_cnt  = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids   = m_adv_uuids;
    init.srdata.p_manuf_specific_data    = &gScanRespData;
    init.evt_handler = on_adv_evt;
    ble_advertising_init(&m_advertising, &init);
    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);

    ble_conn_params_init_t cp_init;
    memset(&cp_init, 0, sizeof(cp_init));
    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;
    ble_conn_params_init(&cp_init);

    ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
}
