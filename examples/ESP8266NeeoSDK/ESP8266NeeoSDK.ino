// Example for a very basic Neeo remote device

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266NeeoSDK.h>

#define HOSTNAME "esp8266-NeeoSDK"
const char* ssid = "........";
const char* password = ".........";

#define NEEOSERVERPORT 3663
#define NEEOSERVERNAME "exampledevice"

ESP8266NeeoSDK neeoRemote;
ESP8266WebServer neeoServer(NEEOSERVERPORT);

void deviceButtonHandler(String command) {
	Serial.print("Device Button Pressed: ");
	Serial.println(command);
}

void setup() {
	Serial.begin(115200);
	Serial.println("Booting");

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      	Serial.println("Connection Failed! Rebooting...");
      	delay(5000);
      	ESP.restart();
    }

	// Set OTA hostname
	ArduinoOTA.setHostname(HOSTNAME);

	// No authentication by default
	ArduinoOTA.setPassword((const char *)"neeo");

	ArduinoOTA.onStart([]() {
	  	Serial.println("Start");
	});
	ArduinoOTA.onEnd([]() {
	  	Serial.println("\nEnd");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
	  	Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
	  	else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();

	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	// Initialise the Neeo service
	neeoRemote.begin(NEEOSERVERNAME, NEEOSERVERPORT);

	// Find the brain
	if(neeoRemote.findBrain())
		Serial.println("Found Brain");
	else
		Serial.println("No Brain Found");

	// Add a device
	int deviceIndex = neeoRemote.addDevice("Test Device", "Device Inc", "ACCESSOIRE");

	// Add the button handler
	neeoRemote.addButtonHandler(deviceIndex, deviceButtonHandler);

	// Add some more search tokens if needed
	neeoRemote.addSearchTokens(deviceIndex, "testdevice");

	// Add some buttons
	neeoRemote.addButton(deviceIndex, "POWER ON", "Power On");
	neeoRemote.addButton(deviceIndex, "POWER OFF", "Power Off");

	// Register the server with the brain
	if(neeoRemote.registerServer())
		Serial.println("Server Registered");
	else
		Serial.println("Register failed");

	// Start the server
	if(neeoRemote.startServer(&neeoServer))
		Serial.println("Server Started");
	else
		Serial.println("Start failed");
}


void loop() {

	ArduinoOTA.handle();

	if (WiFi.status() == WL_DISCONNECTED) {
      	//Serial.println("Wifi Disconnected");
      	delay(5000);
      	ESP.restart();
    }

	// Neeo server loop
	neeoRemote.loop();
}
