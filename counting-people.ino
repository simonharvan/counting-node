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

// SELECT * FROM Logs WHERE createdAt BETWEEN '2019-04-08 10:06:00' AND '2019-04-08 10:07:00';

#include <Wire.h>
#include <Arduino.h>
#include <math.h> 
#include <string.h>
#include <stdlib.h>

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

void sendData(char* data) {
	if ((WiFiMulti.run() == WL_CONNECTED)) {

		WiFiClient client;

		HTTPClient http;

		Serial.print("[HTTP] begin...\n");
		if (http.begin(client, "http://147.175.121.212/api/logs")) {  // HTTP

			http.addHeader("Content-Type", "application/json");
			http.addHeader("x-access-token", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE1NTQ3MDk3OTN9.EhxH-cCCwKgveHTU8VosPQnLUB1aLSpIuXOzrSgwiCc");
			Serial.print("[HTTP] POST...\n");
			
			// start connection and send HTTP header
			int httpCode = http.POST(data);

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

void reverse(char *str, int len) { 
	int i=0, j=len-1, temp; 
	while (i<j) { 
		temp = str[i]; 
		str[i] = str[j]; 
		str[j] = temp; 
		i++; j--; 
	} 
} 

int intToStr(int x, char str[], int d) { 
	int i = 0; 
	while (x) { 
		str[i++] = (x%10) + '0'; 
		x = x/10; 
	} 

	// If number of digits required is more, then 
	// add 0s at the beginning 
	while (i < d) 
		str[i++] = '0'; 

	reverse(str, i); 
	str[i] = '\0'; 
	return i; 
} 

void ftoa(float n, char *res, int afterpoint) { 
// Extract integer part 
	int ipart = (int)n; 

	// Extract floating part 
	float fpart = n - (float)ipart; 

	// convert integer part to string 
	int i = intToStr(ipart, res, 0); 

	// check for display option after point 
	if (afterpoint != 0) { 
		res[i] = '.';  // add dot 

		// Get the value of fraction part upto given no. 
		// of points after dot. The third parameter is needed 
		// to handle cases like 233.007 
		fpart = fpart * pow(10, afterpoint); 

		intToStr((int)fpart, res + i + 1, afterpoint); 
	} 
} 


// 19.39,13.25,26.06,16.32,19.97,17.29,25.66,19.44,21.07,19.56,26.50,20.63,22.58,20.36,27.58,21.61,21.28,20.68,25.97,20.73,21.08,20.65,26.06,21.77,20.21,21.28,26.17,21.74,20.07,20.54,26.62,21.29,13.31,19.64,15.81,25.94,16.45,20.52,17.50,26.66,18.41,21.44,20.03,27.63,20.65,22.70,21.96,27.47,20.47,21.38,20.94,26.40,20.83,21.77,20.90,26.54,20.78,21.25,20.95,26.85,20.48,20.51,20.63,27.80,19.38,15.36,25.98,17.14,20.47,17.84,26.11,19.24,21.85,20.21,27.75,22.21,23.22,22.02,27.42,22.95,22.56,22.22,26.20,22.19,20.65,21.32,26.15,21.56,20.91,21.38,26.23,21.96,21.21,21.18,26.25,21.74,14.57,19.51,16.72,26.24,17.26,21.01,18.55,26.54,19.67,22.17,21.33,28.66,22.05,23.35,21.92,27.48,21.55,22.85,22.22,26.80,20.75,21.82,21.64,26.60,21.00,21.32,21.11,26.31,20.92,20.46,20.92,27.33,20.55,16.44,25.66,18.65,20.71,18.34,26.16,19.85,22.47,21.62,27.75,23.25,23.38,22.54,27.58,23.20,23.25,23.17,27.42,22.86,21.38,21.64,25.85,21.92,21.35,21.08,26.12,22.41,20.49,21.80,26.10,21.48,16.64,20.66,17.02,26.28,17.62,20.82,19.03,26.39,20.74,22.78,22.13,27.96,21.89,23.66,22.44,27.97,22.47,23.93,23.29,29.12,21.47,21.73,22.01,26.10,20.86,21.02,21.41,26.56,21.01,21.10,21.07,26.71,20.89,17.55,25.88,18.32,20.46,18.74,26.36,20.64,23.00,22.17,27.75,22.73,23.36,22.86,29.50,24.55,24.06,23.34,29.35,25.34,22.61,22.02,26.40,22.33,21.61,21.76,25.63,22.28,21.16,21.73,26.37,22.33,16.57,21.16,17.79,26.14,18.46,21.65,19.98,27.12,21.71,23.66,22.07,27.48,22.58,25.09,25.55,30.90,24.58,25.72,25.01,29.56,22.83,21.95,21.42,26.33,21.14,21.51,21.66,26.80,21.03,21.03,21.55,27.17,20.49,17.76,26.09,19.30,20.92,19.83,26.39,20.97,24.10,23.10,27.25,22.46,25.07,24.89,28.83,25.87,26.79,26.51,29.30,25.21,22.61,23.09,26.12,22.42,21.10,22.16,26.01,22.50,21.27,21.42,26.14,22.21,17.41,20.46,18.33,26.37,18.78,21.07,20.30,26.98,22.46,23.37,21.94,27.96,23.52,25.77,23.86,28.42,26.36,26.74,25.30,28.60,23.48,23.34,22.50,26.53,21.35,22.09,22.09,26.44,20.94,21.34,21.29,26.97,20.10,18.93,25.56,19.82,21.51,20.66,27.27,22.12,23.47,22.48,26.95,22.71,23.56,23.86,26.35,23.05,23.54,23.55,28.07,24.47,24.25,24.89,28.42,24.17,21.50,22.14,26.39,22.76,21.34,22.33,26.18,21.86,18.30,20.53,19.64,26.76,20.30,22.24,22.54,28.65,22.40,23.27,21.64,26.70,22.35,22.68,22.13,26.90,22.47,23.18,23.10,27.83,23.35,24.66,24.88,28.17,21.91,21.90,21.96,26.65,21.77,21.25,21.60,27.39,20.38,19.04,26.52,20.83,23.59,23.37,28.96,24.40,24.42,23.34,26.65,22.28,22.25,21.81,26.54,22.68,22.73,22.89,27.92,23.64,24.49,24.65,28.74,25.02,22.06,22.56,26.25,22.09,21.19,21.83,25.81,22.52,18.34,21.43,20.49,28.43,23.30,24.21,23.30,28.83,23.35,24.77,22.67,26.88,21.34,22.07,22.08,26.90,22.48,22.68,23.07,28.92,25.01,25.07,25.41,28.98,21.94,21.95,22.09,27.02,21.42,21.11,21.72,26.78,20.78,19.18,28.04,23.33,23.45,22.72,28.47,24.03,24.88,25.06,29.68,24.75,22.90,22.62,26.85,22.80,23.08,22.75,29.31,26.21,25.74,25.72,28.12,24.23,21.49,22.42,26.39,22.39,20.89,22.10,26.03,22.15,19.18,21.01,21.65,28.83,23.16,23.56,23.38,29.03,24.12,25.50,25.32,29.27,23.45,23.67,23.31,27.71,23.13,24.60,26.01,30.65,25.62,25.18,24.09,27.06,22.07,22.00,22.42,26.84,21.30,21.39,21.88,27.19,20.46,19.90,26.55,21.63,23.90,22.64,29.03,25.22,25.68,25.36,29.26,25.20,24.10,24.47,27.64,24.19,24.64,24.97,29.82,26.38,23.57,24.08,26.34,22.96,21.62,22.33,26.05,23.00,21.06,22.53,26.23,22.75,19.24,20.89,20.52,27.00,21.93,24.03,24.11,29.92,25.25,26.29,25.48,29.63,24.00,24.41,24.00,28.51,24.54,25.54,25.53,28.35,22.62,22.34,22.37,26.41,21.48,21.71,21.64,25.92,21.48,20.98,21.73,27.23,20.06,19.35,26.15,19.85,21.77,21.98,27.69,24.19,24.86,25.30,29.30,25.68,24.41,24.42,27.67,23.90,23.18,23.82,26.05,22.85,21.69,22.09,26.17,22.45,21.17,21.93,26.21,22.69,20.74,21.52,25.99,22.09,18.98,20.62,19.89,25.63,20.24,21.90,22.07,27.60,23.21,24.08,24.82,28.08,22.96,22.73,22.67,26.77,21.91,22.27,22.31,26.44,22.07,21.84,21.94,26.38,21.57,21.47,22.03,26.66,21.61,20.99,20.91,26.32,20.30,19.76,26.35,20.59,20.99,21.24,26.11,21.98,21.28,21.39,26.36,22.54,21.76,21.60,26.20,22.17,21.76,21.59,25.85,22.75,21.61,22.01,25.76,22.63,20.96,22.06,26.15,22.50,20.09,21.70,25.72,21.89,19.67,20.38,20.01,26.62,20.44,20.76,20.94,26.73,20.74,21.83,21.72,26.27,21.60,22.05,21.97,26.30,21.43,21.12,21.93,26.94,21.63,21.53,22.20,26.36,21.17,20.99,21.32,26.25,21.19,20.44,20.56,27.26,19.59,19.76,26.56,20.82,21.02,20.47,25.83,21.20,21.55,21.57,26.34,22.39,21.52,22.00,26.00,22.68,20.97,21.77,25.99,22.69,21.65,21.81,25.87,22.35,20.20,21.12,26.15,21.97,19.97,20.95,25.64,21.48,18.78,20.75,19.77,26.89,20.71,21.34,20.53,26.96,20.67,21.94,22.07,26.45,21.25,21.79,21.40,26.64,21.62,21.17,21.56,26.28,21.36,20.99,21.79,26.37,20.89,21.50,21.09,26.48,20.75,20.41,20.95,26.59