#include "hw_service.h"
#include "nordic_common.h"
#include "ble_srv_common.h"
#include "app_util.h"
#include "nrf_log.h"


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_hw_service   LED Button Service structure.
 * @param[in]   p_ble_evt        Event received from the BLE stack.
 */
static void on_connect(ble_hw_service_t * p_hw_service, ble_evt_t const * p_ble_evt)
{
    p_hw_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_hw_service   LED Button Service structure.
 * @param[in]   p_ble_evt        Event received from the BLE stack.
 */
static void on_disconnect(ble_hw_service_t * p_hw_service, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_hw_service->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_hw_service   LED Button Service structure.
 * @param[in]   p_ble_evt        Event received from the BLE stack.
 */
static void on_write(ble_hw_service_t * p_hw_service, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = (ble_gatts_evt_write_t *) &p_ble_evt->evt.gatts_evt.params.write;
    
    if ((p_evt_write->handle == p_hw_service->data_io_char_handles.value_handle) &&
        (p_evt_write->len == 1) &&
        (p_hw_service->evt_handler != NULL))
    {
      // Handle what happens on a write event to the characteristic value
    }
 
    // Check if the Custom value CCCD is written to and that the value is the appropriate length, i.e 2 bytes.
    if ((p_evt_write->handle == p_hw_service->data_io_char_handles.cccd_handle)
        && (p_evt_write->len == 2)
       )
    {
        // CCCD written, call application event handler
        if (p_hw_service->evt_handler != NULL)
        {
            ble_hw_evt_t evt;
 
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_DATA_IO_EVT_NOTIFICATION_ENABLED;
            }
            else
            {
                evt.evt_type = BLE_DATA_IO_EVT_NOTIFICATION_DISABLED;
            }
 
            p_hw_service->evt_handler(p_hw_service, &evt);
        }
    }
}

/*
Implement the BLE event handler function.
This function gets called automatically by the SoftDevice and is set up by the BLE_hw_DEF macro we defined at the top of the  hw_service.h file.
It processes the BLE event and hands it off to the appropriate function to be processed. It handles the connection event, the disconnection event, and the write event:
*/

void ble_hw_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_hw_service_t * p_hw_service = (ble_hw_service_t *) p_context;
 
    NRF_LOG_INFO("BLE event received. Event type = %d\r\n", p_ble_evt->header.evt_id); 
 
    if (p_hw_service == NULL || p_ble_evt == NULL)
    {
        return;
    }
 
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_hw_service, p_ble_evt);
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_hw_service, p_ble_evt);
            break;
            
        case BLE_GATTS_EVT_WRITE:
            on_write(p_hw_service, p_ble_evt);
            break;
            
        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for adding the Data I/O characteristic.
 *
 */
static uint32_t data_io_char_add(ble_hw_service_t * p_hw_service, const ble_hw_service_init_t * p_hw_service_init)
{
    uint32_t   err_code;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    
    //CCCD only needed for notifications and indications??????????   
    memset(&cccd_md, 0, sizeof(cccd_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    // characteristic metadata in ble_gatts.h
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read          = 1;
    char_md.char_props.write_wo_resp = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = NULL;
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Add the hw Data I/O Characteristic UUID
    ble_uuid128_t base_uuid = {BLE_UUID_HW_DATA_IO_CHAR_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_hw_service->uuid_type);
    if   (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    ble_uuid.type = p_hw_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_HW_DATA_IO_CHAR_UUID;
    
    // attribute metadata in ble_gatts.h
    memset(&attr_md, 0, sizeof(attr_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    // Configure the characteristic value (GATT attribute in ble_gatts.h)
    memset(&attr_char_value, 0, sizeof(attr_char_value));
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;

    // can now add the characteristic to the GATT database managed by the stack.
    return sd_ble_gatts_characteristic_add(p_hw_service->service_handle, 
                                       &char_md,
                                       &attr_char_value,
                                       &p_hw_service->data_io_char_handles);
}

uint32_t ble_hw_service_init(ble_hw_service_t * p_hw_service, const ble_hw_service_init_t * p_hw_service_init)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_hw_service->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_hw_service->evt_handler = p_hw_service_init->evt_handler;

    // Add service
    ble_uuid128_t base_uuid = {BLE_UUID_HW_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_hw_service->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    ble_uuid.type = p_hw_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_HW_SERVICE_UUID;

    // add service to the GATT database
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_hw_service->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // add the characteristic
    err_code = data_io_char_add(p_hw_service, p_hw_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

// handle updating the data I/O sending notifications to the client whenever the data I/O characteristic value is updated.
uint32_t ble_hw_data_io_value_update(ble_hw_service_t * p_hw_service, uint8_t data_io_value)
{
    if (p_hw_service == NULL)
    {
        return NRF_ERROR_NULL;
    }

    // set up a GATT server value variable which is needed to pass to the GATT database to store the characteristic value.
    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;
 
    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));
 
    gatts_value.len     = sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = &data_io_value;

    // Update database.
    err_code = sd_ble_gatts_value_set(p_hw_service->conn_handle,
                                      p_hw_service->data_io_char_handles.value_handle,
                                      &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((p_hw_service->conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params; // HVx : Handle Value x, which can be indication or notification.
 
        memset(&hvx_params, 0, sizeof(hvx_params));
 
        hvx_params.handle = p_hw_service->data_io_char_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;
 
        err_code = sd_ble_gatts_hvx(p_hw_service->conn_handle, &hvx_params);
        NRF_LOG_INFO("sd_ble_gatts_hvx result: %x. \r\n", err_code); 
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
        NRF_LOG_INFO("sd_ble_gatts_hvx result: NRF_ERROR_INVALID_STATE. \r\n"); 
    }
 
    return err_code;
}