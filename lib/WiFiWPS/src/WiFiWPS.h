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

#pragma once
//#ifndef WifiWPS_H_
//#define WifiWPS_H_

#include <WiFi.h>
#include <esp32-hal.h>
#include <esp_wifi.h>				// Wifi lib
#include <esp_wps.h>				// WSP lib
#include <esp_sntp.h>				// sNTP lib
#include <ArduinoOTA.h>				// OTA + mDNS

#define	ARDUINO_EVENT_OTA_START		((arduino_event_id_t)(ARDUINO_EVENT_MAX+1))
#define	ARDUINO_EVENT_OTA_END		((arduino_event_id_t)(ARDUINO_EVENT_MAX+2))

extern	void	message(WiFiEvent_t event, WiFiEventInfo_t& info);

class	WiFiWPS {
public:
	WiFiWPS()				{ _this = this; }

	void	setOTA(String hostname, String password)
			{ OTAHostName = hostname; OTAPassword = password; }

	void	getWifiSsidPsk(String& SSID, String& PSK)
			{ SSID = wifiSSID; PSK = wifiPSK; }

	wl_status_t begin();
	void		wpsStart();

	void		handle()
				{ ArduinoOTA.handle(); }

	bool		timeNtpIsSet()			// NTP time is set
				{ return ntp_time_sync; }

//	virtual	void	message(WiFiEvent_t event, WiFiEventInfo_t& info);

private:
	String		OTAHostName;			// Hostname for OTA/mDNS
	String		OTAPassword;			// Password for OTA
	String		wifiSSID;				// SSID from WPS
	String		wifiPSK;				// PSK from WPS

	volatile	
	bool		ntp_time_sync;
	bool		wps_started;

private:
	static	class WiFiWPS*		_this;
	static	esp_wps_config_t	config;
	
	static void onEvent(WiFiEvent_t event, WiFiEventInfo_t info)
	{ if (_this) _this->onEventHandler(event, info); }

	static	void	onTimeSync(struct timeval *t)
	{ if (_this) _this->onTimeSyncHandler(t); }

	void onEventHandler(WiFiEvent_t event, WiFiEventInfo_t& info);
	void onTimeSyncHandler(struct timeval *t);
};

//#endif /* WifiWPS_H_ */
