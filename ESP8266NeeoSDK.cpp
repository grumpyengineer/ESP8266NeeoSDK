/*
ESP8266 Neeo Smart Remote SDK
Version 0.1
Copyright (c) 2018 Tom the Grumpy Engineer (tom@grumpy.engineeting)

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

// For reference:
// Neeo SDK and examples in node.js - https://github.com/NEEOInc

#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <Hash.h>
#include "ESP8266NeeoSDK.h"

ESP8266NeeoSDK::ESP8266NeeoSDK() {
}

ESP8266NeeoSDK::~ESP8266NeeoSDK() {
}

// Set the name of the server and the port to be used
void ESP8266NeeoSDK::begin(char *name, uint16_t port) {
	_server.name = (char *)os_malloc(strlen(name) + 1);
	os_strcpy(_server.name, name);
	_server.port = port;
	_server.brain = (struct neeoBrain*)(os_malloc(sizeof(struct neeoBrain)));
	_server.devices = 0;
}

// Finds neeo brains on the network and uses the first one found
bool ESP8266NeeoSDK::findBrain(void) {
	neeoBrain* brain = _server.brain;

	int n = MDNS.queryService(NEEOSEARCH, "tcp"); // Send out query for esp tcp services
	if (n == 0) {
		return false;
	}
	else {
      	brain->ip = MDNS.IP(0);
      	brain->port = MDNS.port(0);
    }

	Serial.print("Found Brain: ");
	Serial.print(brain->ip[0]);
	Serial.print(".");
	Serial.print(brain->ip[1]);
	Serial.print(".");
	Serial.print(brain->ip[2]);
	Serial.print(".");
	Serial.print(brain->ip[3]);
	Serial.print(":");
	Serial.println(brain->port);

	return true;
}

// Set the IP and port for the brain manually
void ESP8266NeeoSDK::setBrain(IPAddress ip, uint16_t port) {
	neeoBrain* brain = _server.brain;

   	brain->ip = ip;
   	brain->port = port;
}

// Register the server with the Neeo brain
bool ESP8266NeeoSDK::registerServer(void) {
	neeoBrain* brain = _server.brain;
	HTTPClient http;
	bool success = false;
	String tempString;

	tempString = "http://";
	tempString.concat(brain->ip.toString());
	tempString.concat(":");
	tempString.concat(brain->port);
	tempString.concat(REGISTERURL);

	Serial.print("[HTTP] begin...\n");
	Serial.println(tempString);
	http.begin(tempString);

	http.addHeader("Content-Type", "application/json");

	tempString = "{\"name\":\"src-";
	tempString.concat(sha1(_server.name));
	tempString.concat("\",\"baseUrl\":\"http://");
	tempString.concat(WiFi.localIP().toString());
	tempString.concat(":");
	tempString.concat(_server.port);
	tempString.concat("\"}");

	Serial.println(tempString);

	int httpCode = http.POST(tempString);

	// httpCode will be negative on error
	if(httpCode > 0) {
		// file found at server
		if(httpCode == HTTP_CODE_OK) {
			Serial.println("Register Succeeded!");
			success = true;
		}
	} else {
		Serial.println("Register Failed!");
	}

	http.end();

	return success;
}

void ESP8266NeeoSDK::_handleNotFound(void) {
	String uri;
	String remoteRequest;
	String remoteAction;
	String remoteDevice;
	String remoteCommand;
	String message = "{\"success\":true}";
	int strIndex;
	int found;
	neeoDevice* device = _server.devices;

	uri = _neeoServer->uri();

  	Serial.print("URI: ");
  	Serial.println(uri);
  	Serial.print("Method: ");
  	Serial.println((_neeoServer->method() == HTTP_GET)?"GET":"POST");
  	Serial.print("nArguments: ");
  	Serial.println(_neeoServer->args());
  	for (uint8_t i=0; i<_neeoServer->args(); i++){
    	Serial.print(_neeoServer->argName(i));
    	Serial.print(": ");
    	Serial.println(_neeoServer->arg(i));
  	}

	strIndex = uri.indexOf("/",1);
  	remoteRequest = uri.substring(1,strIndex);

  	remoteAction = uri.substring(strIndex+1);

  	Serial.print("Request: ");
  	Serial.println(remoteRequest);
  	Serial.print("Action: ");
  	Serial.println(remoteAction);

  	if(remoteRequest == "db") {
		message = _getDeviceJson(remoteAction.toInt(),1);
	}

  	if(remoteRequest == "device") {
		strIndex = remoteAction.indexOf("/");
		remoteDevice = remoteAction.substring(0,strIndex);
		remoteCommand = remoteAction.substring(strIndex+1);
		strIndex = remoteCommand.indexOf("/");
		remoteCommand = remoteCommand.substring(0,strIndex);

	  	Serial.print("Device: ");
	  	Serial.println(remoteDevice);

	  	Serial.print("Command: ");
	  	Serial.println(remoteCommand);

	  	found = 0;
		while((device != 0) && found == 0) {
			if(remoteDevice == String(device->_adapterName)) {
				Serial.print("Found device: ");
	  			Serial.println(device->_name);
				found = 1;
			}
			else
				device = device->_next;
		}

		if(found == 1) {
			if(remoteCommand == "subscribe") {
				Serial.println("Device Subscribed");
			}
			else if(remoteCommand == "unsubscribe") {
				Serial.println("Device Unsubscribed");
			}
			else {
				Serial.print("Fire event for: ");
				Serial.println(_urldecode(remoteCommand));
				if(device->_buttonHandler != 0)
					(*device->_buttonHandler)(_urldecode(remoteCommand));
			}
		}


	}
  	Serial.println(message);

	_neeoServer->send(200, "application/json", message);
}

void ESP8266NeeoSDK::_handleSearch(void) {
	int found = 0;
	int foundItems = 0;
	String message = "{}";
	String haystack;
	String needle;
	neeoDevice* device = _server.devices;
	neeoSearchTokens *searchToken;
	int i = 0;

	if(_neeoServer->args() == 1) {
		if(_neeoServer->argName(0) == "q") {
			needle = _neeoServer->arg(0);
			Serial.print("Searching for: ");
			Serial.println(needle);

			while(device != 0) {
				found = 0;
				haystack = String(device->_name);

				if(haystack.indexOf(needle) >= 0) {
					Serial.println("Found search item in device name");
					found = 1;
				}

				searchToken = device->_additionalSearchTokens;
				while(searchToken != 0) {
					haystack = String(searchToken->_token);

					if(haystack.indexOf(needle) >= 0) {
						Serial.println("Found search item in additional search token");
						found = 1;
					}

					searchToken = searchToken->_next;
				}

				if(found != 0) {
					if(foundItems == 0)
						message = "[";
					else
						message.concat(",");

					message.concat("{\"item\":");
					message.concat(_getDeviceJson(i,0));
					message.concat(",\"score\":0,");
					message.concat("\"maxScore\":");
					message.concat(needle.length());
					message.concat("}");

					foundItems++;
				}

				device = device->_next;
				i++;
			}

			if(foundItems != 0)
				message.concat("]");
		}
	}

	Serial.println(message);

	_neeoServer->send(200, "application/json", message);
}

// start the server so the Neeo brain can talk to it
bool ESP8266NeeoSDK::startServer(ESP8266WebServer *server) {

	_neeoServer = server;

	_neeoServer->on("/db/search", [this](){ _handleSearch(); });
	_neeoServer->onNotFound([this](){ _handleNotFound(); });
	_neeoServer->begin();

	return true;

}

// server loop
void ESP8266NeeoSDK::loop() {

	_neeoServer->handleClient();

}

int ESP8266NeeoSDK::addDevice(char *name, char *manufacturer, char *type) {

	int i = 0;
	struct neeoDevice *newDevice = (struct neeoDevice*)(os_malloc(sizeof(struct neeoDevice)));
	String tempString;

	Serial.println("Adding device");

	tempString = "apt-" + sha1(name);

	newDevice->_name = (char *)os_malloc(strlen(name) + 1);
	os_strcpy(newDevice->_name, name);
	newDevice->_manufacturer = (char *)os_malloc(strlen(manufacturer) + 1);
	os_strcpy(newDevice->_manufacturer, manufacturer);
	newDevice->_type = (char *)os_malloc(strlen(type) + 1);
	os_strcpy(newDevice->_type, type);
	newDevice->_adapterName = (char *)os_malloc(strlen(tempString.c_str()) + 1);
	os_strcpy(newDevice->_adapterName, tempString.c_str());
	newDevice->_next = 0;
	newDevice->_buttons = 0;
	newDevice->_additionalSearchTokens = 0;
	newDevice->_buttonHandler = 0;

	if(_server.devices == 0) {
		Serial.println("Adding first device");
		_server.devices = newDevice;
	}
	else {
		Serial.println("Skipping over existing devices");
    	neeoDevice* devicePtr = _server.devices;
    	i++;
    	while(devicePtr->_next != 0) {
			devicePtr = devicePtr->_next;
			i++;
		}
    	devicePtr->_next = newDevice;
	}

	return i;
}

bool ESP8266NeeoSDK::addButtonHandler(int idx, buttonHandler buttonHandlerFunction) {
	neeoDevice* device = _server.devices;

  	while (device != 0 && idx-- > 0) {
    	device = device->_next;
  	}
  	if (idx > 0) {
    	return false;
  	}

  	device->_buttonHandler = buttonHandlerFunction;

  	return true;
}

bool ESP8266NeeoSDK::addSearchTokens(int idx, char *token) {
	struct neeoSearchTokens *newSearchToken = (struct neeoSearchTokens*)(os_malloc(sizeof(struct neeoSearchTokens)));
	neeoDevice* device = _server.devices;

	newSearchToken->_token = (char *)os_malloc(strlen(token) + 1);
	os_strcpy(newSearchToken->_token, token);
	newSearchToken->_next = 0;

  	while (device != 0 && idx-- > 0) {
    	device = device->_next;
  	}
  	if (idx > 0) {
    	return false;
  	}

	if(device->_additionalSearchTokens == 0) {
		device->_additionalSearchTokens = newSearchToken;
	}
	else {
    	neeoSearchTokens* searchTokenPtr = device->_additionalSearchTokens;
    	while(searchTokenPtr->_next != 0) {
			searchTokenPtr = searchTokenPtr->_next;
		}
    	searchTokenPtr->_next = newSearchToken;
	}

	return true;
}

bool ESP8266NeeoSDK::addButton(int idx, char *name, char *label) {
	struct neeoButton *newButton = (struct neeoButton*)(os_malloc(sizeof(struct neeoButton)));
	neeoDevice* device = _server.devices;

	newButton->_name = (char *)os_malloc(strlen(name) + 1);
	os_strcpy(newButton->_name, name);
	newButton->_label = (char *)os_malloc(strlen(label) + 1);
	os_strcpy(newButton->_label, label);
	newButton->_next = 0;

  	while (device != 0 && idx-- > 0) {
    	device = device->_next;
  	}
  	if (idx > 0) {
    	return false;
  	}

	if(device->_buttons == 0) {
		device->_buttons = newButton;
	}
	else {
    	neeoButton* buttonPtr = device->_buttons;
    	while(buttonPtr->_next != 0) {
			buttonPtr = buttonPtr->_next;
		}
    	buttonPtr->_next = newButton;
	}

	return true;
}

String ESP8266NeeoSDK::_getDeviceJson(int idx, int addTokens) {
	neeoDevice* device = _server.devices;
	neeoSearchTokens *searchToken;
	neeoButton *buttons;
	String tempString = "";
	String extraTokens = "\"dataEntryTokens\": [";
	String tempExtraTokens;
	int i;
	int strIndex;


	i = idx;
 	while (device != 0 && i-- > 0) {
    	device = device->_next;
  	}
  	if (i > 0) {
    	return tempString;
  	}

	extraTokens.concat("\"");
	tempExtraTokens = String(device->_manufacturer);
	tempExtraTokens.toLowerCase();
	extraTokens.concat(tempExtraTokens);
	extraTokens.concat("\"");
	extraTokens.concat(",");
	extraTokens.concat("\"");
	tempExtraTokens = String(device->_type);
	tempExtraTokens.toLowerCase();
	extraTokens.concat(tempExtraTokens);
	extraTokens.concat("\"");

	strIndex = 0;
	tempExtraTokens = String(device->_name);
	tempExtraTokens.toLowerCase();

	while(tempExtraTokens.indexOf(" ", strIndex) >= 0) {
		extraTokens.concat(",");
		extraTokens.concat("\"");
		extraTokens.concat(tempExtraTokens.substring(strIndex,tempExtraTokens.indexOf(" ", strIndex)));
		extraTokens.concat("\"");
		strIndex = tempExtraTokens.indexOf(" ", strIndex) + 1;
	}
	extraTokens.concat(",");
	extraTokens.concat("\"");
	extraTokens.concat(tempExtraTokens.substring(strIndex));
	extraTokens.concat("\"");

	tempString = "{";
	tempString.concat("\"id\":");
	tempString.concat(String(idx));
	tempString.concat(",");
	tempString.concat("\"adapterName\":\"");
	tempString.concat(String(device->_adapterName));
	tempString.concat("\",");
	tempString.concat("\"type\":\"");
	tempString.concat(String(device->_type));
	tempString.concat("\",");
	tempString.concat("\"manufacturer\":\"");
	tempString.concat(String(device->_manufacturer));
	tempString.concat("\",");
	tempString.concat("\"name\":\"");
	tempString.concat(String(device->_name));
	tempString.concat("\",");

	tempString.concat("\"tokens\":\"");
	searchToken = device->_additionalSearchTokens;
	while(searchToken != 0) {
		tempString.concat(String(searchToken->_token));

		extraTokens.concat(",");
		extraTokens.concat("\"");
		tempExtraTokens = String(searchToken->_token);
		tempExtraTokens.toLowerCase();
		extraTokens.concat(tempExtraTokens);
		extraTokens.concat("\"");

		searchToken = searchToken->_next;
		if(searchToken != 0)
			tempString.concat(" ");
	}
	tempString.concat("\",");

	tempString.concat("\"device\":{");
	tempString.concat("\"name\":\"");
	tempString.concat(String(device->_name)+"\",");

	tempString.concat("\"tokens\":[");
	searchToken = device->_additionalSearchTokens;
	while(searchToken != 0) {
		tempString.concat("\"");
		tempString.concat(String(searchToken->_token));
		searchToken = searchToken->_next;
		tempString.concat("\"");
		if(searchToken != 0)
			tempString.concat(",");
	}
	tempString.concat("]},");

	tempString.concat("\"setup\":{},");
	tempString.concat("\"timing\":{},");

	tempString.concat("\"capabilities\":[");

	buttons = device->_buttons;
	while(buttons != 0) {
		tempString.concat("{");
		tempString.concat("\"type\":\"button\",");
		tempString.concat("\"name\":\"");
		tempString.concat(_urlencode(String(buttons->_name)));
		tempString.concat("\",");
		tempString.concat("\"label\":\"");
		tempString.concat(String(buttons->_label));
		tempString.concat("\",");
		tempString.concat("\"path\":\"/device/");
		tempString.concat(String(device->_adapterName));
		tempString.concat("/");
		tempString.concat(_urlencode(String(buttons->_name)));
		tempString.concat("\"");
		tempString.concat("}");
		buttons = buttons->_next;

		if(buttons != 0)
			tempString.concat(",");
	}

	tempString.concat("],");

	tempString.concat("\"deviceCapabilities\":[]");

	extraTokens.concat("]");

	if(addTokens == 1) {
		tempString.concat(",");
		tempString.concat(extraTokens);
	}

	tempString.concat("}");

	return tempString;
}

String ESP8266NeeoSDK::_urlencode(String str)
{
	String encodedString="";
	char c;
	char code0;
	char code1;
	char code2;
	for (int i =0; i < str.length(); i++){
		c=str.charAt(i);
		if (isalnum(c)){
			encodedString+=c;
		} else{
			code1=(c & 0xf)+'0';
			if ((c & 0xf) >9){
				code1=(c & 0xf) - 10 + 'A';
			}
			c=(c>>4)&0xf;
			code0=c+'0';
			if (c > 9){
				code0=c - 10 + 'A';
			}
			code2='\0';
			encodedString+='%';
			encodedString+=code0;
			encodedString+=code1;
		}
  		yield();
	}

	return encodedString;
}

String ESP8266NeeoSDK::_urldecode(String str)
{

    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (_h2int(code0) << 4) | _h2int(code1);
        encodedString+=c;
      } else{

        encodedString+=c;
      }

      yield();
    }

   return encodedString;
}

unsigned char ESP8266NeeoSDK::_h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
