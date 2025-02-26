//=====================================================================
// Handle the WiFi connect with sNTP, OTA, mDNS and WPS.
// 2025/02/15 - Fred Krom, pe0fko.
//
// V0.1	- Initial version
//
// Arduino Tools menu:
// Processor:	ESP32 Dev Module
//
// Ports used: GPIO0, Boot, used for WPS
//
//=====================================================================

#include <WiFiWPS.h>

void WiFiWPS::onEventHandler(WiFiEvent_t event, WiFiEventInfo_t info)
{
	switch (event) 
	{
#if DEBUG
	case ARDUINO_EVENT_WIFI_STA_START:
		log_i("WiFi Station Mode Started"); 
		break;

	case WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED:
	{
		log_i("WiFi connected, ssid: %s, channel: %d, authmode: %d",
			info.wifi_sta_connected.ssid,
			info.wifi_sta_connected.channel,
			info.wifi_sta_connected.authmode
		);
	}
	break;
#endif

	case ARDUINO_EVENT_WIFI_STA_GOT_IP:
		log_i("WiFi IP: %s, GW: %s, NM: %s", 
			IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str(),
			IPAddress(info.got_ip.ip_info.gw.addr).toString().c_str(),
			IPAddress(info.got_ip.ip_info.netmask.addr).toString().c_str()
		);

		// Set the timezone for TZ_Europe_Amsterdam, CET-1CEST,M3.5.0,M10.5.0/3
		configTime(0, 0, "time.google.com", "pool.ntp.org" );
		setenv("TZ","CET-1CEST,M3.5.0,M10.5.0/3",1);	tzset();	// After the configTime!
		sntp_set_sync_interval(60 * 60 * 1000);						// 1 hour
		esp_sntp_init();											// Start sNTP

#ifdef FEATURE_OTA
		// Start OTA & mDNS server.
		if (OTAHostName.length() != 0) {
			log_i("OTA: Hostname %s defined", OTAHostName.c_str());
			ArduinoOTA.setHostname(OTAHostName.c_str());
			if (OTAPassword.length() != 0)
				ArduinoOTA.setPassword(OTAPassword.c_str());
			ArduinoOTA.begin();
		}
#endif
		break;

	case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
#ifdef DEBUG
		Serial.printf("WiFi disconnected, ssid: %-.32s, reason: %d, rssi: %d\n",
			info.wifi_sta_disconnected.ssid,
			info.wifi_sta_disconnected.reason,
			info.wifi_sta_disconnected.rssi
			);
#endif
		esp_sntp_stop();								// Stop sNTP
		ntp_time_sync = false;

		break;

	case ARDUINO_EVENT_WPS_ER_SUCCESS:
	{
		wifi_config_t conf;
		ESP_ERROR_CHECK( esp_wifi_get_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf) );
		ESP_ERROR_CHECK( esp_wifi_wps_disable() );
		wifiSSID = reinterpret_cast<char*>(conf.sta.ssid);
		wifiPSK  = reinterpret_cast<char*>(conf.sta.password);

		log_i("WiFi WPS Successful, SSID:%s Password:%s", wifiSSID.c_str(), wifiPSK.c_str() );
		delay(10);
		WiFi.begin();	// Connect to the WPS credentials in NVS.
	}
	break;

	case ARDUINO_EVENT_WPS_ER_FAILED:
	case ARDUINO_EVENT_WPS_ER_TIMEOUT:
		ESP_ERROR_CHECK( esp_wifi_wps_disable() );
		ESP_ERROR_CHECK( esp_wifi_wps_enable(&config) );
		ESP_ERROR_CHECK( esp_wifi_wps_start(0) );
		break;

	case ARDUINO_EVENT_WPS_ER_PIN:
		Serial.printf("WPS_PIN = %8.8s\n", info.wps_er_pin.pin_code);
		break;

	default:
		break;
	}
}

void WiFiWPS::wpsStart()
{
	log_i("WiFi WPS mode started.");
	wps_started = true;
	ESP_ERROR_CHECK( esp_wifi_disconnect() );
	ESP_ERROR_CHECK( esp_wifi_wps_enable(&config) );
	ESP_ERROR_CHECK( esp_wifi_wps_start(0) );
}


wl_status_t WiFiWPS::begin()
{
	int count = 0;
	int status;
	
	wps_started = false;
	ntp_time_sync = false;

	WiFi.onEvent(onEvent);						// Set WiF event handler
	sntp_set_time_sync_notification_cb(onTimeSync);	// set time sync notification event

	pinMode(0, INPUT);							// Boot button, start the WPS (press after the boot code)
	WiFi.mode(WIFI_MODE_STA);					// Wifi station mode

	log_i("Trying to connect WiFi");

	WiFi.begin();								// Try to connect to NVS Wifi cridentionals

	while((status = WiFi.status()) != WL_CONNECTED)		// Wait for a connect in 500ms steps
	{
		// After 20sec wifi connect start the WPS
		if ( wps_started == false && ( ++count == 20 || digitalRead(0) == 0 ) ) {
			wpsStart();
		}

		if (count == 300) ESP.restart();		// Reboot after boot in 5min
		log_d("Waiting on the WiFi connect.");	// Wait info
		delay(1000);							// Delay 1sec
	}
	return WL_CONNECTED;
}

void WiFiWPS::onTimeSyncHandler(struct timeval *t)
{
	ntp_time_sync = true;

#ifdef DEBUG
	struct tm info;
	Serial.println(localtime_r(&t->tv_sec, &info), "sNTP time sync: %A, %B %d %Y, %H:%M:%S");
#endif	
}

// Pointer to the one and only WiFiWPS class
class WiFiWPS* WiFiWPS::_this = nullptr;

// The used WPS config information
esp_wps_config_t WiFiWPS::config = //WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);
{	.wps_type = WPS_TYPE_PBC
,	.factory_info =
	{	ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(manufacturer,	"PE0FKO")
		ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(model_number,	"ESP32")
		ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(model_name,		"ESPRESSIF IOT")
		ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(device_name,	"ESP Show BTC Value")
	}
};
