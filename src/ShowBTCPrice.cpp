//=====================================================================
// Show BTC Price from the BitVavo provider.
// 2018/10/14 - Fred Krom, pe0fko.
//
// V0.1	- Initial version
//
// Arduino Tools menu:
// Board:		ESP-WROOM-32, Wemos D1 R32, and other WSP32
// Processor:	ESP32 Dev Module
//
// Ports used:
//
//=====================================================================

#include <Arduino.h>				// Arduino
#include <WiFi.h>					// WiFi
#include <time.h>					// time
#include <HTTPClient.h>				// HTTPClient
#include <WiFiClientSecure.h>		// WiFiClientSecure
#include <ArduinoJson.h>			// ArduinoJson
#include <SPI.h>					// SPI
#include <Wire.h>					// I2C
#include <Adafruit_SSD1306.h>		// Adafruit SSD1306 Wemos Mini OLED
#include <ESPmDNS.h>				// mDNS
#include <ArduinoOTA.h>				// OTA
#include <WiFi_SSID.h>				// WiFi SSID and password
#include "certificate.h"			// Root certificate for https definded

#define SCREEN_WIDTH	128			// OLED display width, in pixels
#define SCREEN_HEIGHT	32			// OLED display height, in pixels

#define OLED_RESET		-1			// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS	0x3C		// Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const		uint32_t	Access_Value	= 60UL * 1000;	// 1min in ms
static		uint32_t	Access_Timer	= 0UL;
static		uint32_t	Process_Timer	= 0UL;
static		WiFiMulti	wifiMulti;
static		String		HostName		= "ShowBtc";
static		int			graph[128]		= {0};
static		int			graphLength		= 0;

// Forward references
void		JsonDecode(const char* json);
void		displayTime();
void		displayBTC(int nr);
void		displayGraph(int nr);


void setup() 
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	while(!Serial) ;

	Serial.printf("====     Show BTC Price     ====\n");
	Serial.printf("Build: %s %s\n", __DATE__, __TIME__);

	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
		Serial.printf("SSD1306 display initialize failed....\n");
		while(1);
	}

	display.clearDisplay();
	display.setTextSize(1);					// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setTextColor(SSD1306_WHITE);	// White text
	display.invertDisplay(false);
	display.setCursor(0, 4);	display.printf("== Show BTC Price ==");
	display.setCursor(0, 16);	display.printf(" PE0FKO:" __DATE__);
	display.display();
	delay(2000);

	WiFi.mode(WIFI_STA);
	for(uint8_t i = 0; i < WifiApListNumber; i++)
		wifiMulti.addAP(WifiApList[i].ssid, WifiApList[i].passphrase);

	// Set the timezone for TZ_Europe_Amsterdam
	configTime(0, 0, "time.google.com");
	setenv("TZ","CET-1CEST,M3.5.0,M10.5.0/3",1);
	tzset();

	// Start mDNS service
	MDNS.begin(HostName);

	// Start OTA server.
	ArduinoOTA.setHostname(static_cast<const char*>(HostName.c_str()));
	ArduinoOTA.setPassword(OtaPassword);
	ArduinoOTA.begin();

	display.clearDisplay();
	display.setCursor(0, 4);
	display.printf("URL: %s", https_host);
	display.display();
	Serial.printf("Access host: %s\n", https_host);

	Access_Timer = millis() - Access_Value;
}


void ErrorMsg(int ms, const char line[])
{
	display.invertDisplay(true);
	display.clearDisplay();
	display.setTextSize(1);					// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setCursor(0, 8);
	display.print(line);
	display.display();

	delay(ms);
	display.invertDisplay(false);
}


void loop() 
{
	ArduinoOTA.handle();

	if ((wifiMulti.run() != WL_CONNECTED)) {
		Serial.printf("Error: WiFI Multi connect, not connected.\n");
		ErrorMsg(1000, "E: Connect WiFi");
	} 
	else if (millis() - Access_Timer > Access_Value)
	{
		Access_Timer += Access_Value;

		Process_Timer = millis();

		WiFiClientSecure *client = new WiFiClientSecure;
		if(client) 
		{
			SET_ROOT_CERTIFICATE_CACERT;		// Set the root certificate or setInsecure!

	        HTTPClient https;	        		// Add a scoping block for HTTPClient https to 
												// make sure it is destroyed before WiFiClientSecure *client is 
	    
			if (https.begin(*client, https_host))  // HTTPS
			{
				int httpCode = https.GET();		// start connection and send HTTP header
				if (httpCode > 0) 
				{
					// HTTP header has been send and Server response header has been handled	    
					// file found at server
					if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
					{
						String payload = https.getString();
						JsonDecode(payload.c_str());
					}
				} else {
					Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
					ErrorMsg(1000, "E: HTTPS GET");
				}
				https.end();
			}
			else 
			{
				Serial.printf("[HTTPS] Unable to connect\n");
				ErrorMsg(1000, "E: HTTPS connect");
	        }
			delete client;
		} 
		else 
		{
			Serial.printf("[HTTPS] Unable to create https client\n");
			ErrorMsg(1000, "E: HTTPS client");
		}

		Process_Timer -= millis();
		Serial.printf("Process time: %d ms\n", -Process_Timer);
	}
}

void JsonDecode(const char* json)
{
	DeserializationError error;
	JsonDocument doc;						// Allocate the JSON document

	error = deserializeJson(doc, json);		// Deserialize the JSON document
	if (error) {
		Serial.printf("deserializeJson() failed: %s\n", error.f_str());
		ErrorMsg(1000, "E: JSON");
		return;
	}

/*  {"market":"BTC-EUR","startTimestamp":1707345544376,"timestamp":1707431944376,
    "open":"41029",
    "openTimestamp":1707345556907,
    "high":"42249",
    "low":"4.089E+4",
    "last":"41945",
    "closeTimestamp":1707431938361,
    "bid":"41945",
    "bidSize":"0.04847245",
    "ask":"41952",
    "askSize":"0.31887",
    "volume":"940.65235665",
    "volumeQuote":"39236728.00683897"}
*/

#if 0
	int   bk_open     = doc["open"];
	int   bk_high     = doc["high"];
	int   bk_low      = doc["low"];
	int   bk_last     = doc["last"];
	int   bk_bid      = doc["bid"];
	float bk_bidSize  = doc["bidSize"];
	int   bk_ask      = doc["ask"];
	float bk_askSize  = doc["askSize"];
	float bk_volume   = doc["volume"];

	static  int   bk_last_prev = 0;
	if (bk_last_prev == 0) bk_last_prev = bk_last;

	Serial.printf(
		"Open:%5d (%5d -- %5d) Last:%5d(%4d),  Bid:%5d(%3d) Vol:%4.2f,  Ask:%5d(%3d) Size:%.4f Vol:%4.2f,  (%.2f)\n",
		bk_open, bk_low, bk_high, bk_last, bk_last-bk_last_prev,
		bk_bid, bk_bid-bk_last, bk_bidSize, 
		bk_ask, bk_ask-bk_last, bk_askSize, bk_volume,
		0.20664391 * bk_last
		);

	bk_last_prev = bk_last;
#endif

	int btc = doc["last"];

	display.clearDisplay();
//	display.setTextColor(SSD1306_WHITE); // Draw white text
//	display.setTextSize(1);      // text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc

	displayTime();
	displayBTC(btc);
	displayGraph(btc);

	display.display();
}

void displayTime()
{
	struct tm timeinfo;
	time_t now = time(nullptr);
	localtime_r(&now, &timeinfo);

	display.setTextSize(1);      // text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setCursor(2, 32-8+1);
	display.printf("%2d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
}

void displayGraph(int nr)
{
	const int L = 128;		// 128, 64, 32...
	int min = 10000000;
	int max = 0;
	int range;

	if (graphLength < L) {
		graph[graphLength++] = nr;
	} else {
		memcpy((void*)&graph[0], (void*)&graph[1], (L-1)*sizeof(int));
		graph[L-1] = nr;
	}

	for (int i = 0; i < graphLength; i++) {
		if (graph[i] > max)	max = graph[i];
		if (graph[i] < min)	min = graph[i];
	}
	range = max - min;

	Serial.printf("BTC: %d, Range: %d - (%d-%d)\n", nr, range, min, max);
	display.setTextSize(1);      			// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setCursor(128-3*6-6-4*6, 32-8+1);
	display.printf("%4d", range);

	for (int i = 0; i < graphLength; i++) {
		int n = graph[i];
		n -= min;
		n *= 31;
		n /= range;
		if (range == 0) n = 0;
		display.drawPixel(i * 128 / L, 31-n, SSD1306_INVERSE);
	}
}

void displayBTC(int nr)
{
	display.setCursor(128-3*6, 32-8+1);     // Start at top-left corner
	display.printf("BTC");

	display.setTextSize(3);      			// text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setCursor(18, 4);				// Start at top-left corner

	if (nr >= 1000) {
		display.setCursor(8, 3);
		display.printf("%3d", nr / 1000);
		// Draw dot
		int x = display.getCursorX();
		display.drawFastHLine(x, 3+24-5-2, 3, SSD1306_WHITE);
		display.drawFastHLine(x, 3+24-4-2, 3, SSD1306_WHITE);
		display.drawFastHLine(x, 3+24-3-2, 3, SSD1306_WHITE);

		display.setCursor(x+6, 3);
		display.printf("%03d", nr % 1000);
	} else {
		display.setCursor(16+3*18+6, 3);
		display.printf("%3d", nr);
	}
}