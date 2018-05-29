# ESP8266NeeoSDK
**ESP8266NeeoSDK** is a very basic implementation of the SDK provided by Neeo Inc for the Neeo Thinking Remote.   Their SDK is written in node.js and is designed to run on a PC or, at minimum, on a Raspberry Pi.

The original code can be found here https://github.com/NEEOInc/neeo-sdk
Examples for the SDK can be found here https://github.com/NEEOInc/neeo-sdk-examples

**Note:** This is a work in progress and is extreme alpha right now!

## Installation
Copy the folder 'ESP8266NeeoSDK' into your Arduino `Libraries` folder, as described in the [Arduino documentation](<http://arduino.cc/en/Guide/Libraries>).

## Limitations
This implementation does not include any of the idiot checking that is found in the node.js version.

At this time only type 'button' works.
No sensors, no sliders, no imageurls.

## Usage 

At the top of your sketch you must include the **ESP8266NeeoSDK** header file

```C
    #include <ESP8266NeeoSDK.h>
```

At the global scope you need to define the server port and the server name.   
And then instantiate buth a **ESP8266NeeoSDK** and **ESP8266WebServer**.

```C
    #define NEEOSERVERPORT 3663
    #define NEEOSERVERNAME "exampledevice"

    ESP8266NeeoSDK neeoRemote;
    ESP8266WebServer neeoServer(NEEOSERVERPORT);
```

Once WiFi is up and you have an IP address, initialise the remote and find the Neeo brain.

```C
    neeoRemote.begin(NEEOSERVERNAME, NEEOSERVERPORT);
    neeoRemote.findBrain();
```

findBrain does return a bool so you can confirm that you've located the Neeo brain.

Multiple devices can be added.   The first device is index 0, the second is index 1 and so on.
Once a device has been added you can add extra search tokens, button handler function and then add buttons.
All the add functions return bool so you can again check for success or failure.

```C
	int deviceIndex = neeoRemote.addDevice("Test Device", "Device Inc", "ACCESSOIRE");
	neeoRemote.addButtonHandler(deviceIndex, deviceButtonHandler);

    neeoRemote.addSearchTokens(deviceIndex, "testdevice");

	neeoRemote.addButton(deviceIndex, "POWER ON", "Power On");
	neeoRemote.addButton(deviceIndex, "POWER OFF", "Power Off");
```

Once the devices have been set up the server needs to be registered and started
```C
	if(neeoRemote.registerServer())
		Serial.println("Server Registered");
	else
		Serial.println("Register failed");

	if(neeoRemote.startServer(&neeoServer))
		Serial.println("Server Started");
	else
		Serial.println("Start failed");    
```

And all that is needed in the loop is
```C
    neeoRemote.loop();
```
