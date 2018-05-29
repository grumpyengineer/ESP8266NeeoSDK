/*
ESP8266 Neeo Smart Remote SDK
Version 0.1
Copyright (c) 2018 Tom the Grumpy Engineer (tom@grumpy.engineeting)


This is a simple implementation of the Neeo Smart Remote SDK that turns
an ESP8266 module into a standalone device without needing the node.js
SDK running on another server

Requirements:
- ESP8266WiFi library
- ESP8266mDNS library
- ArduinoOTA library
- ESP8266HTTPClient library
- ESP8266WebServer library
- Hash library

REcommended
- ArduinoOTA library

Usage:
- Include the Neeo SDK library in the sketch.
- Call the begin method in the sketch's setup to clear the data structures
- Call the findBrain or setBrain method in the sketch's setup to
- Call the addDevice method in the sketch's setup
- Call the addButton method in the sketch's setup
- Call the register method in the sketch's setup to register the device(s) with the brain
- Call the loop method in each iteration of the sketch's loop function.

License (MIT license):
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
#ifndef ESP8266NEEOSDK_H
#define ESP8266NEEOSDK_H

//this should be defined at build time
#ifndef ARDUINO_BOARD
#define ARDUINO_BOARD "generic"
#endif

extern "C" {
    #include "osapi.h"
    #include "ets_sys.h"
    #include "user_interface.h"
}
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"

#define NEEOSEARCH "neeo"
#define REGISTERURL "/v1/api/registerSdkDeviceAdapter"

typedef void ( *buttonHandler )(String command);
typedef int ( *sensorHandler )(String sensor);

struct neeoButton {
	neeoButton* _next;
	char *_name;
	char *_label;
};

struct neeoSensor {
	neeoSensor* _next;
	char *_name;
	char *_label;
};

struct neeoSearchTokens {
	neeoSearchTokens* _next;
	char *_token;
};

struct neeoDevice {
	neeoDevice* _next;
	char *_name;
	char *_manufacturer;
	char *_type;
	char *_adapterName;
	neeoSearchTokens* _additionalSearchTokens;
	neeoButton* _buttons;
	buttonHandler _buttonHandler;

    //this.buttonhandler = undefined;
    //this.initialiseFunction = undefined;
   // this.deviceCapabilities = [];
   // this.sensors = [];
   // this.buttons = [];
   // this.sliders = [];
   // this.textLabels = [];
   // this.imageUrls = [];
   // this.switches = [];
   // this.discovery = [];
   // this.setup = {};
};

struct neeoBrain {
	IPAddress ip;
	uint16_t port;
};

struct neeoServer {
	char *name;
	uint16_t port;
	neeoBrain* brain;
	neeoDevice* devices;
};

class ESP8266NeeoSDK {
public:
	ESP8266NeeoSDK();
	~ESP8266NeeoSDK();
	void begin(char *hostName, uint16_t port);
	bool findBrain();
	void setBrain(IPAddress ip, uint16_t port);
	bool registerServer();
	bool startServer(ESP8266WebServer *server);
	void loop();

	int addDevice(char *name, char *manufacturer, char *type);
	bool addButtonHandler(int idx, buttonHandler buttonHandlerFunction);
	bool addSearchTokens(int idx, char *token);
	bool addButton(int idx, char *name, char *label);



private:
	neeoServer _server;
	ESP8266WebServer* _neeoServer;
	void _handleNotFound();
	void _handleSearch();
	String _getDeviceJson(int idx, int addTokens);
	String _urlencode(String str);
	String _urldecode(String str);
	unsigned char _h2int(char c);

};


#endif //ESP8266NEEOSDK_H