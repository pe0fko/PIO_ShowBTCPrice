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

#ifndef WIFIWPS_H
#define WIFIWPS_H

#include <WiFiWPS.h>

void WiFiWPS::onEventHandler(WiFiEvent_t event, WiFiEventInfo_t& info)
{
	switch (event) 
	{
	case ARDUINO_EVENT_WIFI_STA_START:
		// log_i("WiFi station started.\n");
		break;

	case ARDUINO_EVENT_WIFI_STA_CONNECTED:
		// log_i("WiFi connected, ssid: %.*s, channel: %d, authmode: %d",
		// 	info.wifi_sta_connected.ssid_len,
		// 	info.wifi_sta_connected.ssid,
		// 	info.wifi_sta_connected.channel,
		// 	info.wifi_sta_connected.authmode
		// );
		break;

	case ARDUINO_EVENT_WIFI_STA_GOT_IP:

		// log_i("WiFi IP: %s, GW: %s, NM: %s", 
		// 	IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str(),
		// 	IPAddress(info.got_ip.ip_info.gw.addr).toString().c_str(),
		// 	IPAddress(info.got_ip.ip_info.netmask.addr).toString().c_str()
		// );

		// Set the timezone for TZ_Europe_Amsterdam, CET-1CEST,M3.5.0,M10.5.0/3
		configTime(0, 0, "time.google.com", "pool.ntp.org" );
		setenv("TZ","CET-1CEST,M3.5.0,M10.5.0/3",1);	tzset();	// After the configTime!
		sntp_set_sync_interval(60 * 60 * 1000);						// 1 hour
		esp_sntp_init();											// Start sNTP

		// Start OTA & mDNS server.
		if (OTAHostName.length() != 0) 
		{
			log_i("OTA: Hostname %s defined", OTAHostName.c_str());
			ArduinoOTA.setHostname(OTAHostName.c_str());
			if (OTAPassword.length() != 0) {
				ArduinoOTA.setPassword(OTAPassword.c_str());
			}

			// ArduinoOTA.setRebootOnSuccess(true);
			// ArduinoOTA.setTimeout(1000);
			// ArduinoOTA.setMdnsEnabled(true);
			// ArduinoOTA.setPort(3232);

			// ArduinoOTA.onStart([](void){
			// 	int nullData = 0;
			// 	::message(ARDUINO_EVENT_OTA_START, (arduino_event_info_t&)nullData);
			// });
			
			// ArduinoOTA.onEnd([](void){
			// 	int nullData = 0;
			// 	::message(ARDUINO_EVENT_OTA_START, (arduino_event_info_t&)nullData);
			// });
			
			ArduinoOTA.begin();
		}

		break;

	case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
#ifdef DEBUG
		// Serial.printf("WiFi disconnected, ssid: %-.32s, reason: %d, rssi: %d\n",
		// 	info.wifi_sta_disconnected.ssid,
		// 	info.wifi_sta_disconnected.reason,
		// 	info.wifi_sta_disconnected.rssi
		// 	);
#endif
		ArduinoOTA.end();							// Stop OTA
		esp_sntp_stop();							// Stop sNTP
		ntp_time_sync = false;
		wps_timer = millis();
//		ESP_ERROR_CHECK( esp_wifi_connect() );		// Reconnect to the WiFi
		break;

	case ARDUINO_EVENT_WPS_ER_SUCCESS:
	{
		wifi_config_t conf;
		ESP_ERROR_CHECK( esp_wifi_get_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf) );
		ESP_ERROR_CHECK( esp_wifi_wps_disable() );
		// wps_started = false;

		wifiSSID = reinterpret_cast<char*>(conf.sta.ssid);
		wifiPSK  = reinterpret_cast<char*>(conf.sta.password);
		log_i("WiFi WPS Successful, SSID:%s Password:%s", wifiSSID.c_str(), wifiPSK.c_str() );

		delay(1000);
		// WiFi.begin();			// Connect to the WPS credentials in NVS.
		ESP.restart();				// When the SSID is received, reboot it
		// wps_started = false;
	}
	break;

	// case ARDUINO_EVENT_WPS_START:
	// 	log_i("***** ARDUINO_EVENT_WPS_START *****");
	// 	break;

	case ARDUINO_EVENT_WPS_ER_FAILED:
		ESP_ERROR_CHECK( esp_wifi_wps_disable() );
		wps_started = false;
		break;

	case ARDUINO_EVENT_WPS_ER_TIMEOUT:
		ESP_ERROR_CHECK( esp_wifi_wps_disable() );
		ESP_ERROR_CHECK( esp_wifi_wps_enable(&config) );
		ESP_ERROR_CHECK( esp_wifi_wps_start(0) );
		break;

	case ARDUINO_EVENT_WPS_ER_PIN:
		Serial.printf("WPS_PIN = %8.8s\n", info.wps_er_pin.pin_code);
		break;

	default:
		log_e("Unknow event detected!");
		return;
	}

	::message(event, info);
}

void WiFiWPS::wpsStart()
{
	if (!wps_started)
	{
		wps_started = true;
		log_d("WiFi WPS mode started.");
		// message("WPS/nStarted\n");
		ESP_ERROR_CHECK( esp_wifi_disconnect() );
		ESP_ERROR_CHECK( esp_wifi_wps_enable(&config) );
		ESP_ERROR_CHECK( esp_wifi_wps_start(0) );

		WiFiEventInfo_t info;	// Unused
		::message(ARDUINO_EVENT_WPS_START, info);
	}
}

wl_status_t WiFiWPS::init()
{
	int count = 0;
	int status;
	
	wps_started = false;
	ntp_time_sync = false;

	wps_timer = millis();						// Timer to start WPS

	WiFi.onEvent(onEvent);						// Set WiF event handler
	sntp_set_time_sync_notification_cb(onTimeSync);	// set time sync notification event

	pinMode(0, INPUT);							// Boot button, start the WPS (press after the boot code)
	WiFi.mode(WIFI_MODE_STA);					// Wifi station mode
	WiFi.persistent(true);						// Set WiFi SSID to persistent to save the credentials
	// WiFi.setAutoReconnect(false);		// Disable auto reconnect
	// WiFi.setAutoConnect(false);			// Disable auto connect

	// return WiFi.begin();						// Try to connect to NVS Wifi cridentionals
	WiFi.begin();
	int cnt = 30;
	while(cnt-- != 0 && WiFi.status() != WL_CONNECTED) {
		Serial.print('.'); delay(500);
	}
	Serial.println();

	return WiFi.status();
}

wl_status_t WiFiWPS::run()
{
	ArduinoOTA.handle();						// Handle the OTA

	if (wps_started) 
	{
		return WL_IDLE_STATUS;
	}

	if (digitalRead(0) == LOW) {
		wpsStart();
		return WL_DISCONNECTED;
	}

	switch (WiFi.status()) {
	case WL_IDLE_STATUS:
		return WL_IDLE_STATUS;
		break;

	case WL_CONNECTED:
		return WL_CONNECTED;
		break;

	case WL_DISCONNECTED:
		// After 5min start the WPS
		if ((millis() - wps_timer) > 5*60*1000) {
			wpsStart();
		}
		break;

	case WL_NO_SSID_AVAIL:
		log_d("WiFi status, WL_NO_SSID_AVAIL");
		wpsStart();
		break;

	case WL_CONNECT_FAILED:
		log_d("WiFi status, WL_CONNECT_FAILED");
		break;

	// case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
	// case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
	// case WL_DISCONNECTED: return "WL_DISCONNECTED";
	
	default:
		log_d("Waiting on the WiFi connect, status=%d.", WiFi.status());
		break;
	}

	return WiFi.status();

	// while((status = WiFi.status()) != WL_CONNECTED)		// Wait for a connect in 500ms steps
	// {
	// 	// After 20sec wifi connect start the WPS
	// 	if ( wps_started == false && ( ++count == 20 || digitalRead(0) == 0 ) ) {
	// 		wpsStart();
	// 	}

	// 	if (count == 300) ESP.restart();		// Reboot after boot in 5min
	// 	log_d("Waiting on the WiFi connect.");	// Wait info
	// 	delay(1000);							// Delay 1sec
	// }
	// return WL_CONNECTED;
}


void WiFiWPS::onTimeSyncHandler(struct timeval *t)
{
	ntp_time_sync = true;

#ifdef DEBUG
	struct tm info;
	Serial.println(localtime_r(&t->tv_sec, &info), "sNTP time sync: %A, %B %d %Y, %H:%M:%S");
#endif	
}


// int WiFiWPS::message(const char* format, ...)
// {
// 	char buf[64];
// 	va_list arg;
// 	va_start(arg, format);
// 	int len = vsnprintf(buf, sizeof(buf), format, arg);
// 	va_end(arg);
// 	if(len >= 0) {
// 		len = Serial.write(buf, len);
// 	}
// 	return len;
// }


// void message( WiFiEvent_t event, WiFiEventInfo_t& info )
// {
// 	switch (event)
// 	{
// 		case ARDUINO_EVENT_WIFI_STA_START:
// 		case ARDUINO_EVENT_WIFI_STA_STOP:
// 			break;

// 		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
// 			break;

// 		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
// 			break;

// 		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
// 			break;

// 		case ARDUINO_EVENT_WPS_ER_SUCCESS:
// 			break;

// 		case ARDUINO_EVENT_WPS_ER_FAILED:
// 			break;

// 		case ARDUINO_EVENT_WPS_ER_TIMEOUT:
// 			break;

// 		case ARDUINO_EVENT_WPS_ER_PIN:
// 			break;

// 		default:
// 			break;
// 	}
// }



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

class	WiFiWPS	wifiWPS;		// Define the WiFi WPS class

#endif