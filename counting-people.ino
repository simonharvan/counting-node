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

#include "MLX_count_people.h"

#define SDA_PIN 5 //is ok with 0 too
#define SCL_PIN 4

const int16_t MLX90640_address = 0x33; //Default 7-bit unshifted address of the MLX90640


#define TA_SHIFT 8 //Default shift for MLX90640 in open air

#define STDEVS_ARRAY 255;

static int AVG_TRAINING = 250;

static float mlx90640To[768];

static uint16_t mlx90640Frame0[835];
static uint16_t mlx90640Frame1[835];

paramsMLX90640 mlx90640;

// char textToSend[15000];
int trainingCycles = 0;
float maxOfBackground = -99999999999;
float minValue = 0, maxValue = 0;

static Man people[10];
static int peopleSize = 0;

ESP8266WiFiMulti WiFiMulti;

static struct Image images[IMAGE_NUM];
static int imagesIndex = 0; 
static long currentImage = 0;
static int in = 0, out = 0;
static float gaussians[768];

void doSomethingWithResult(float *numbers);
void sendData(char* text);
boolean isConnected();

void setupMLX90640() {
	if (isConnected() == false)
	{
		Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
		while (1);
	}

	//Get device parameters - We only have to do this once
	int status;
	int error = 1;
	uint16_t eeMLX90640[832];
	uint16_t controlRegister1;

	status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
	if (status != 0)
		Serial.println("Failed to load system parameters");

	status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
	if (status != 0)
		Serial.println("Parameter extraction failed");

	//Once params are extracted, we can release eeMLX90640 array
	MLX90640_SetRefreshRate(MLX90640_address, 0x05); //Set rate to 16Hz
	MLX90640_SetChessMode(MLX90640_address);
	

	error = MLX90640_I2CRead(MLX90640_address, 0x800D, 1, &controlRegister1);
	if(error != 0)
	{
	    Serial.println("MLX90640 setup wasn't possible.");
		while (1);
	} 

	controlRegister1 = (controlRegister1 | 0x0001);
	controlRegister1 = (controlRegister1 & 0xFFFB);

	error = MLX90640_I2CWrite(MLX90640_address, 0x800D, controlRegister1); 
    if(error != 0)
    {
    	Serial.println("MLX90640 control register not possible to write");
    } 
}

void setupWiFi() {

	for (uint8_t t = 4; t > 0; t--) {
		Serial.printf("[SETUP] WAIT %d...\n", t);
		Serial.flush();
		delay(1000);
	}
	// WiFi.forceSleepBegin(0);
	WiFi.mode(WIFI_STA);
	WiFiMulti.addAP("Muhaha", "woma2125");
}


void setup()
{
	delay(4000);
	Wire.begin(SDA_PIN, SCL_PIN);
	Wire.setClock(400000);

	Serial.begin(115200);
	while (!Serial); //Wait for user to open terminal

	setupMLX90640();
	setupWiFi();
	Wire.setClock(1000000);
}

int mode;
int status;
long startTime;
long stopTime;
// float stdevs[AVG_TRAINING];
// long stdevsCounter = 0;

void loop()
{
	
	startTime = millis();
	memset(mlx90640To, 0, sizeof(float) * 768);
	
	memset(mlx90640Frame0, 0, sizeof(uint16_t) * 835);
	memset(mlx90640Frame1, 0, sizeof(uint16_t) * 835);
	status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame0);
	if (status < 0)
	{
		Serial.print("GetFrame Error: ");
		Serial.println(status);
		return;
	}

	MLX90640_GetImage(mlx90640Frame0, &mlx90640, mlx90640To);
	
	delay(25);

   	status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame1);
	if (status < 0)
	{
		Serial.print("GetFrame Error: ");
		Serial.println(status);
		return;
	}
    
	MLX90640_GetImage(mlx90640Frame1, &mlx90640, mlx90640To);
	
	mode = MLX90640_GetCurMode(MLX90640_address);

    MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, mlx90640To, mode, &mlx90640);
    MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, mlx90640To, mode, &mlx90640);
	
	
	stopTime = millis();
	
	Serial.print("Read rate: ");
	Serial.print(stopTime - startTime);
	Serial.println(" ms");

	doSomethingWithResult();
}

void doSomethingWithResult() {
	float *numbers;
	startTime = millis();
	if (trainingCycles > AVG_TRAINING) {
		numbers = applyGaussian(mlx90640To, 32, 24);
		memcpy(numbers, gaussians, sizeof(gaussians));
		Serial.println("HERE");
		float stdDev = getStdDev(numbers, 768);
		if (stdDev > maxOfBackground) {
			Serial.println("HERE 2");
			printArray(numbers, 768);
			findMinMax(numbers, 768, &minValue, &maxValue);
			
			float threshold = findThreshold(numbers, 768, (maxValue - minValue) / 2);
			
			numbers = setThreshold(numbers, 768, threshold);
			
			imagesIndex = getIndexForImages(imagesIndex);
			peopleSize = images[imagesIndex].size = 0;
			int *detected = detectPeople(numbers, gaussians, 32, 24, images[imagesIndex].people, &peopleSize);
			images[imagesIndex].size = peopleSize;
			images[imagesIndex].time = millis();

			detectDirection(images, imagesIndex, &in, &out);
		
		
			printf("In - %d, Out - %d \n", in, out);

			imagesIndex++;
			currentImage++;
			
			
			for (int i = 0; i < peopleSize; ++i){
				printf("Man detected at x - %d, y - %d\n", people[i].x, people[i].y);
			}
			peopleSize = 0;
		}
	}else {
		trainingCycles++;
		if (trainingCycles < 5) {
			free(numbers);
			return;
		}
		
		numbers = applyGaussian(mlx90640To, 32, 24);
		float stdDev = getStdDev(numbers, 768);
		Serial.println(stdDev);
		if (stdDev > maxOfBackground){
			maxOfBackground = stdDev;
		}
	}
	stopTime = millis();
	Serial.print("Process rate: ");
	Serial.print(stopTime - startTime);
	Serial.println(" ms");
	free(numbers);

}

//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected() {

  	Wire.beginTransmission((uint8_t)MLX90640_address);
  	if (Wire.endTransmission() != 0)
    	return (false); //Sensor did not ACK
  	return (true);
}

void printArray(float *array, int size) {
	 for (int x = 0 ; x < size ; x++){
    	
    	Serial.print(array[x]);
    	Serial.print(",");
    }
    Serial.println("");
}

void printIntArray(int *array, int size) {
	 for (int x = 0 ; x < size ; x++){
    	
    	Serial.print(array[x]);
    	Serial.print(",");
    }
    Serial.println("");
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
