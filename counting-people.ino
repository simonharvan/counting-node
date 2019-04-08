/*
  Read the temperature pixels from the MLX90640 IR array
  By: Nathan Seidle
  SparkFun Electronics
  Date: May 22nd, 2018
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  Feel like supporting open source hardware?
  Buy a board from SparkFun! https://www.sparkfun.com/products/14769

  This example initializes the MLX90640 and outputs the 768 temperature values
  from the 768 pixels.

  This example will work with a Teensy 3.1 and above. The MLX90640 requires some
  hefty calculations and larger arrays. You will need a microcontroller with 20,000
  bytes or more of RAM.

  This relies on the driver written by Melexis and can be found at:
  https://github.com/melexis/mlx90640-library

  Hardware Connections:
  Connect the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  to the Qwiic board
  Connect the male pins to the Teensy. The pinouts can be found here: https://www.pjrc.com/teensy/pinout.html
  Open the serial monitor at 9600 baud to see the output
*/



#include <Wire.h>
#include <Arduino.h>
#include <math.h> 
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"


#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#define SDA_PIN 5 //is ok with 0 too
#define SCL_PIN 4

const int16_t MLX90640_address = 0x33; //Default 7-bit unshifted address of the MLX90640

#define TA_SHIFT 8 //Default shift for MLX90640 in open air

static float mlx90640To[768];
paramsMLX90640 mlx90640;

char textToSend[4800];
byte page = 0;
//long counter = 0;

ESP8266WiFiMulti WiFiMulti;

void setup()
{
	delay(1000);
	Wire.begin(SDA_PIN, SCL_PIN);
	Wire.setClock(400000);

	Serial.begin(115200);
	while (!Serial); //Wait for user to open terminal

	if (isConnected() == false)
	{
		Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
		while (1);
	}

	//Get device parameters - We only have to do this once
	int status;
	uint16_t eeMLX90640[832];

	status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
	if (status != 0)
		Serial.println("Failed to load system parameters");

	status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
	if (status != 0)
		Serial.println("Parameter extraction failed");

	//Once params are extracted, we can release eeMLX90640 array
	MLX90640_SetRefreshRate(MLX90640_address, 0x03); //Set rate to 8Hz

	for (uint8_t t = 4; t > 0; t--) {
		Serial.printf("[SETUP] WAIT %d...\n", t);
		Serial.flush();
		delay(1000);
	}

	WiFi.mode(WIFI_STA);
	WiFiMulti.addAP("Muhaha", "woma2125");
	Wire.setClock(1000000);
}


void loop()
{
	long startTime;
	long stopTime;
	startTime = millis();  
	for (byte i = 0; i < 2; ++i){
		uint16_t mlx90640Frame[835];
		memset(mlx90640Frame, 0, sizeof(uint16_t) * 835);
		int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
		if (status < 0)
		{
			Serial.print("GetFrame Error: ");
			Serial.println(status);
		}
	    
		float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
		float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);
		int mode = MLX90640_GetCurMode(MLX90640_address);
	    float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
	    float emissivity = 0.95;
		
	    MLX90640_GetImage(mlx90640Frame, &mlx90640, mlx90640To);
	    MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, mlx90640To, mode, &mlx90640);
	    MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, mlx90640To, mode, &mlx90640);
	}
	stopTime = millis();
	
	
	Serial.print("Read rate: ");
	Serial.print(stopTime - startTime);
	Serial.println(" ms");

    

   	doSomethingWithResult(mlx90640To);
}

void doSomethingWithResult(float *mlx90640To) {
	memset(textToSend, 0, sizeof(textToSend));
    strcat(textToSend, "{\"text\":\"");

    char tmp[7];
	
    for (int x = 0 ; x < 768 ; x++){
    	char s[11];
    	Serial.print(dtostrf(mlx90640To[x], 10, 2, s));
    	Serial.print(",");
    	// memset(tmp, 0, sizeof(tmp));
    	// ftoa(mlx90640To[x], tmp, 2);

    	// strcat(tmp, ",");
    	// strcat(textToSend, tmp);
    }
    // strcat(textToSend, "\"}");

    Serial.println("");

 	// sendData(textToSend);
}



//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected() {

  	Wire.beginTransmission((uint8_t)MLX90640_address);
  	if (Wire.endTransmission() != 0)
    	return (false); //Sensor did not ACK
  	return (true);
}

void sendData(char* text) {
	if ((WiFiMulti.run() == WL_CONNECTED)) {

		WiFiClient client;

		HTTPClient http;

		Serial.print("[HTTP] begin...\n");
		if (http.begin(client, "http://147.175.121.212/api/logs")) {  // HTTP

			http.addHeader("Content-Type", "application/json");
			http.addHeader("x-access-token", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE1NTQ3MDk3OTN9.EhxH-cCCwKgveHTU8VosPQnLUB1aLSpIuXOzrSgwiCc");
			Serial.print("[HTTP] POST...\n");
			cJSON *root = cJSON_CreateObject();
			cJSON_AddItemToObject(root, "text", cJSON_CreateString(text));

			
			// start connection and send HTTP header
			int httpCode = http.POST(cJSON_Print(root));

			// httpCode will be negative on error
			if (httpCode > 0) {
		  		// HTTP header has been send and Server response header has been handled
				Serial.printf("[HTTP] POST... code: %d\n", httpCode);

		  		// file found at server
				if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
					String payload = http.getString();
					Serial.println(payload);
				}
			} else {
				Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
			}
		}
    }

}
