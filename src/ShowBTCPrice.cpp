/**
   BasicHTTPSClient.ino

    Created on: 14.10.2018

*/

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
//#include <TZ.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>			// Adafruit GFX Library
#include <Adafruit_SSD1306.h>		// Adafruit SSD1306 Wemos Mini OLED
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <WiFi_SSID.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#if 1

//	client->setInsecure();
const char* https_host = "https://api.bitvavo.com/v2/ticker/24h?market=BTC-EUR";

#elif 1
//// echo | openssl s_client -showcerts -servername api.bitvavo.com -connect api.bitvavo.com:443 | openssl x509 -inform pem  -text
/*
 * Find all the certificate....
 * openssl s_client -showcerts -servername api.bitvavo.com -connect api.bitvavo.com:443
 * openssl x509 -in cert.x509 -text
 *
 *	client->setCACert(rootCACertificate);
 * 
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            b0:57:3e:91:73:97:27:70:db:b4:87:cb:3a:45:2b:38
        Signature Algorithm: sha256WithRSAEncryption
        Issuer: C = US, O = Internet Security Research Group, CN = ISRG Root X1
        Validity
            Not Before: Mar 13 00:00:00 2024 GMT
            Not After : Mar 12 23:59:59 2027 GMT
        Subject: C = US, O = Let's Encrypt, CN = E6
*/

const char* https_host = "https://api.bitvavo.com/v2/ticker/24h?market=BTC-EUR";
const char rootCACertificate [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIEVzCCAj+gAwIBAgIRALBXPpFzlydw27SHyzpFKzgwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw
WhcNMjcwMzEyMjM1OTU5WjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDELMAkGA1UEAxMCRTYwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAATZ8Z5G
h/ghcWCoJuuj+rnq2h25EqfUJtlRFLFhfHWWvyILOR/VvtEKRqotPEoJhC6+QJVV
6RlAN2Z17TJOdwRJ+HB7wxjnzvdxEP6sdNgA1O1tHHMWMxCcOrLqbGL0vbijgfgw
gfUwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcD
ATASBgNVHRMBAf8ECDAGAQH/AgEAMB0GA1UdDgQWBBSTJ0aYA6lRaI6Y1sRCSNsj
v1iU0jAfBgNVHSMEGDAWgBR5tFnme7bl5AFzgAiIyBpY9umbbjAyBggrBgEFBQcB
AQQmMCQwIgYIKwYBBQUHMAKGFmh0dHA6Ly94MS5pLmxlbmNyLm9yZy8wEwYDVR0g
BAwwCjAIBgZngQwBAgEwJwYDVR0fBCAwHjAcoBqgGIYWaHR0cDovL3gxLmMubGVu
Y3Iub3JnLzANBgkqhkiG9w0BAQsFAAOCAgEAfYt7SiA1sgWGCIpunk46r4AExIRc
MxkKgUhNlrrv1B21hOaXN/5miE+LOTbrcmU/M9yvC6MVY730GNFoL8IhJ8j8vrOL
pMY22OP6baS1k9YMrtDTlwJHoGby04ThTUeBDksS9RiuHvicZqBedQdIF65pZuhp
eDcGBcLiYasQr/EO5gxxtLyTmgsHSOVSBcFOn9lgv7LECPq9i7mfH3mpxgrRKSxH
pOoZ0KXMcB+hHuvlklHntvcI0mMMQ0mhYj6qtMFStkF1RpCG3IPdIwpVCQqu8GV7
s8ubknRzs+3C/Bm19RFOoiPpDkwvyNfvmQ14XkyqqKK5oZ8zhD32kFRQkxa8uZSu
h4aTImFxknu39waBxIRXE4jKxlAmQc4QjFZoq1KmQqQg0J/1JF8RlFvJas1VcjLv
YlvUB2t6npO6oQjB3l+PNf0DpQH7iUx3Wz5AjQCi6L25FjyE06q6BZ/QlmtYdl/8
ZYao4SRqPEs/6cAiF+Qf5zg2UkaWtDphl1LKMuTNLotvsX99HP69V2faNyegodQ0
LyTApr/vT01YPE46vNsDLgK+4cL6TrzC/a4WcmF5SRJ938zrv/duJHLXQIku5v0+
EwOy59Hdm0PT/Er/84dDV0CSjdR/2XuZM3kpysSKLgD1cKiDA+IRguODCxfO9cyY
Ig46v9mFmBvyH04=
-----END CERTIFICATE-----
)CERT";


#elif 1
/*
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            41:d2:9d:d1:72:ea:ee:a7:80:c1:2c:6c:e9:2f:87:52
        Signature Algorithm: ecdsa-with-SHA384
        Issuer: C = US, O = Internet Security Research Group, CN = ISRG Root X2
        Validity
            Not Before: Sep  4 00:00:00 2020 GMT
            Not After : Sep 17 16:00:00 2040 GMT
        Subject: C = US, O = Internet Security Research Group, CN = ISRG Root X2
*/
const char* https_host = "https://api.bitvavo.com/v2/ticker/24h?market=BTC-EUR";
const char rootCACertificate [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
)CERT";

#elif 1

const char* https_host = "https://ca.ict.nl/4/";

const char rootCACertificate [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIGEzCCA/ugAwIBAgIQfVtRJrR2uhHbdBYLvFMNpzANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTgx
MTAyMDAwMDAwWhcNMzAxMjMxMjM1OTU5WjCBjzELMAkGA1UEBhMCR0IxGzAZBgNV
BAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEYMBYGA1UE
ChMPU2VjdGlnbyBMaW1pdGVkMTcwNQYDVQQDEy5TZWN0aWdvIFJTQSBEb21haW4g
VmFsaWRhdGlvbiBTZWN1cmUgU2VydmVyIENBMIIBIjANBgkqhkiG9w0BAQEFAAOC
AQ8AMIIBCgKCAQEA1nMz1tc8INAA0hdFuNY+B6I/x0HuMjDJsGz99J/LEpgPLT+N
TQEMgg8Xf2Iu6bhIefsWg06t1zIlk7cHv7lQP6lMw0Aq6Tn/2YHKHxYyQdqAJrkj
eocgHuP/IJo8lURvh3UGkEC0MpMWCRAIIz7S3YcPb11RFGoKacVPAXJpz9OTTG0E
oKMbgn6xmrntxZ7FN3ifmgg0+1YuWMQJDgZkW7w33PGfKGioVrCSo1yfu4iYCBsk
Haswha6vsC6eep3BwEIc4gLw6uBK0u+QDrTBQBbwb4VCSmT3pDCg/r8uoydajotY
uK3DGReEY+1vVv2Dy2A0xHS+5p3b4eTlygxfFQIDAQABo4IBbjCCAWowHwYDVR0j
BBgwFoAUU3m/WqorSs9UgOHYm8Cd8rIDZsswHQYDVR0OBBYEFI2MXsRUrYrhd+mb
+ZsF4bgBjWHhMA4GA1UdDwEB/wQEAwIBhjASBgNVHRMBAf8ECDAGAQH/AgEAMB0G
A1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAbBgNVHSAEFDASMAYGBFUdIAAw
CAYGZ4EMAQIBMFAGA1UdHwRJMEcwRaBDoEGGP2h0dHA6Ly9jcmwudXNlcnRydXN0
LmNvbS9VU0VSVHJ1c3RSU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDB2Bggr
BgEFBQcBAQRqMGgwPwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRydXN0LmNv
bS9VU0VSVHJ1c3RSU0FBZGRUcnVzdENBLmNydDAlBggrBgEFBQcwAYYZaHR0cDov
L29jc3AudXNlcnRydXN0LmNvbTANBgkqhkiG9w0BAQwFAAOCAgEAMr9hvQ5Iw0/H
ukdN+Jx4GQHcEx2Ab/zDcLRSmjEzmldS+zGea6TvVKqJjUAXaPgREHzSyrHxVYbH
7rM2kYb2OVG/Rr8PoLq0935JxCo2F57kaDl6r5ROVm+yezu/Coa9zcV3HAO4OLGi
H19+24rcRki2aArPsrW04jTkZ6k4Zgle0rj8nSg6F0AnwnJOKf0hPHzPE/uWLMUx
RP0T7dWbqWlod3zu4f+k+TY4CFM5ooQ0nBnzvg6s1SQ36yOoeNDT5++SR2RiOSLv
xvcRviKFxmZEJCaOEDKNyJOuB56DPi/Z+fVGjmO+wea03KbNIaiGCpXZLoUmGv38
sbZXQm2V0TP2ORQGgkE49Y9Y3IBbpNV9lXj9p5v//cWoaasm56ekBYdbqbe4oyAL
l6lFhd2zi+WJN44pDfwGF/Y4QA5C5BIG+3vzxhFoYt/jmPQT2BVPi7Fp2RBgvGQq
6jG35LWjOhSbJuMLe/0CjraZwTiXWTb2qHSihrZe68Zk6s+go/lunrotEbaGmAhY
LcmsJWTyXnW0OMGuf1pGg+pRyrbxmRE1a6Vqe8YAsOf4vmSyrcjC8azjUeqkk+B5
yOGBQMkKW+ESPMFgKuOXwIlCypTPRpgSabuY0MLTDXJLR27lk8QyKGOHQ+SwMj4K
00u/I5sUKUErmgQfky3xxzlIPK1aEn8=
-----END CERTIFICATE-----
)CERT";

#endif

const		uint32_t	Access_Value	= 60UL * 1000;
static		uint32_t	Access_Timer	= 0UL;

void		JsonDecode(const char* json);
void		displayBTC(int nr);
void		displayGraph(int nr);

WiFiMulti	wifiMulti;
String		HostName					= "ShowBtc";
int			graph[128]					= {0};
int			graphLength					= 0;



// ./.arduino15/packages/esp8266/hardware/esp8266/3.1.2/cores/esp8266/TZ.h
//#define TZ_Europe_Amsterdam     PSTR("CET-1CEST,M3.5.0,M10.5.0/3")

// Not sure if WiFiClientSecure checks the validity date of the certificate. 
// Setting clock just to be sure...
void setClock() 
{
//  configTime(0, 0, "pool.ntp.org");
	configTime(0, 0, "time.google.com");
//	configTime(TZ_Europe_Amsterdam, "time.google.com","nl.pool.ntp.org");

	Serial.print(F("Waiting for NTP time sync: "));
	time_t nowSecs = time(nullptr);
	while (nowSecs < 8 * 3600 * 2) {
		delay(500);
		Serial.print(F("."));
		yield();
		nowSecs = time(nullptr);
	}

	Serial.println();
	struct tm timeinfo;
	gmtime_r(&nowSecs, &timeinfo);
	Serial.print(F("Current time: "));
	Serial.print(asctime(&timeinfo));
}


void setup() 
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	while(!Serial) ;

	Serial.println("====     Show BTC Price     ====");
	Serial.println(F("DATE:" __DATE__ " - TIME:" __TIME__));

	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
		Serial.println(F("SSD1306 allocation failed"));
		for(;;); // Don't proceed, loop forever
	}

	display.clearDisplay();
	display.setTextSize(1);      // text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setCursor(0, 4);     // Start at top-left corner
	display.print(F("WiFi connect"));
	display.display();

	WiFi.mode(WIFI_STA);
	for(uint8_t i = 0; i < WifiApListNumber; i++)
		wifiMulti.addAP(WifiApList[i].ssid, WifiApList[i].passwd);

	// Start mDNS service
	if (MDNS.begin(HostName))
//		MDNS.addService("http", "tcp", 80);
//	else
		;	//		PRINT_P("mDNS ERROR!\n");

	// Start OTA server.
	ArduinoOTA.setHostname((const char *)HostName.c_str());
//	ArduinoOTA.onStart([]() { ssd1306_printf_P(100, PSTR("OTA update\nRunning")); });
//	ArduinoOTA.onEnd([]()   { ssd1306_printf_P(100, PSTR("OTA update\nReboot")); ESP.restart(); });
	ArduinoOTA.setPassword("pe0fko");
	ArduinoOTA.begin();

	// wait for WiFi connection
//	Serial.print("Waiting for WiFi to connect...");
//	while ((wifiMulti.run() != WL_CONNECTED)) {
//		Serial.print(".");
//	}
//	Serial.println(" connected");

//	setClock();  
	configTime(0, 0, "time.google.com");

//	time_t now = time(nullptr);			// get UNIX timestamp
//	Serial.printf("Time: %s\n", ctime(&now));					// convert timestamp and display

	Serial.print("Access host: ");
	Serial.println(https_host);

	display.clearDisplay();
	display.setTextSize(1);      // text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setCursor(0, 4);     // Start at top-left corner
	display.print(https_host);
	display.display();

	Access_Timer = millis() - Access_Value;
}

void loop() 
{
	ArduinoOTA.handle();

	if ((wifiMulti.run() != WL_CONNECTED)) {
		Serial.println(F("WiFI Multi connect."));
		delay(200);
	} 
	else if (millis() - Access_Timer > Access_Value)
	{
		Access_Timer += Access_Value;

//		time_t now = time(nullptr);
//		Serial.printf("Date/Time: %s", ctime(&now));

		WiFiClientSecure *client = new WiFiClientSecure;
		if(client) 
		{
			client->setInsecure();
//			client->setCACert(rootCACertificate);

	        // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
	        HTTPClient https;
	    
//			Serial.print("[HTTPS] begin...\n");
			if (https.begin(*client, https_host))  // HTTPS
			{
//				Serial.print("[HTTPS] GET...\n");
				// start connection and send HTTP header
				int httpCode = https.GET();
	    
				// httpCode will be negative on error
				if (httpCode > 0) {
					// HTTP header has been send and Server response header has been handled
//					Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
	    
					// file found at server
					if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
					{
						String payload = https.getString();
//						Serial.println(payload);
						JsonDecode(payload.c_str());
					}
				} 
				else 
				{
				Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
				}
				https.end();
	        } 
			else 
			{
	          Serial.printf("[HTTPS] Unable to connect\n");
	        }
			delete client;
		} 
		else 
		{
			Serial.println("Unable to create client");
		}
	}
}

void JsonDecode(const char* json)
{
	// Allocate the JSON document
	JsonDocument doc;

	// Deserialize the JSON document
	DeserializationError error = deserializeJson(doc, json);

	// Test if parsing succeeds
	if (error) {
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(error.f_str());
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

	displayBTC(bk_last);
	displayGraph(bk_last);
	display.display();
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

	Serial.printf("Range: %d - (%d-%d)\n", range, min, max);
	display.setTextSize(1);						// 6x8
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
	display.clearDisplay();
	display.invertDisplay(false);
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setTextSize(1);      // text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc

//	display.setCursor(0, 32-8);     // Start at top-left corner
//	display.print("pe0fko");
	display.setCursor(128-3*6, 32-8+1);     // Start at top-left corner
	display.print("BTC");

	display.setTextSize(3);      // text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
	display.setCursor(18, 4);     // Start at top-left corner

	if (nr >= 1000) 
	{
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