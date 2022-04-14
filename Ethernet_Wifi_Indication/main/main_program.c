#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <stddef.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"

#include "mqtt_client.h"
#include "driver/uart.h"

#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include <netdb.h>

#include "nvs.h"
#include "nvs_flash.h"

#include <sys/time.h>
#include <time.h>
#include "esp_sntp.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include <driver/i2c.h>
#include "esp_wifi.h"

// #include "enc28j60.h"
#include "esp_eth_enc28j60.h"
#include "driver/spi_master.h"  

#include <stdbool.h>
#include <esp_system.h>
#include "led_strip.h"

#define FW_version 24
#define mqttbroker "mqtt://104.211.188.23:5131"

//.............wifi declaration.....................
#define EXAMPLE_WIFI_SSID "ALITER"
#define EXAMPLE_WIFI_PASS "Aliter@256"
#define EXAMPLE_MAXIMUM_RETRY 2
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

bool wifi_con_flag = false;
bool eth_con_flag = false;

//----------------------------------
int status_nvs = 0;
static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
char *nvs_ssid;
char *nvs_passwd;
char *nvs_sta_ip = "192.168.0.250";
char *nvs_sta_gw = "192.168.0.1";
char *nvs_sta_snt = "255.255.255.0";
char *nvs_sta_dns1 = "192.168.0.1";
char *nvs_sta_dns2 = "8.8.8.8";
char *nvs_sta_dhcp = "1";

char *nvs_sta_eip = "192.168.0.250";
char *nvs_sta_egw = "192.168.0.1";
char *nvs_sta_esnt = "255.255.255.0";
char *nvs_sta_edns1 = "192.168.0.1";
char *nvs_sta_edns2 = "8.8.8.8";
char *nvs_sta_edhcp = "1";

char *mqtt_url = "mqtt://104.211.188.23:5131";
int8_t rssi;

void cmd_parser(const char *data);

static const char *TAG = "gateway";
static const int RX_BUF_SIZE = 1024;
int mqtt_con_flag = 0;
esp_mqtt_client_handle_t client;
int restart_flag = 0;

//-----------------------------------------------------------------
char data_topic[50];
char cmd_topic[50];
char logs_topic[50];

//-----------------------------------------------------------------
/* OTA init*/
#define HASH_LEN 32
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
// char  *Server_Cmd;
char Ota_Cmd[20] = "{\"START_OTA\"}";
char nvs_cmd[20] = "{\"ERASE_NVS\"}";
char reboot_cmd[20] = "{\"REBOOT\"}";
char Ota_Url[200] = "https://beta.alitersolutions.com/demo/images/OTA/simple_ota.bin";
int count1 = 12;
int count2 = 0;
char string2[200];
int logid1;
int content_len=0;
TaskHandle_t xHandle_button_read;
TaskHandle_t xHandle_gpio_task;

////////////////////////////////////////////////////////////////////////RGB DECLARATIONS////////////////////////////////////////////////////////////

const gpio_num_t LED_CLK = 18; // CONFIG_TM1637_CLK_PIN;(18)
const gpio_num_t LED_DTA = 19; // CONFIG_TM1637_DIO_PIN;(19)


#define LED_STRIP_LENGTH 1U
#define LED_STRIP_RMT_INTR_NUM 19U

static struct led_color_t led_strip_buf_1[LED_STRIP_LENGTH];
static struct led_color_t led_strip_buf_2[LED_STRIP_LENGTH];
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct led_strip_t led_strip = { //++ Structure to Initialise RGB
    .rgb_led_type = RGB_LED_TYPE_WS2812,
    .rmt_channel = RMT_CHANNEL_1,
    .rmt_interrupt_num = LED_STRIP_RMT_INTR_NUM,
    .gpio = GPIO_NUM_5,
    .led_strip_buf_1 = led_strip_buf_1,
    .led_strip_buf_2 = led_strip_buf_2,
    .led_strip_length = LED_STRIP_LENGTH};

//-----------------------------------------------------------------------------
#define OTA_URL_SIZE 256
#define Uart_0_TXD_PIN (GPIO_NUM_1)
#define Uart_0_RXD_PIN (GPIO_NUM_3)
#define TXD_PIN (GPIO_NUM_13)
#define RXD_PIN (GPIO_NUM_12)
//--------------------------------------------------------------------------------------------

#define WIFI_LED 26
#define ETH_LED 27
char received_ts[100];
char dev_speci[250];
uint8_t base_mac_addr[6] = {0};
char mac_json[40];

//----------------------Static Wifi-------------------------------------
static esp_err_t example_set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE))
    {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        esp_netif_set_dns_info(netif, type, &dns);
    }
    return ESP_OK;
}
static void example_set_static_ip(esp_netif_t *netif)
{
    if (strcmp(nvs_sta_dhcp, "1") == 0)
    {
        ESP_LOGI(TAG, "Dhcp IP \n ");
        if (esp_netif_dhcpc_start(netif) != ESP_OK)
        {
            ESP_LOGI(TAG, "Failed to stop dhcp client");
            return;
        }
    }

    if (strcmp(nvs_sta_dhcp, "0") == 0)
    {
        ESP_LOGI(TAG, "Static IP \n ");
        if (esp_netif_dhcpc_stop(netif) != ESP_OK)
        {
            ESP_LOGI(TAG, "Failed to stop dhcp client");
            return;
        }

        esp_netif_ip_info_t info_t;
        memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
        info_t.ip.addr = esp_ip4addr_aton((const char *)nvs_sta_ip);
        info_t.gw.addr = esp_ip4addr_aton((const char *)nvs_sta_gw);
        info_t.netmask.addr = esp_ip4addr_aton((const char *)nvs_sta_snt);
        esp_netif_set_ip_info(netif, &info_t);

        ESP_LOGI(TAG, "Success to set static ip: %s, netmask: %s, gw: %s", nvs_sta_ip, nvs_sta_snt, nvs_sta_gw);
        ESP_ERROR_CHECK(example_set_dns_server(netif, ipaddr_addr(nvs_sta_dns1), ESP_NETIF_DNS_MAIN));
        ESP_ERROR_CHECK(example_set_dns_server(netif, ipaddr_addr(nvs_sta_dns2), ESP_NETIF_DNS_BACKUP));
    }
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        example_set_static_ip(arg);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        example_set_static_ip(arg);
        if (s_retry_num < EXAMPLE_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
        }
        else
        {
            gpio_set_level(WIFI_LED,0);
            wifi_con_flag = false;
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        gpio_set_level(WIFI_LED,1);
        wifi_con_flag = true;
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void wifi_init_sta(void)
{

    s_wifi_event_group = xEventGroupCreate();

    esp_netif_init();

    esp_event_loop_create_default();

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &event_handler,
                                        sta_netif,
                                        &instance_any_id);

    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &event_handler,
                                        sta_netif,
                                        &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            // .ssid = EXAMPLE_WIFI_SSID,
            //.password = EXAMPLE_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };

    strcpy((char *)wifi_config.sta.ssid, (char *)nvs_ssid);
    strcpy((char *)wifi_config.sta.password, (char *)nvs_passwd);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 nvs_ssid, nvs_passwd);
        // log_wifi = 1;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 nvs_ssid, nvs_passwd);
        // log_wifi = 0;
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    vEventGroupDelete(s_wifi_event_group);
}

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
            
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        gpio_set_level(ETH_LED,0);
        eth_con_flag = false;
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    gpio_set_level(ETH_LED,1);
    eth_con_flag = true;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    
}

void ethernet_init_sta()
{
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);
   
   spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_EXAMPLE_ENC28J60_MISO_GPIO,
        .mosi_io_num = CONFIG_EXAMPLE_ENC28J60_MOSI_GPIO,
        .sclk_io_num = CONFIG_EXAMPLE_ENC28J60_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
   
     ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_EXAMPLE_ENC28J60_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    /* ENC28J60 ethernet driver is based on spi driver */
    spi_device_interface_config_t devcfg = {
        .command_bits = 3,
        .address_bits = 5,
        .mode = 0,
        .clock_speed_hz = CONFIG_EXAMPLE_ENC28J60_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num = CONFIG_EXAMPLE_ENC28J60_CS_GPIO,
        .queue_size = 20,
        .cs_ena_posttrans = enc28j60_cal_spi_cs_hold_time(CONFIG_EXAMPLE_ENC28J60_SPI_CLOCK_MHZ),
    };

   spi_device_handle_t spi_handle = NULL;
    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_EXAMPLE_ENC28J60_SPI_HOST, &devcfg, &spi_handle));

     eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(spi_handle);
    enc28j60_config.int_gpio_num = CONFIG_EXAMPLE_ENC28J60_INT_GPIO;

      eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.smi_mdc_gpio_num = -1;  // ENC28J60 doesn't have SMI interface
    mac_config.smi_mdio_gpio_num = -1;
    esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);
   
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
    phy_config.reset_gpio_num = -1; // ENC28J60 doesn't have a pin to reset internal PHY
    esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

     esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

    /* ENC28J60 doesn't burn any factory MAC address, we need to set it manually.
       02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    */
       mac->set_addr(mac, (uint8_t[]) {
        //0x02, 0x00, 0x00, 0x12, 0x34, 0x56
        base_mac_addr[0], base_mac_addr[1], base_mac_addr[2], base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]
    });
   
   // ENC28J60 Errata #1 check
    if (emac_enc28j60_get_chip_info(mac) < ENC28J60_REV_B5 && CONFIG_EXAMPLE_ENC28J60_SPI_CLOCK_MHZ < 8) {
        ESP_LOGE(TAG, "SPI frequency must be at least 8 MHz for chip revision less than 5");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
   
   
    // Set default handlers to process TCP/IP stuffs
   ESP_ERROR_CHECK(esp_eth_set_default_handlers(eth_netif));
    //-------------------------------------------------------------------------------------------------------
   
   if(strcmp(nvs_sta_edhcp,"0")==0)
   {
      // printf(" Trueee \n");
   
   
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif));
    // char* ip= "192.168.1.251";
    // char* gateway = "192.168.1.1";
    // char* netmask = "255.255.255.0";
    // char* dns = "8.8.8.8";

    esp_netif_ip_info_t info_t;
    //esp_netif_dns_info_t dns_info;
    memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
    info_t.ip.addr = esp_ip4addr_aton((const char *)nvs_sta_eip);
    info_t.gw.addr = esp_ip4addr_aton((const char *)nvs_sta_egw);
    info_t.netmask.addr = esp_ip4addr_aton((const char *) nvs_sta_esnt);
    // dns_info.ip.u_addr = esp_ip4addr_aton((const char *)dns);
    // IP_ADDR4(&dns_info.ip, 208, 91, 112, 53);

    esp_netif_set_ip_info(eth_netif, &info_t); 

        // ESP_LOGI(TAG, "Success to set static ip: %s, netmask: %s, gw: %s", nvs_sta_eip, nvs_sta_egw, nvs_sta_esnt);
        ESP_ERROR_CHECK(example_set_dns_server(eth_netif, ipaddr_addr(nvs_sta_edns1), ESP_NETIF_DNS_MAIN));
        ESP_ERROR_CHECK(example_set_dns_server(eth_netif, ipaddr_addr(nvs_sta_edns2), ESP_NETIF_DNS_BACKUP));
    // esp_netif_set_dns_info(eth_netif, esp_ip4addr_aton(nvs_sta_edns1), &dns_info);
    //    esp_netif_set_dns_info(eth_netif, nvs_sta_edns2, &dns_info);
    //    ESP_ERROR_CHECK(esp_eth_set_default_handlers(eth_netif));
   }
    //-------------------------------------------------------------------------------------------------------

    
    /* attach Ethernet driver to TCP/IP stack */
    // ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle));
   
   
    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    /* start Ethernet driver state machine */
    esp_eth_start(eth_handle);
    enc28j60_set_phy_duplex(phy, ETH_DUPLEX_FULL);

}

//--------------------------------------------------------------------------------

/* OTA loops */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
         content_len = esp_http_client_get_content_length(evt->client);
        
    }
    return ESP_OK;
}

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

#ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /**
     * Secure version check from firmware image header prevents subsequent download and flash write of
     * entire firmware image. However this is optional because it is also taken care in API
     * esp_https_ota_finish at the end of OTA update procedure.
     */
    const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    if (new_app_info->secure_version < hw_sec_version) {
        ESP_LOGW(TAG, "New firmware security version is less than eFuse programmed, %d < %d", new_app_info->secure_version, hw_sec_version);
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}

// void simple_ota_example_task()
void ota_task()
// void advanced_ota_example_task(void *pvParameter)
{
    vTaskDelete(xHandle_button_read);
    vTaskDelete(xHandle_gpio_task);
    
    ESP_LOGI(TAG, "Starting OTA example");
    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url=Ota_Url,
        .cert_pem = (char *)server_cert_pem_start,
        .event_handler = _http_event_handler,
        .timeout_ms =5000,
        .keep_alive_enable = true,
    };
#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

        esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
#ifdef CONFIG_EXAMPLE_ENABLE_PARTIAL_HTTP_DOWNLOAD
        .partial_http_download = true,
        .max_http_request_size = CONFIG_EXAMPLE_HTTP_REQUEST_SIZE,
#endif
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    // printf("Total size  %d \n",esp_https_ota_get_image_size(https_ota_handle));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
 goto ota_end;
    }
// printf("Total size  %d \n",content_len);
    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "image header verification failed");
        goto ota_end;
    }
    int old_percentage=0;
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        int current_len=esp_https_ota_get_image_len_read(https_ota_handle);
        // ESP_LOGI(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
        int current_percentage =current_len*100/content_len;
        if(current_percentage!=old_percentage)
        { 
            old_percentage=current_percentage;
            ESP_LOGW("OTA"," Downloading...( %d %%) \n",old_percentage);
        }
        
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(TAG, "Complete data was not received.");
         goto ota_end;
    } else {
        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
            ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
                 goto ota_end;
             
            }
            ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
             goto ota_end;

        }
    }

ota_end:
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
             for (int i = 3; i > 0; i--) 
            {
                ESP_LOGE("OTA FAILED","Restarting in %d seconds...\n", i);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            ESP_LOGI("OTA FAILED","Restarting now.\n");
            fflush(stdout);
           esp_restart();
    
        vTaskDelete(NULL);

}

//----------uart intialization------------------------------------------------------------------------------------
void uart_0_init()
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, Uart_0_TXD_PIN, Uart_0_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

/*Start UART Operation*/
static void uart0_rx_task()
{
    static const char *RX_TASK_TAG = "uart_0_rx_task";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1)
     {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0)
          {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            //ESP_LOGI(RX_TASK_TAG,"received data :\t %s \n" ,data);
            cmd_parser((char*)data);
           }

           wifi_ap_record_t wifidata;
           esp_wifi_sta_get_ap_info(&wifidata);
           rssi= wifidata.rssi;

           if(rssi == 0 && status_nvs == 1)
           {
               gpio_set_level(WIFI_LED,0);
               esp_wifi_connect();
           }
           else if((rssi < 0 && rssi > -100) && status_nvs == 1)
           {
               gpio_set_level(WIFI_LED,1);
               wifi_con_flag = true;
           }
        
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    free(data);
}

char dataString[200];
void substr(const char *src1, int m, int n)
{
    // get the length of the destination string
    int len = n - m;

    // allocate (len + 1) chars for destination (+1 for extra null character)
    char *dest = (char *)malloc(sizeof(char) * (len + 1));

    // extracts characters between m'th and n'th index from source string
    // and copy them into the destination string
    for (int i = m; i < n && (*(src1 + i) != '\0'); i++)
    {
        *dest = *(src1 + i);
        dest++;
    }

    // null-terminate the destination string
    *dest = '\0';

    // return the destination string
    strcpy(dataString, dest - len);
    // 	return dest - len;
}

void cmd_parser(const char *data)
{
    int len1 = strcspn(data, "=");
    int len2;
    char src[200], check_cmd[20];
    substr(data, 0, len1);
    strcpy(check_cmd, dataString);

    if(strcmp(check_cmd, "NETWORK") == 0)
    {
        nvs_handle_t my_handle;
        esp_err_t err;

        int index = 9;
        int count = 0;
        char network[50];
        uint8_t status_received = 0;
        memset(network,0,strlen(network));

        while(data[index] != '\"')
        {
            network[count] = data[index];
            index++;
            count++;
        }

        printf("%s\n", network);

        if(strcmp(network, "WIFI") == 0)
        {
            status_received = 1;
        }
        else if(strcmp(network, "ETHERNET") == 0)
        {
            status_received = 0;
        }

        printf("NETWORK SET : %d\n", status_received);

        err = nvs_open("NETWORK", NVS_READWRITE, &my_handle);
        if (err != ESP_OK)
        {
            printf("Error (%s) opening NETWORK NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            // printf("nvs open\n");
            err = nvs_set_u8(my_handle, "status", status_received);
            printf((err != ESP_OK) ? "status Failed!\n" : "status save\n");
            nvs_close(my_handle);
        }

        vTaskDelay(500/portTICK_PERIOD_MS);

        esp_restart();
    }
    
    else if (strcmp(check_cmd, "WIFI") == 0)
    {
        nvs_handle_t my_handle;
        esp_err_t err;

        // ESP_LOGI(TAG, "Setting up Wifi ");
        char ssidData[100], passData[100], dhcpData[5];
        substr(data, len1 + 1, strlen(data));
        strcpy(src, dataString);
        len1 = strcspn(src, ":");
        substr(src, 0, len1);
        strcpy(check_cmd, dataString);
        // printf( "check_cmd = %s\n", check_cmd );
        len2 = strcspn(src, ",");
        if (strcmp(check_cmd, "SSID") == 0)
        {
            substr(src, len1 + 2, len2 - 1);
            strcpy(ssidData, dataString);
            printf("SSID: %s\n", ssidData);
        }
        //--------------------------------------------------------------------------------------------------
        substr(src, len2, strlen(src));
        strcpy(src, dataString);
        len1 = strcspn(src, ":");
        substr(src, 1, len1);
        strcpy(check_cmd, dataString);

        // printf( "check_cmd = %s\n", check_cmd );
        if (strcmp(check_cmd, "PASS") == 0)
        {
            // len1=strcspn (src,":\"");
            substr(src, len1 + 2, strlen(src));
            strcpy(passData, dataString);
            len2 = strcspn(passData, "\"");
            substr(passData, 0, len2);
            strcpy(passData, dataString);
            printf("PASS: %s\n", passData);
            // printf( "check_cmd = %s\n", src );
        }

        substr(src, len2 + 2, strlen(src));
        strcpy(src, dataString);
        len2 = strcspn(src, ","); // printf("len 2 %d  \n",len2);
        substr(src, len2, strlen(src));
        strcpy(src, dataString);
        len1 = strcspn(src, ":");
        substr(src, 1, len1);
        strcpy(check_cmd, dataString);
        if (strcmp(check_cmd, "DHCP") == 0)
        {
            substr(src, len1 + 2, strlen(src));
            strcpy(dhcpData, dataString);
            len2 = strcspn(dhcpData, "\"");
            substr(dhcpData, 0, len2);
            strcpy(dhcpData, dataString);
            printf("dhcpData: %s\n", dhcpData);

            err = nvs_open("wifi", NVS_READWRITE, &my_handle);
            if (err != ESP_OK)
            {
                // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
            }
            else
            {
                // printf("nvs open\n");
                err = nvs_set_str(my_handle, "ssid", ssidData);
                // printf((err != ESP_OK) ? "ssid Failed!\n" : "ssid save\n");

                err = nvs_set_str(my_handle, "passwd", passData);
                // printf((err != ESP_OK) ? "Failed!\n" : "passwd save\n");
                err = nvs_set_str(my_handle, "dhcp", dhcpData);
                // ESP_LOGI(TAG,"Committing updates in NVS ... ");
                err = nvs_commit(my_handle);
                // printf((err != ESP_OK) ? "Failed!\n" : "commit\n");
                nvs_close(my_handle);
            }
        }
    }

    else if (strcmp(check_cmd, "WSTAIP") == 0)
    {
        nvs_handle_t my_handle;
        esp_err_t err;

        char sta_ip[100], sta_gw[100], sta_snt[100], sta_dns1[100], sta_dns2[100];

        substr(data, len1 + 1, strlen(data));
        strncpy(src, dataString, sizeof(src));
        len1 = strcspn(src, ":");
        substr(src, 0, len1);
        strcpy(check_cmd, dataString);
        len2 = strcspn(src, ",");

        if (strcmp(check_cmd, "IP") == 0)
        {
            substr(src, len1 + 2, len2 - 1);
            strcpy(sta_ip, dataString);
            printf("IP: %s\n", sta_ip);
        }

        substr(src, len2, strlen(src));
        strcpy(src, dataString);
        len1 = strcspn(src, ":");
        substr(src, 1, len1);
        strcpy(check_cmd, dataString);
        // printf( "check_cmd = %s\n", check_cmd );

        if (strcmp(check_cmd, "GW") == 0)
        {
            substr(src, len1 + 2, strlen(src));
            strncpy(sta_gw, dataString, sizeof(sta_gw));
            len2 = strcspn(sta_gw, "\"");
            substr(sta_gw, 0, len2);
            strncpy(sta_gw, dataString, sizeof(sta_gw));
            printf("GW: %s\n", sta_gw);
        }

        // printf("GW1: %s\n",sta_gw);

        len2 = strcspn(src, ",");
        substr(src, len2 + 1, strlen(src));
        strcpy(src, dataString);
        len2 = strcspn(src, ",");
        substr(src, len2 + 1, strlen(src));
        strcpy(src, dataString);
        len1 = strcspn(src, ":");
        substr(src, 0, len1);
        strcpy(check_cmd, dataString);

        if (strcmp(check_cmd, "SNT") == 0)
        {
            substr(src, len1 + 2, strlen(src));
            strcpy(sta_snt, dataString);
            len2 = strcspn(sta_snt, "\"");
            substr(sta_snt, 0, len2);
            strcpy(sta_snt, dataString);
            printf("SNT: %s\n", sta_snt);

            // printf("GW2: %s\n",sta_gw);
        }

        len2 = strcspn(src, ",");
        substr(src, len2 + 1, strlen(src));
        strcpy(src, dataString);
        len1 = strcspn(src, ":");
        substr(src, 0, len1);
        strcpy(check_cmd, dataString);
        // printf( "check_cmd = %s\n", check_cmd );
        // printf("%s\n",src);

        if (strcmp(check_cmd, "DNS1") == 0)
        {
            substr(src, len1 + 2, strlen(src));
            strcpy(sta_dns1, dataString);
            len2 = strcspn(sta_dns1, "\"");
            substr(sta_dns1, 0, len2);
            strcpy(sta_dns1, dataString);
            printf("DNS1: %s\n", sta_dns1);
            // printf("GW3: %s\n",sta_gw);
        }

        len2 = strcspn(src, ",");
        substr(src, len2 + 1, strlen(src));
        strcpy(src, dataString);
        len1 = strcspn(src, ":");
        substr(src, 0, len1);
        strcpy(check_cmd, dataString);
        // printf( "check_cmd = %s\n", check_cmd );

        if (strcmp(check_cmd, "DNS2") == 0)
        {
            substr(src, len1 + 2, strlen(src));
            strcpy(sta_dns2, dataString);
            len2 = strcspn(sta_dns2, "\"");
            substr(sta_dns2, 0, len2);
            strcpy(sta_dns2, dataString);
            printf("DNS2: %s\n", sta_dns2);

            // printf("GW4: %s\n",sta_gw);
        }

        err = nvs_open("wifi", NVS_READWRITE, &my_handle);

        if (err != ESP_OK)
        {
            // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {

            err = nvs_set_str(my_handle, "sta_ip", sta_ip);

            err = nvs_set_str(my_handle, "sta_gw", sta_gw);

            err = nvs_set_str(my_handle, "sta_snt", sta_snt);

            err = nvs_set_str(my_handle, "sta_dns1", sta_dns1);

            err = nvs_set_str(my_handle, "sta_dns2", sta_dns2);

            err = nvs_commit(my_handle);

            nvs_close(my_handle);
        }
    }

       //--------------------ETHERNET---------------------------------
    
    else  if(strcmp(check_cmd,"ETH")==0)
    {
        
        char dhcpData[5];
        substr(data,len1+1,strlen(data));strcpy(src,dataString);
        len1=strcspn (src,":");
        substr(src,0,len1);strcpy(check_cmd,dataString);
      
        if(strcmp(check_cmd,"DHCP")==0)
        {
            nvs_handle_t my_handle;
            esp_err_t err;

            //   printf("Check command %s \n", check_cmd);
            substr(src,len1+2,strlen(src)); strcpy(dhcpData, dataString);
            len2=strcspn (dhcpData,"\"");
            substr(dhcpData,0,len2); strcpy(dhcpData, dataString);
            printf("dhcpData: %s\n",dhcpData);


             err = nvs_open("ethernet", NVS_READWRITE, &my_handle);
                
                if (err != ESP_OK) {
                //printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
                } else {
                //printf("nvs open\n");
                //printf("GW_nvs: %s\n",sta_gw);

                 //printf("sta_ip  %s\n",sta_ip);
                err = nvs_set_str(my_handle, "dhcp", dhcpData);

                // ESP_LOGI(TAG,"Committing updates in NVS ... ");
                err = nvs_commit(my_handle);
                //printf((err != ESP_OK) ? "Failed!\n" : "commit Done\n");
                 nvs_close(my_handle);
                }
        }

    }

    else  if(strcmp(check_cmd,"ESTAIP")==0)
    {
        nvs_handle_t my_handle;
        esp_err_t err;
          
        char sta_ip[100],sta_gw[100],sta_snt[100],sta_dns1[100],sta_dns2[100];

        substr(data,len1+1,strlen(data));strncpy(src,dataString,sizeof(src));
        len1=strcspn (src,":");
        substr(src,0,len1);strcpy(check_cmd,dataString);
        len2=strcspn (src,",");

        if(strcmp(check_cmd,"IP")==0)
         {
             substr(src,len1+2,len2-1); strcpy(sta_ip, dataString);
            printf("ETHERNET IP: %s\n",sta_ip);
         }

        substr(src,len2,strlen(src));strcpy(src,dataString);
        len1=strcspn (src,":");
        substr(src,1,len1);strcpy(check_cmd,dataString);
        // printf( "check_cmd = %s\n", check_cmd );

         if(strcmp(check_cmd,"GW")==0)
         {
                substr(src,len1+2,strlen(src)); strncpy(sta_gw, dataString,sizeof(sta_gw));
                len2=strcspn (sta_gw,"\"");
                substr(sta_gw,0,len2);strncpy(sta_gw, dataString,sizeof(sta_gw));
                printf("ETHERNET GW: %s\n",sta_gw);
                
         }

         //printf("GW1: %s\n",sta_gw);

        len2=strcspn (src,",");
        substr(src,len2+1,strlen(src));strcpy(src,dataString);
        len2=strcspn (src,",");
        substr(src,len2+1,strlen(src));strcpy(src,dataString);
        len1=strcspn (src,":");
        substr(src,0,len1);strcpy(check_cmd,dataString);

          if(strcmp(check_cmd,"SNT")==0)
         {
             substr(src,len1+2,strlen(src)); strcpy(sta_snt, dataString);
             len2=strcspn (sta_snt,"\"");
             substr(sta_snt,0,len2); strcpy(sta_snt, dataString);
            printf("ETHERNET SNT: %s\n",sta_snt);  
                //printf("GW2: %s\n",sta_gw);
         }

        len2=strcspn (src,",");
        substr(src,len2+1,strlen(src));strcpy(src,dataString);
        len1=strcspn (src,":");
        substr(src,0,len1);strcpy(check_cmd,dataString);
        // printf( "check_cmd = %s\n", check_cmd );
        // printf("%s\n",src);

         if(strcmp(check_cmd,"DNS1")==0)
         {
             substr(src,len1+2,strlen(src)); strcpy(sta_dns1, dataString);
             len2=strcspn (sta_dns1,"\"");
             substr(sta_dns1,0,len2); strcpy(sta_dns1, dataString);
             printf("ETHERNET DNS1: %s\n",sta_dns1);
                //printf("GW3: %s\n",sta_gw);
         }

        len2=strcspn (src,",");
        substr(src,len2+1,strlen(src));strcpy(src,dataString);
        len1=strcspn (src,":");
        substr(src,0,len1);strcpy(check_cmd,dataString);
        // printf( "check_cmd = %s\n", check_cmd );

        if(strcmp(check_cmd,"DNS2")==0)
         {
             substr(src,len1+2,strlen(src)); strcpy(sta_dns2, dataString);
             len2=strcspn (sta_dns2,"\"");
             substr(sta_dns2,0,len2); strcpy(sta_dns2, dataString);
            printf("ETHERNET DNS2: %s\n",sta_dns2);

            //printf("GW4: %s\n",sta_gw);
                
         }
    
             err = nvs_open("ethernet", NVS_READWRITE, &my_handle);
                
                if (err != ESP_OK) {
                //printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
                } else {
                //printf("nvs open\n");
                //printf("GW_nvs: %s\n",sta_gw);

                 //printf("sta_ip  %s\n",sta_ip);
                err = nvs_set_str(my_handle, "sta_ip", sta_ip);
                //printf((err != ESP_OK) ? "Failed!\n" : "ip save\n");
                
                 //printf("sta_gw  %s\n",sta_gw);
                err = nvs_set_str(my_handle, "sta_gw",sta_gw );
                //printf((err != ESP_OK) ? "Failed!\n" : "sta_gw save\n");

                //printf("sta_snt  %s\n",sta_snt);
                err = nvs_set_str(my_handle, "sta_snt",sta_snt );
                //printf((err != ESP_OK) ? "Failed!\n" : "sbn save\n");

                //printf("sta_dns1  %s\n",sta_dns1);
                err = nvs_set_str(my_handle, "sta_dns1",sta_dns1 );
                //printf((err != ESP_OK) ? "Failed!\n" : "dns1 save\n");

                err = nvs_set_str(my_handle, "sta_dns2",sta_dns2 );
                //printf((err != ESP_OK) ? "Failed!\n" : "dns2 save\n");

               // ESP_LOGI(TAG,"Committing updates in NVS ... ");
                err = nvs_commit(my_handle);
                //printf((err != ESP_OK) ? "Failed!\n" : "commit Done\n");
                 nvs_close(my_handle);
                }

    }

    //------------------mqtt--------------
    else if (strcmp(check_cmd, "MQTT") == 0)
    {
        //  printf("%s \n",data);
        nvs_handle_t my_handle;
        esp_err_t err;

        char mqttData[100];
        substr(data, len1 + 2, strlen(data));
        strcpy(mqttData, dataString);
        len2 = strcspn(mqttData, "\"");
        substr(mqttData, 0, len2);
        strcpy(mqttData, dataString);

        printf("%s \n", mqttData);
        err = nvs_open("mqttData", NVS_READWRITE, &my_handle);

        if (err != ESP_OK)
        {
            // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            err = nvs_set_str(my_handle, "mqtt_url", mqttData);
            err = nvs_commit(my_handle);
            // printf((err != ESP_OK) ? "Failed!\n" : "commit Done\n");
            nvs_close(my_handle);
        }
    }
    //--------------ACTION----------
    else if (strcmp(check_cmd, "ACTION") == 0)
    {
        // printf("%s \n",check_cmd);
        printf("%s \n", data);
        len1 = strcspn(data, "{");
        len2 = strcspn(data, "}");
        substr(data, len1, len2 + 1);
        strcpy(check_cmd, dataString);
        // printf("%s \n",check_cmd);
        if (strcmp(check_cmd, reboot_cmd) == 0)
        {
            for (int i = 3; i > 0; i--)
            {
                ESP_LOGI(TAG, "Restarting in %d seconds...\n", i);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            ESP_LOGI(TAG, "Restarting now.\n");
            fflush(stdout);
            esp_restart();
        }
        if (strcmp(check_cmd, nvs_cmd) == 0)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            esp_err_t err = nvs_flash_init();
            if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
            {
                ESP_ERROR_CHECK(nvs_flash_erase());
                err = nvs_flash_init();
            }
            ESP_ERROR_CHECK(err);
            ESP_LOGI(TAG, "Restarting now.\n");
            fflush(stdout);
            esp_restart();
        }

        if (data[9] == 'O' && data[10] == 'T' && data[11] == 'A')
        {
            // printf("Received OTA URL: %s \n",data);
            // count1 += 7;
            int ota_url_count1 = 19;
            //   int ota_url_count2=0;
            memset(Ota_Url, 0, strlen(Ota_Url));
            memset(string2, 0, strlen(string2));

            while (data[ota_url_count1] != '\"')
            {
                string2[count2] = data[ota_url_count1];
                ota_url_count1++;
                count2++;
            }
            //  count1 += 7;
            count2 = 0;
            strncpy(Ota_Url, string2, sizeof(Ota_Url));
            printf("Ota_Url    %s\n", Ota_Url);

        }

        if (strcmp(check_cmd, Ota_Cmd) == 0)
        {
            ota_task();
        }
    }

    else
    {
        printf("ERROR! Wrong Command");
    }
}

//-------------------------Publish Task-----------------------------------------------------------------

void publish_task()
{
    uint32_t publish_count = 0;
    char count_topic[100];

    while(1)
    {
        if(mqtt_con_flag==1)
        {

            if(status_nvs == 1)
            {
                sprintf(count_topic, "{\"device\":%s,\"COUNT\":\"%d\",WIFI}", mac_json,publish_count);
                logid1 = esp_mqtt_client_publish(client, data_topic, count_topic, 0, 1, 0);
                ESP_LOGI("MQTT", "sent publish successful, on topic %s", data_topic);
            }

            else if(status_nvs == 0)
            {
                sprintf(count_topic, "{\"device\":%s,\"COUNT\":\"%d\",ETHERNET}",mac_json,publish_count);
                logid1 = esp_mqtt_client_publish(client, data_topic, count_topic, 0, 1, 0);
                ESP_LOGI("MQTT", "sent publish successful, on topic %s", data_topic);
            }

            publish_count++;
        }

        vTaskDelay( 2000 / portTICK_PERIOD_MS);
    }
}

void cmd_topic_ISR(char *Server_Cmd)
{
    ESP_LOGI(TAG, " cmd topic ISR JSON: %s\n", Server_Cmd);

    if (Server_Cmd[2] == 't' && Server_Cmd[3] == 's')
    {
        int tscount1 = 7;
        int tscount2 = 0;

        while (Server_Cmd[tscount1] != '\"')
        {
            received_ts[tscount2] = Server_Cmd[tscount1];
            tscount1++;
            tscount2++;
        }
        ESP_LOGI(TAG, "Received ts:%s", received_ts);
    }
    else if (Server_Cmd[2] == 'O' && Server_Cmd[3] == 'T' && Server_Cmd[4] == 'A')
    {
        // printf("Received OTA URL: %s \n",Server_Cmd);
        memset(Ota_Url, 0, strlen(Ota_Url));
        memset(string2, 0, strlen(string2));
        while (Server_Cmd[count1] != '\"')
        {
            string2[count2] = Server_Cmd[count1];
            count1++;
            count2++;
        }
        count1 = 12;
        count2 = 0;
        strncpy(Ota_Url, string2, sizeof(Ota_Url));
        // ESP_LOGI("Ota_Url:%s\n",string2);

        logid1 = esp_mqtt_client_publish(client, logs_topic, string2, 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, on topic %s", logs_topic);
        ESP_LOGI(TAG, "data publish = %s", string2);

        // memset(Server_Cmd,0,strlen(Server_Cmd));
    }
    else if (strncmp(Server_Cmd, Ota_Cmd, 5) == 0)
    {
        // memset(Server_Cmd,0,strlen(Server_Cmd));
        // ESP_LOGI("OTA INTILIZATION\n");

        logid1 = esp_mqtt_client_publish(client, logs_topic, "OTA INTILIZATION", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, on topic %s", logs_topic);
        ESP_LOGI(TAG, "data publish = %s", "OTA INTILIZATION");

        // xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
        ota_task();
    }
    else if (strncmp(Server_Cmd, nvs_cmd, 5) == 0)
    {
        // ESP_LOGI("Erasing nvs");
        logid1 = esp_mqtt_client_publish(client, logs_topic, "Erasing nvs", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, on topic %s", logs_topic);
        ESP_LOGI(TAG, "data publish = %s", "Erasing nvs");

        ESP_ERROR_CHECK(nvs_flash_erase());
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);
        // Restart module

        ESP_LOGI(TAG, "Restarting now.\n");
        fflush(stdout);
        esp_restart();
    }
    else if (strncmp(Server_Cmd, reboot_cmd, 5) == 0)
    {
        /* code */
        // Restart module
        logid1 = esp_mqtt_client_publish(client, logs_topic, "REBOOT", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, on topic %s", logs_topic);
        ESP_LOGI(TAG, "data publish = %s", "REBOOT");
        for (int i = 5; i >= 0; i--)
        {
            ESP_LOGI(TAG, "Restarting in %d seconds...\n", i);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Restarting now.\n");
        fflush(stdout);
        esp_restart();
    }

    else
    {
        ESP_LOGI(TAG, "Wrong command \n");
    }
    // free(Server_Cmd);
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    // esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:

        mqtt_con_flag = 1;
        restart_flag = 0;
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        msg_id = esp_mqtt_client_publish(client, logs_topic, dev_speci, 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, on topic %s", logs_topic);
        msg_id = esp_mqtt_client_subscribe(client, cmd_topic, 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:

        mqtt_con_flag = 0;
        restart_flag++;
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:

        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC= %.*s\r\n", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA= %.*s\r\n", event->data_len, event->data);
        char Server_Cmd[300];
        memset(Server_Cmd, 0, strlen(Server_Cmd));

        strncpy(Server_Cmd, event->data, event->data_len);

        sprintf(Server_Cmd, "%.*s", event->data_len, event->data);
        // ESP_LOGI(TAG,"Raw topic:%s\n\r",raw_topic);
        ESP_LOGI(TAG, "Raw data :%s\n\r", Server_Cmd);
        cmd_topic_ISR(Server_Cmd);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    if (wifi_con_flag == true || eth_con_flag == true)
    { //++ Check if Wifi is Connected, then only check for MQTT Connection
        mqtt_event_handler_cb(event_data);
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqttbroker,
        //.event_handle = mqtt_event_handler,
    };
    mqtt_cfg.uri = mqtt_url;
    printf("%s \n", mqtt_cfg.uri);

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

//--------------------------------------------------------------------------------
/* OTA loops */
static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i)
    {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s %s", label, hash_print);
}

static void get_sha256_of_partitions(void)
{
    uint8_t sha_256[HASH_LEN] = {0};
    esp_partition_t partition;

    // get sha256 digest for bootloader
    partition.address = ESP_BOOTLOADER_OFFSET;
    partition.size = ESP_PARTITION_TABLE_OFFSET;
    partition.type = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
}
//-------------------------------------------------------------------------------
void non_volatile_mem_int()
{
    esp_err_t err;
    // Open
    ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle... \n");
    nvs_handle my_handle;

    // QUANTITY COUNT
    err = nvs_open("NETWORK", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) opening Quantity\n", esp_err_to_name(err));
    }
    else
    {
        printf("open NETWORK\n");
        uint8_t network_status = 0;

        err = nvs_get_u8(my_handle, "status", &network_status);
        if (err != ESP_OK)
        {  
            network_status = 1;     //++ Default Network set to WIFI
            ESP_LOGI(TAG, "Error (%s) opening status\n", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "status = %d\n", network_status);
        }
        status_nvs = network_status; //++ Network Status Saved in NVS

    }

    // wifi create
    err = nvs_open("wifi", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) opening wifi\n", esp_err_to_name(err));
    }
    else
    {
        printf("open wifi\n");

        size_t required_size;
        err = nvs_get_str(my_handle, "ssid", NULL, &required_size);

        nvs_ssid = malloc(required_size);
        err = nvs_get_str(my_handle, "ssid", nvs_ssid, &required_size);
        if (err != ESP_OK)
        {
            nvs_ssid = "ALITER";
            ESP_LOGI(TAG, "Error (%s) opening ssid\n", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "ssid = %s\n", nvs_ssid);
        }

        err = nvs_get_str(my_handle, "passwd", NULL, &required_size);

        nvs_passwd = malloc(required_size);
        err = nvs_get_str(my_handle, "passwd", nvs_passwd, &required_size);
        if (err != ESP_OK)
        {
            nvs_passwd = "Aliter@256";
            ESP_LOGI(TAG, "Error (%s) opening passwd!\n", esp_err_to_name(err));
        }
        else
        {
            // printf("Done\n");
            ESP_LOGI(TAG, "passwd = %s\n", nvs_passwd);
        }

        err = nvs_get_str(my_handle, "sta_ip", NULL, &required_size);

        nvs_sta_ip = malloc(required_size);
        err = nvs_get_str(my_handle, "sta_ip", nvs_sta_ip, &required_size);
        if (err != ESP_OK)
        {

            nvs_sta_ip = "192.168.0.250";
            ESP_LOGI(TAG, "Error (%s) opening ip\n", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "nvs_sta_ip = %s\n", nvs_sta_ip);
        }

        err = nvs_get_str(my_handle, "sta_gw", NULL, &required_size);

        nvs_sta_gw = malloc(required_size);
        err = nvs_get_str(my_handle, "sta_gw", nvs_sta_gw, &required_size);
        if (err != ESP_OK)
        {
            nvs_sta_gw = "192.168.0.1";

            ESP_LOGI(TAG, "Error (%s) gw_ip\n", esp_err_to_name(err));
        }
        else
        {

            ESP_LOGI(TAG, "nvs_sta_gw = %s\n", nvs_sta_gw);
        }

        err = nvs_get_str(my_handle, "sta_snt", NULL, &required_size);

        nvs_sta_snt = malloc(required_size);
        err = nvs_get_str(my_handle, "sta_snt", nvs_sta_snt, &required_size);
        if (err != ESP_OK)
        {
            nvs_sta_snt = "255.255.255.0";
            ESP_LOGI(TAG, "Error (%s) opening subnet\n", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "nvs_sta_snt = %s\n", nvs_sta_snt);
        }

        err = nvs_get_str(my_handle, "sta_dns1", NULL, &required_size);

        nvs_sta_dns1 = malloc(required_size);
        err = nvs_get_str(my_handle, "sta_dns1", nvs_sta_dns1, &required_size);

        if (err != ESP_OK)
        {
            nvs_sta_dns1 = "192.168.0.1";
            ESP_LOGI(TAG, "Error (%s) opening dns1\n", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "nvs_sta_dns1 = %s\n", nvs_sta_dns1);
        }

        err = nvs_get_str(my_handle, "sta_dns2", NULL, &required_size);

        nvs_sta_dns2 = malloc(required_size);
        err = nvs_get_str(my_handle, "sta_dns2", nvs_sta_dns2, &required_size);
        if (err != ESP_OK)
        {
            nvs_sta_dns2 = "8.8.8.8";
            ESP_LOGI(TAG, "Error (%s) opening dns2\n", esp_err_to_name(err));
        }
        else
        {

            ESP_LOGI(TAG, "nvs_sta_dns2 = %s\n", nvs_sta_dns2);
        }

        err = nvs_get_str(my_handle, "dhcp", NULL, &required_size);
        nvs_sta_dhcp = malloc(required_size);
        err = nvs_get_str(my_handle, "dhcp", nvs_sta_dhcp, &required_size);
        if (err != ESP_OK)
        {
            nvs_sta_dhcp = "1";
            ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "nvs_sta_dhcp = %s\n", nvs_sta_dhcp);
        }
    }

    err = nvs_open("ethernet", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
          ESP_LOGI(TAG,"Error (%s) opening ethernet\n", esp_err_to_name(err));
    } 
    else {
         
         ESP_LOGI(TAG,"Ethernet open");
        size_t required_size;
        err=nvs_get_str(my_handle, "dhcp", NULL, &required_size);
        nvs_sta_edhcp = malloc(required_size);
        err=nvs_get_str(my_handle, "dhcp", nvs_sta_edhcp, &required_size);
        if (err != ESP_OK) 
            {
                nvs_sta_edhcp = "1";
                ESP_LOGI(TAG,"Error (%s) opening NVS handle!\n", esp_err_to_name(err));
            }
        else {
                ESP_LOGI(TAG,"nvs_sta_edhcp = %s\n", nvs_sta_edhcp);
            }
            
            err=nvs_get_str(my_handle, "sta_ip", NULL, &required_size);
            nvs_sta_eip = malloc(required_size);
            err=nvs_get_str(my_handle, "sta_ip", nvs_sta_eip, &required_size);
            if (err != ESP_OK) 
            {
            
                nvs_sta_eip= "192.168.0.250";
                printf("Error (%s) opening ethip\n", esp_err_to_name(err));
            }
            else{
                ESP_LOGI(TAG,"nvs_sta_eip = %s\n", nvs_sta_eip);
                }

        err=nvs_get_str(my_handle, "sta_gw", NULL, &required_size);
        nvs_sta_egw = malloc(required_size);
        err=nvs_get_str(my_handle, "sta_gw", nvs_sta_egw, &required_size);
        if (err != ESP_OK) 
        {
            nvs_sta_egw = "192.168.0.1";
            ESP_LOGI(TAG,"Error (%s) opening ethgwip!\n", esp_err_to_name(err));
        }
        else {
          ESP_LOGI(TAG,"nvs_sta_egw = %s\n", nvs_sta_egw);
        }

        err=nvs_get_str(my_handle, "sta_snt", NULL, &required_size);
        nvs_sta_esnt = malloc(required_size);
        err=nvs_get_str(my_handle, "sta_snt", nvs_sta_esnt, &required_size);
        if (err != ESP_OK) 
            {
                nvs_sta_esnt = "255.255.255.0";
                ESP_LOGI(TAG,"Error (%s) opening ethsn\n", esp_err_to_name(err));
            }
        else{
                ESP_LOGI(TAG,"nvs_sta_esnt = %s\n", nvs_sta_esnt);
            }


        err=nvs_get_str(my_handle, "sta_dns1", NULL, &required_size);
        nvs_sta_edns1 = malloc(required_size);
        err=nvs_get_str(my_handle, "sta_dns1", nvs_sta_edns1, &required_size);
       
        if (err != ESP_OK) 
            {
                nvs_sta_edns1 = "192.168.0.1";
                ESP_LOGI(TAG,"Error (%s) opening eth dns1\n", esp_err_to_name(err));
            }
        else {
            ESP_LOGI(TAG,"nvs_sta_edns1 = %s\n", nvs_sta_edns1);
             }

        err=nvs_get_str(my_handle, "sta_dns2", NULL, &required_size);
   
          nvs_sta_edns2 = malloc(required_size);
        err=nvs_get_str(my_handle, "sta_dns2", nvs_sta_edns2, &required_size);
        if (err != ESP_OK) 
            {
                nvs_sta_edns2 = "8.8.8.8";
                ESP_LOGI(TAG,"Error (%s) opening eth dns2!\n", esp_err_to_name(err));
            }
        else {
            ESP_LOGI(TAG,"nvs_sta_edns2 = %s\n", nvs_sta_edns2);
            }
          //  nvs_close(my_handle);
    }

    // mqtt url

    err = nvs_open("mqttData", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~~~~~MQTT URL~~~~~~~~~~~~~~~~~~~~~~~");

        size_t required_size;
        err = nvs_get_str(my_handle, "mqtt_url", NULL, &required_size);

        mqtt_url = malloc(required_size);
        err = nvs_get_str(my_handle, "mqtt_url", mqtt_url, &required_size);
        if (err != ESP_OK)
        {
            mqtt_url = "mqtt://104.211.188.23:5131";
            ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "mqtt_url = %s\n", mqtt_url);
        }
    }

    nvs_close(my_handle);
    printf("\n");
    fflush(stdout);
}

//-------------------------------------------------------------------------------
void app_main(void)
{

    // led_strip.access_semaphore = xSemaphoreCreateBinary(); //++ ISR for RGB & Initialisation
    // bool led_init_ok = led_strip_init(&led_strip);
    // assert(led_init_ok);

    esp_err_t err;

    // Initialize NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    get_sha256_of_partitions();
    gpio_install_isr_service(0);

    non_volatile_mem_int();

    gpio_set_direction(WIFI_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(ETH_LED, GPIO_MODE_OUTPUT);

    gpio_set_level(WIFI_LED,0);
    gpio_set_level(ETH_LED,0);

    // Get Mac Address
    esp_efuse_mac_get_default(base_mac_addr);
    sprintf(mac_json, "%x:%x:%x:%x:%x:%x", base_mac_addr[0], base_mac_addr[1], base_mac_addr[2], base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);
    printf("mac_id = %s \n", mac_json);

    if(status_nvs == 1)
    {
        printf("WIFI IS SET\n");
        // Wifi Initialize
        wifi_init_sta();
    }
    else if(status_nvs == 0)
    {
        printf("ETHERNET IS SET\n");
        // Ethernet Initialize
        ethernet_init_sta();
    }

    // Serial UART
    uart_0_init();

    // MQTT TOPICS
    sprintf(data_topic, "%s/data", mac_json);
    sprintf(cmd_topic, "%s/cmd", mac_json);
    sprintf(logs_topic, "%s/logs", mac_json);
    sprintf(dev_speci, "{\"device\":\"%s\",\"FW_Ver\":%d}", mac_json, FW_version);

    // MQTT START
    mqtt_app_start();

    xTaskCreate(publish_task, "publish_task", 2048 * 2, NULL, configMAX_PRIORITIES, &xHandle_button_read);    //++ Posting Task
    xTaskCreate(uart0_rx_task, "uart0_rx_task", 2048 * 2, NULL, configMAX_PRIORITIES-1, NULL);                //++ UART + WIFI Reconnect Task

}

// led_strip_set_pixel_rgb(&led_strip, 0, 75, 0, 75); //++ PURPLE COLOUR
// led_strip_show(&led_strip);

// NETWORK="WIFI"
// NETWORK="ETHERNET"