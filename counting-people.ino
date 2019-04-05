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

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"


#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#define SDA_PIN 2
#define SCL_PIN 3

const int16_t MLX90640_address = 0x33; //Default 7-bit unshifted address of the MLX90640

#define TA_SHIFT 8 //Default shift for MLX90640 in open air

static float mlx90640To[768];
uint16_t mlx90640Frame[834];
paramsMLX90640 mlx90640;

char textToSend[4800];
//long counter = 0;

ESP8266WiFiMulti WiFiMulti;

void setup()
{
  delay(1000);
  Wire.begin(0, 4);
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
  MLX90640_SetRefreshRate(MLX90640_address, 0x04); //Set rate to 8Hz

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
  long startTime = millis();
  long stopTime;
  for (byte x = 0 ; x < 2 ; x++)
  {
    mlx90640Frame[0] = 0;
    mlx90640Frame[33] = 0;
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
    stopTime = millis();
    float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
    float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

    float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
    float emissivity = 0.95;

    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
  }
  

  Serial.print("Read rate: ");
  Serial.print( stopTime - startTime);
  Serial.println(" ms");

  memset(textToSend, 0, sizeof(textToSend));
  strcat(textToSend, "{\"text\":\"");
 
  char tmp[7];
 
  for (int x = 0 ; x < 768 ; x++)
  {
    Serial.print(mlx90640To[x], 2);
    Serial.print(",");
    memset(tmp, 0, sizeof(tmp));
    ftoa(mlx90640To[x], tmp, 2);
    
    strcat(tmp, ",");
    strcat(textToSend, tmp);
  }
  strcat(textToSend, "\"}");
  
  Serial.println("");
  
//  sendData(textToSend);
  
  
//  counter++;
}

//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected()
{
  
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
        http.addHeader("x-access-token", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE1NTQ0NTI1NTEsImV4cCI6MTU1NDUzODk1MX0.wJrx4-i_y9v8bCfrPYoYrGNmSKctTABE_5MmgKunwfQ");
        Serial.print("[HTTP] POST...\n");
        // start connection and send HTTP header
        
        int httpCode = http.POST(data);
        
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);
  
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

void reverse(char *str, int len) 
{ 
    int i=0, j=len-1, temp; 
    while (i<j) 
    { 
        temp = str[i]; 
        str[i] = str[j]; 
        str[j] = temp; 
        i++; j--; 
    } 
} 

int intToStr(int x, char str[], int d) 
{ 
    int i = 0; 
    while (x) 
    { 
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

void ftoa(float n, char *res, int afterpoint) 
{ 
    // Extract integer part 
    int ipart = (int)n; 
  
    // Extract floating part 
    float fpart = n - (float)ipart; 
  
    // convert integer part to string 
    int i = intToStr(ipart, res, 0); 
  
    // check for display option after point 
    if (afterpoint != 0) 
    { 
        res[i] = '.';  // add dot 
  
        // Get the value of fraction part upto given no. 
        // of points after dot. The third parameter is needed 
        // to handle cases like 233.007 
        fpart = fpart * pow(10, afterpoint); 
  
        intToStr((int)fpart, res + i + 1, afterpoint); 
    } 
} 
