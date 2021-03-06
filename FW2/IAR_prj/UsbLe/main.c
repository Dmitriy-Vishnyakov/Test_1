/**
ECG Spiro USB/BLE device FW
ome from sbd_ble_uart_example
USBD CDC ACM over BLE application main file.
*/

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"

#include "app_error.h"
#include "app_util.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#include "bee_data_types.h"
#include "host_interfase.h"
#include "flash_mem.h"
#include "timestamp_timer.h"


#include "../twi_sensor/acc_sensor.h"

#ifdef ECG
#include "ecg_pin_config.h"
#include "../ecg_sensor/ecg_adc.h"
#endif
#ifdef SPIRO
#include "spiro_pin_config.h"
#endif
//------------------------------------------------------------------------------

// app vars
#define BLE_MAX_SEND_LENGTH  120
#if BLE_MAX_SEND_LENGTH > (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
#error "length too long"
#endif

//------------------------------------------------------------------------------
const char version_name[] = "UsbLe V1.002";
char firmware_version[VERSION_NAME_LENGTH];
#define DEVICE_NAME  "MobiCardio 1234"                      /**< Name of device. Will be included in the advertising data. */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define RX_BUFFER_SIZE 512
static char rx_buffer_fifo[RX_BUFFER_SIZE];
static uint16_t inRxBufferIndex = 0;
static uint16_t outRxBufferIndex = 0;
static uint16_t rxBufferLength = 0;

#define TX_BUFFER_SIZE 4096
static char tx_buffer_fifo[TX_BUFFER_SIZE];
static uint16_t inTxBufferIndex = 0;
static uint16_t outTxBufferIndex = 0;
static uint16_t txBufferLength = 0;

static char send_buffer[BLE_MAX_SEND_LENGTH];

bool readyToSend = true;
WriteDataFn_t SendDataToHostFn = NULL;
//------------------------------------------------------------------------------

// app prototipes
void channelWriteBle(char*data, uint16_t dataLength);
void channelWriteUsb(char*data, uint16_t dataLength);
void addDataToOutputQueue(char*data, uint16_t dataLength);
//------------------------------------------------------------------------------

#define LED_BLE_NUS_CONN (BSP_BOARD_LED_0)
#define LED_BLE_NUS_RX   (BSP_BOARD_LED_1)
#define LED_CDC_ACM_CONN (BSP_BOARD_LED_2)
#define LED_CDC_ACM_RX   (BSP_BOARD_LED_3)

#define LED_BLINK_INTERVAL 800

APP_TIMER_DEF(m_blink_ble);
APP_TIMER_DEF(m_blink_cdc);


/**
 * @brief App timer handler for blinking the LEDs.
 *
 * @param p_context LED to blink.
 */
void blink_handler(void * p_context)
{
    bsp_board_led_invert((uint32_t) p_context);
}

#define ENDLINE_STRING "\r\n"

// USB DEFINES START
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

static char m_cdc_data_array[BLE_NUS_MAX_DATA_LEN];

/** @brief CDC_ACM class instance */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

// USB DEFINES END

// BLE DEFINES START
#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_FEATURE_NOT_SUPPORTED       BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2        /**< Reply when unsupported features are requested. */

#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_DURATION                18000                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */


#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms). Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms). Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating an event (connect or start of notification) to the first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump. Can be used to identify stack location on stack unwind. */

BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;                 /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};
//------------------------------------------------------------------------------
// BLE DEFINES END

/**
 * @brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of an assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name) {
	
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}
////////////////////////////////////////////////////////////////////////////////


/** @brief Function for initializing the timer module. */
static void timers_init(void) {

    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&m_blink_ble, APP_TIMER_MODE_REPEATED, blink_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&m_blink_cdc, APP_TIMER_MODE_REPEATED, blink_handler);
    APP_ERROR_CHECK(err_code);
}
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void) {

    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}
////////////////////////////////////////////////////////////////////////////////


/**
 * @brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function processes the data received from the Nordic UART BLE Service and sends
 *          it to the USBD CDC ACM module.
 *
 * @param[in] p_evt Nordic UART Service event.
 */
static void nus_data_handler(ble_nus_evt_t * p_evt) {

    if (p_evt->type == BLE_NUS_EVT_RX_DATA) {
		
		bsp_board_led_invert(LED_BLE_NUS_RX);

		//  move incomming data to our buffer
		NRF_LOG_INFO("Got bytes from BLE: %lu ", p_evt->params.rx_data.length);
		for (int i = 0; i < p_evt->params.rx_data.length; i++) {
			rx_buffer_fifo[inRxBufferIndex] = p_evt->params.rx_data.p_data[i];
			inRxBufferIndex = (inRxBufferIndex+1)&(RX_BUFFER_SIZE-1);
			rxBufferLength++;
		}
		SendDataToHostFn = channelWriteBle;
    }
}
////////////////////////////////////////////////////////////////////////////////


/** @brief Function for initializing services that will be used by the application. */
static void services_init(void) {

    uint32_t       err_code;
    ble_nus_init_t nus_init;

    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}
////////////////////////////////////////////////////////////////////////////////


/**
 * @brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error) {

    APP_ERROR_HANDLER(nrf_error);
}
////////////////////////////////////////////////////////////////////////////////


/** @brief Function for initializing the Connection Parameters module. */
static void conn_params_init(void) {

    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = true;
    cp_init.evt_handler                    = NULL;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}
////////////////////////////////////////////////////////////////////////////////


/**
 * @brief Function for putting the chip into sleep mode.
 *
 * @note This function does not return.
 */
static void sleep_mode_enter(void) {

    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}
////////////////////////////////////////////////////////////////////////////////


/** @brief Function for starting advertising. */
static void advertising_start(void) {

    uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Function for handling advertising events.
 *
 * @details This function is called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt) {

    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = app_timer_start(m_blink_ble,
                                       APP_TIMER_TICKS(LED_BLINK_INTERVAL),
                                       (void *) LED_BLE_NUS_CONN);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            NRF_LOG_INFO("Advertising timeout, restarting.")
            advertising_start();
            break;
        default:
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////


/**
 * @brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context) {

    uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("BLE NUS connected");
            err_code = app_timer_stop(m_blink_ble);
            APP_ERROR_CHECK(err_code);
            bsp_board_led_on(LED_BLE_NUS_CONN);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            
            readyToSend = true;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("BLE NUS disconnected");
            // LED indication will be changed when advertising starts.
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported.
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
        {
            ble_gap_data_length_params_t dl_params;

            // Clearing the struct will effectively set members to @ref BLE_GAP_DATA_LENGTH_AUTO.
            memset(&dl_params, 0, sizeof(ble_gap_data_length_params_t));
            err_code = sd_ble_gap_data_length_update(p_ble_evt->evt.gap_evt.conn_handle, &dl_params, NULL);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(p_ble_evt->evt.gattc_evt.conn_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
        {
            ble_gatts_evt_rw_authorize_request_t  req;
            ble_gatts_rw_authorize_reply_params_t auth_reply;

            req = p_ble_evt->evt.gatts_evt.params.authorize_request;

            if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
            {
                if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ)     ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                {
                    if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    }
                    else
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                               &auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
        } break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST

        default:
            // No implementation needed.
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////


/**
 * @brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void) {

    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}
////////////////////////////////////////////////////////////////////////////////


/** @brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt) {

    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}
////////////////////////////////////////////////////////////////////////////////


/** @brief Function for initializing the GATT library. */
void gatt_init(void) {

    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}
////////////////////////////////////////////////////////////////////////////////


/**
 * @brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event) {

    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////


/** @brief Function for initializing the Advertising functionality. */
static void advertising_init(void) {

    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}
////////////////////////////////////////////////////////////////////////////////


/** @brief Function for initializing buttons and LEDs. */
static void buttons_leds_init(void) {

    uint32_t err_code = bsp_init(BSP_INIT_LEDS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

//#ifdef SPIRO	
	nrf_gpio_cfg_output(TEST_PIN);
//	nrf_gpio_cfg_output(TEST2_PIN);
	nrf_gpio_cfg_output(LED_R);
	nrf_gpio_cfg_output(LED_G);
	nrf_gpio_cfg_output(LED_B);
	nrf_gpio_cfg_output(LED_IR);
	

	LED_R_ON;
	nrf_delay_ms(400);
	LED_G_ON;
	nrf_delay_ms(400);
	LED_B_ON;
	nrf_delay_ms(400);
	LED_OFF;
	nrf_delay_ms(300);
	
}


/** @brief Function for initializing the nrf_log module. */
static void log_init(void) {

    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}
////////////////////////////////////////////////////////////////////////////////


/** @brief Function for placing the application in low power state while waiting for events. */
static void power_manage(void) {

    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}
////////////////////////////////////////////////////////////////////////////////


/**
 * @brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void) {
	
    UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
    power_manage();
}
////////////////////////////////////////////////////////////////////////////////


// USB CODE START
static bool m_usb_connected = false;
/** @brief User event handler @ref app_usbd_cdc_acm_user_ev_handler_t */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event) {

    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            /*Set up the first transfer*/
            ret_code_t ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                                   m_cdc_data_array,
                                                   1);
            UNUSED_VARIABLE(ret);
            ret = app_timer_stop(m_blink_cdc);
            APP_ERROR_CHECK(ret);
            bsp_board_led_on(LED_CDC_ACM_CONN);
            NRF_LOG_INFO("CDC ACM port opened");
            
            readyToSend = true;
            break;
        }

        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            NRF_LOG_INFO("CDC ACM port closed");
            if (m_usb_connected)
            {
                ret_code_t ret = app_timer_start(m_blink_cdc,
                                                 APP_TIMER_TICKS(LED_BLINK_INTERVAL),
                                                 (void *) LED_CDC_ACM_CONN);
                APP_ERROR_CHECK(ret);
            }
            break;

        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
			readyToSend = true;
            break;

        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
			{ // '{' need it for vars declaration inside
            ret_code_t ret;
			size_t size = 0;
            do {
				size++;
				rx_buffer_fifo[inRxBufferIndex] = m_cdc_data_array[0];
				inRxBufferIndex = (inRxBufferIndex+1)&(RX_BUFFER_SIZE-1);
				rxBufferLength++;

                // Fetch data until internal buffer is empty
                ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, &m_cdc_data_array[0], 1);
            } while (ret == NRF_SUCCESS);
			SendDataToHostFn = channelWriteUsb;

			NRF_LOG_INFO("Usb got bytes: %lu ", size);
            break;
		}
        default:
            break;
    }
}
/////////////////////////////////////////////////////////////////////////////


static void usbd_user_ev_handler(app_usbd_event_type_t event) {

	switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            break;

        case APP_USBD_EVT_DRV_RESUME:
            break;

        case APP_USBD_EVT_STARTED:
            break;

        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;

        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");

            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;

        case APP_USBD_EVT_POWER_REMOVED:
        {
            NRF_LOG_INFO("USB power removed");
            ret_code_t err_code = app_timer_stop(m_blink_cdc);
            APP_ERROR_CHECK(err_code);
            bsp_board_led_off(LED_CDC_ACM_CONN);
            m_usb_connected = false;
            app_usbd_stop();
        }
            break;

        case APP_USBD_EVT_POWER_READY:
        {
            NRF_LOG_INFO("USB ready");
            ret_code_t err_code = app_timer_start(m_blink_cdc,
                                                  APP_TIMER_TICKS(LED_BLINK_INTERVAL),
                                                  (void *) LED_CDC_ACM_CONN);
            APP_ERROR_CHECK(err_code);
            m_usb_connected = true;
            app_usbd_start();
        }
            break;

        default:
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////


void channelWriteUsb(char*data, uint16_t dataLength) {

	app_usbd_cdc_acm_write(&m_app_cdc_acm, data, dataLength);
	NRF_LOG_INFO("USB channelWriteUsb bytes count: %d", dataLength);
	readyToSend = false;
}
////////////////////////////////////////////////////////////////////////////////
// USB CODE END

void channelWriteBle(char* data, uint16_t dataLength) {

	uint16_t length = dataLength;
	if ( length > BLE_MAX_SEND_LENGTH ) length = BLE_MAX_SEND_LENGTH;
	
	ble_nus_data_send(&m_nus, (uint8_t*)data, &length, m_conn_handle);
	NRF_LOG_INFO("BLE channelWriteBle bytes count: %d", length);
	readyToSend = false; // "true" will be in the ble_nus_on_ble_evt()
}
////////////////////////////////////////////////////////////////////////////////


void checkIncommingData() {
	
	while ( rxBufferLength > 0 ) {
		addIncomingData(rx_buffer_fifo[outRxBufferIndex]);
		outRxBufferIndex = (outRxBufferIndex+1) & (RX_BUFFER_SIZE-1);
		rxBufferLength--;
	}
}
////////////////////////////////////////////////////////////////////////////////


void checkOutStream() {
	
	if ( !readyToSend ) return;
	int count = 0;
	while ( outTxBufferIndex != inTxBufferIndex ) {
	
		send_buffer[count] = tx_buffer_fifo[outTxBufferIndex];
		outTxBufferIndex = (outTxBufferIndex+1) & (TX_BUFFER_SIZE-1);
		count++;
		txBufferLength++;
		if (count >= BLE_MAX_SEND_LENGTH ) break;
	}
	
	if ( count > 0 && SendDataToHostFn != NULL ) {
//		TEST_ON;
		SendDataToHostFn(send_buffer, count);
//		TEST_OFF;
	}
}
////////////////////////////////////////////////////////////////////////////////


void addDataToOutputQueue(char*data, uint16_t dataLength) {

	for ( int count = 0; count < dataLength; count++) {
		tx_buffer_fifo[inTxBufferIndex] = data[count];
		inTxBufferIndex = (inTxBufferIndex+1) & (TX_BUFFER_SIZE-1);
		txBufferLength--;
	}
}
////////////////////////////////////////////////////////////////////////////////


void initRxBuffer() {

	inRxBufferIndex = 0;
	outRxBufferIndex = 0;
	rxBufferLength = 0;
	
	inTxBufferIndex = 0;
	outTxBufferIndex = 0;
	txBufferLength = 0;
}
////////////////////////////////////////////////////////////////////////////////


void showError(uint16_t code) {

	LED_OFF;
	while (1) {
		for (int i = 0; i < code; i++) {
			LED_R_ON;
			nrf_delay_ms(200);
			LED_R_OFF;
			nrf_delay_ms(200);
		}
		nrf_delay_ms(1000);
	}
}
////////////////////////////////////////////////////////////////////////////////


#define _DEBUG
#ifdef _DEBUG
#include "../ecg_sensor/ads129x/ads129x.h"
#endif

/** @brief Application main function. */
int main(void) {

	nrf_gpio_cfg_output(TEST_PIN);
	nrf_gpio_cfg_output(TEST2_PIN);
	nrf_gpio_cfg_output(ADS_PWDN);

	nrf_delay_ms(200); 
	accDebug();

	ADS_PWDN_ON;

	// common initialization
    log_init();
    timers_init();
    buttons_leds_init();

	ret_code_t ret;
    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);

	uint16_t errorCode = 0;
	if ( !adcTestChip() ) errorCode++;
	if ( errorCode ) showError(errorCode);
	
//	debugAdsChip();
	
	// init USB
    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler
    };
    app_usbd_serial_num_generate();
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);
    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);
    ret = app_usbd_power_events_enable();
    APP_ERROR_CHECK(ret);
	// end for USB

	// ble init
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();
    advertising_start();
	// end for ble

	// app init
	channelWriteFn = addDataToOutputQueue;

	memset(firmware_version, 0, sizeof(firmware_version));
	sprintf(&firmware_version[0], "%s %s", version_name, __DATE__);
	initRxBuffer();
	timestampTimerInit();
	hostInterfaseInit();

    // Enter main loop.
    NRF_LOG_INFO("USBD BLE UART example started.");
	printf("\nUSBD BLE UART example started.\n");
	
	// usb on start
    app_usbd_enable();
	m_usb_connected = true;
	app_usbd_start();
	SendDataToHostFn = channelWriteUsb;
	
	
	bool boardOk = adcTestChip();
	
    for (;;) {
		
		checkIncommingData();
		checkOutStream();
		
        while (app_usbd_event_queue_process()) {
            /* Nothing to do */
        }
        
        idle_state_handle();
    }
}

/**
 * @}
 */
