#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

#include "service.h"
#include "utils.h"

#define UUID_ATTR1 0x0001
#define UUID_ATTR2 0x0002

static const ble_uuid128_t gUUID =
{
    .uuid128 = UUID_BLE_SERVICE_BASE
};

static BLEService gService;

static BLEAttr          gAttrHSVDesc;
static ble_gatts_attr_t gAttrHSV;

static BLEAttr          gAttrInputDesc;
static ble_gatts_attr_t gAttrInput;

ret_code_t bleServiceSetup(void)
{
    memset(&gService, 0, sizeof(gService));
    gService.uuid.uuid = UUID_BLE_SERVICE_SHRT;
    gService.uuid.type = BLE_UUID_TYPE_VENDOR_BEGIN;

    ret_code_t errCode;
    errCode = sd_ble_uuid_vs_add(&gUUID, &gService.uuid.type);
    VERIFY_SUCCESS(errCode);
    errCode = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &gService.uuid, &gService.hserv);
    VERIFY_SUCCESS(errCode);
    return NRF_SUCCESS;
}

ret_code_t bleServiceAttrHSVSetup(ColorHSV* ptr)
{
    memset(&gAttrHSVDesc, 0, sizeof(gAttrHSVDesc));
    gAttrHSVDesc.uuid.uuid                = UUID_ATTR1;
    gAttrHSVDesc.uuid.type                = BLE_UUID_TYPE_VENDOR_BEGIN;
    gAttrHSVDesc.charmd.char_props.read   = 1;
    gAttrHSVDesc.charmd.char_props.notify = 1;
    gAttrHSVDesc.attrmd.vloc              = BLE_GATTS_VLOC_USER;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&gAttrHSVDesc.attrmd.read_perm);

    memset(&gAttrHSV, 0, sizeof(gAttrHSV));
    gAttrHSV.p_uuid    = &gAttrHSVDesc.uuid;
    gAttrHSV.p_attr_md = &gAttrHSVDesc.attrmd;
    gAttrHSV.init_len  = sizeof(*ptr);
    gAttrHSV.max_len   = sizeof(*ptr);
    gAttrHSV.p_value   = (uint8_t*)ptr;

    ret_code_t errCode;
    errCode = sd_ble_uuid_vs_add(&gUUID, &gAttrHSVDesc.uuid.type);
    VERIFY_SUCCESS(errCode);
    errCode = sd_ble_gatts_characteristic_add(gService.hserv, &gAttrHSVDesc.charmd, &gAttrHSV, &gAttrHSVDesc.handles);
    VERIFY_SUCCESS(errCode);
    return NRF_SUCCESS;
}

ret_code_t bleServiceAttrHSVNotify(void)
{
    ble_gatts_hvx_params_t params;
    memset(&params, 0, sizeof(params));
    params.handle = gAttrHSVDesc.handles.value_handle;
    params.type   = BLE_GATT_HVX_NOTIFICATION;
    params.p_len  = &gAttrHSV.init_len;
    return sd_ble_gatts_hvx(gService.hconn, &params);
}

uint32_t bleServiceAttrHSVGetHandle(void)
{
    return gAttrInputDesc.handles.value_handle;
}

ret_code_t bleServiceAttrInputSetup(ColorHSV* ptr)
{
    memset(&gAttrInputDesc, 0, sizeof(gAttrInputDesc));
    gAttrInputDesc.uuid.uuid               = UUID_ATTR2;
    gAttrInputDesc.uuid.type               = BLE_UUID_TYPE_VENDOR_BEGIN;
    gAttrInputDesc.charmd.char_props.write = 1;
    gAttrInputDesc.attrmd.vloc             = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&gAttrInputDesc.attrmd.write_perm);

    memset(&gAttrInput, 0, sizeof(gAttrInput));
    gAttrInput.p_uuid    = &gAttrInputDesc.uuid;
    gAttrInput.p_attr_md = &gAttrInputDesc.attrmd;
    gAttrInput.init_len  = sizeof(*ptr);
    gAttrInput.max_len   = sizeof(*ptr);
    gAttrInput.p_value   = (uint8_t*)ptr;

    ret_code_t errCode;
    errCode = sd_ble_uuid_vs_add(&gUUID, &gAttrInputDesc.uuid.type);
    VERIFY_SUCCESS(errCode);
    errCode = sd_ble_gatts_characteristic_add(gService.hserv, &gAttrInputDesc.charmd, &gAttrInput, &gAttrInputDesc.handles);
    VERIFY_SUCCESS(errCode);
    return NRF_SUCCESS;
}
