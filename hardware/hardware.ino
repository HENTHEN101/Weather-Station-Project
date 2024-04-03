//##################################################################################################################
//##                                      ELET2415 DATA ACQUISITION SYSTEM CODE                                   ##
//##                                                                                                              ##
//##################################################################################################################

 

// IMPORT ALL REQUIRED LIBRARIES
#include <rom/rtc.h>
#include <ArduinoJson.h>
#include <math.h>  // https://www.tutorialspoint.com/c_standard_library/math_h.htm

#include <SPI.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "DHT.h"


#ifndef _WIFI_H 
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#ifndef STDLIB_H
#include <stdlib.h>
#endif

#ifndef STDIO_H
#include <stdio.h>
#endif

#ifndef ARDUINO_H
#include <Arduino.h>
#endif 
 
// DEFINE VARIABLES
#define ARDUINOJSON_USE_DOUBLE      1 

// DEFINE THE CONTROL PINS FOR THE DHT22 
#define DHTPIN 4 //datapin of sensor
#define DHTTYPE DHT22 // define the type of sensor

// define data pin and wet to dry values for soil mositure semsor
#define soil 32
#define wet 2317
#define dry 3520

//deefine buttons 
#define BTN1      33
#define BTN2      27
#define BTN3      2
//define tft control and data pins
#define TFT_DC    17
#define TFT_CS    5
#define TFT_RST   16
#define TFT_CLK   18
#define TFT_MOSI  23
#define TFT_MISO  19

Adafruit_BMP280 bmp;

//----------------------------------------Defines colors
// Assign human-readable names to some common 16-bit color values:
#define BLACK       0x0000
#define BLUE        0x001F
#define LTBLUE		  0x73DF
#define RED         0xF800
#define GREEN       0x07E0
#define DKGREEN     0x04A3
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define DARKORANGE  0xFB60
#define MAROON      0x7800
#define DKGREY      0x4A49
#define LTGREY      0xE71C

double h;
double t;
double f;
double hif;
double hic;
int m;
double p;
double E;
float i=0;
float j=0;
float prepress =0;
float currpress =100;

//The x and y points for the Temperature bar
int x_bar_t = 20;
int y_bar_t = 65;

//-The variable to hold the conversion value from the temperature value to the value for the temperature bar
int T_to_Bar;
int pre;

//Menu = 0 to display the Main Menu Display, Menu = 1 to show temp an humid and Menu = 2 to show soil mositure and Menu=3 to show BMP data
int Menu = 0;
//----------------------------------------


// MQTT CLIENT CONFIG  
static const char* pubtopic      = "620152511";                    // Add your ID number here
static const char* subtopic[]    = {"620152511_sub","/elet2415"};  // Array of Topics(Strings) to subscribe to
static const char* mqtt_server   = "www.yanacreations.com";         // Broker IP address or Domain name as a String 
static uint16_t mqtt_port        = 1883;

// WIFI CREDENTIALS
const char* ssid       = "MonaConnect"; // Add your Wi-Fi ssid
const char* password   = ""; // Add your Wi-Fi password 



// TASK HANDLES 
TaskHandle_t xMQTT_Connect          = NULL; 
TaskHandle_t xNTPHandle             = NULL;  
TaskHandle_t xLOOPHandle            = NULL;  
TaskHandle_t xUpdateHandle          = NULL;
TaskHandle_t xButtonCheckeHandle    = NULL; 



// FUNCTION DECLARATION   
void checkHEAP(const char* Name);   // RETURN REMAINING HEAP SIZE FOR A TASK
void initMQTT(void);                // CONFIG AND INITIALIZE MQTT PROTOCOL
unsigned long getTimeStamp(void);   // GET 10 DIGIT TIMESTAMP FOR CURRENT TIME
void callback(char* topic, byte* payload, unsigned int length);
void initialize(void);
bool publish(const char *topic, const char *payload); // PUBLISH MQTT MESSAGE(PAYLOAD) TO A TOPIC
void vButtonCheck( void * pvParameters );
void vUpdate( void * pvParameters ); 


/* Init class Instances for the DHT22 etcc */
 DHT dht(DHTPIN,DHTTYPE);

//############### IMPORT HEADER FILES ##################
#ifndef NTP_H
#include "NTP.h"
#endif

#ifndef MQTT_H
#include "mqtt.h"
#endif


/* Initialize class objects*/
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

void setup() {
  Serial.begin(115200);  // INIT SERIAL  

  /* TFT DISPLAY SET UP */
  dht.begin();
  tft.begin();
 unsigned status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
    }

    /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  /* ENABLE PULL-UP RESISTORS */
  pinMode(BTN1,INPUT_PULLUP);
  pinMode(BTN2,INPUT_PULLUP);
  pinMode(BTN3,INPUT_PULLUP);

  tft.fillScreen(WHITE);
  Menu_display();

  initialize();           // INIT WIFI, MQTT & NTP 

  vButtonCheckFunction(); // UNCOMMENT IF USING BUTTONS THEN ADD LOGIC FOR INTERFACING WITH BUTTONS IN THE vButtonCheck FUNCTION

}

void loop() {
  // put your main code here, to run repeatedly:
 
//----------------------------------------Menu or Mode to control the LED
  if (Menu == 1) {
    ShowDHT11Data();
  }

  //----------------------------------------Menu or Mode to display DHT11 sensor data
  if (Menu == 2) {
    showSoilData();
  }

  if (Menu == 3) {
    showBMPData();
  }
  //----------------------------------------

  vTaskDelay(1000 / portTICK_PERIOD_MS);  
  
}

  
//####################################################################
//#                          UTIL FUNCTIONS                          #       
//####################################################################
void vButtonCheck( void * pvParameters )  {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );     
      
    for( ;; ) {
        // Add code here to check if a button(S) is pressed
        // then execute appropriate function if a button is pressed  

        // 1. Implement button1  functionality
        if(!digitalRead(BTN1)){
          delay(500);
          Menu = 1;
          tft.fillScreen(WHITE);
          delay(500);
          
          tft.drawLine(15, 50, 300, 50, MAROON);
          tft.drawLine(15, 51, 300, 51, MAROON);

          tft.setTextSize(2);
          tft.setTextColor(BLUE);
          tft.setCursor(42, 13);
          tft.print("Temperature &         Humidity");
          
          draw_bar(x_bar_t, y_bar_t);
        
          tft.drawLine(123, 65, 123, 250, MAROON);
          tft.drawLine(123, 132, 320, 132, MAROON);

          tft.fillRect(130, 65, 100, 27, CYAN);
          tft.setTextSize(2);
          tft.setTextColor(WHITE);
          tft.setCursor(133, 70);
          tft.print("Humidity");

          tft.fillRect(130, 145, 100, 43, GREEN);
          tft.setTextSize(2);
          tft.setTextColor(WHITE);
          tft.setCursor(155, 150);
          tft.print("Heat");
          tft.setCursor(148, 170);
          tft.print("Index");
          delay(1000);
        }       
        // 2. Implement button2  functionality

        if(!digitalRead(BTN2)){
          delay(500);
          Menu =2;
          tft.fillScreen(WHITE);
          delay(500);

          tft.drawLine(15, 30, 300, 30, MAROON);
          tft.drawLine(15, 31, 300, 31, MAROON);

          tft.setTextSize(2);
          tft.setTextColor(BLUE);
          tft.setCursor(42, 13);
          tft.print("Soil Moisture");

          draw_moisture_sensor();
          
          delay(1000);
        }

        // 3. Implement button3  functionality

        if(!digitalRead(BTN3)){
          delay(500);
          Menu = 3;
          tft.fillScreen(BLACK);
          delay(500);


          tft.drawLine(15, 50, 300, 50, MAROON);
          tft.drawLine(15, 51, 300, 51, MAROON);

          tft.setTextSize(2);
          tft.setTextColor(BLUE);
          tft.setCursor(42, 13);
          tft.print("Pressure and          Altitude");

          draw_pressure_meter();
          delay(1000);
        }
        
        vTaskDelay(200 / portTICK_PERIOD_MS);  
        }
}

void vUpdate( void * pvParameters )  {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );    
 
    for( ;; ) {

      GetDHT11Data();
      GetSoilData();
      GetBMPData();

      if(isNumber(t)){
        if(isNumber(p)){
              // ##Publish update according to ‘{"id": "student_id", "timestamp": 1702212234, "temperature": 30, "humidity":90, "heatindex": 30}’

              // 1. Create JSon object
              StaticJsonDocument<1000> doc;
              
              // 2. Create message buffer/array to store serialized JSON object
              char message[1100]={0};
              
              // 3. Add key:value pairs to JSon object based on above schema
              doc["id"] = "620152511";
              doc["timestamp"] = getTimeStamp();
              doc["temperature"] = t;
              doc["humidity"] = h;
              doc["heatindex"] = hic;
              doc["soilmoisture"]=pre;
              doc["pressure"]=p;
              doc["altitude"]=E;

              // 4. Seralize / Covert JSon object to JSon string and store in message array

              serializeJson(doc,message);
               
              // 5. Publish message to a topic sobscribed to by both backend and frontend
              if(mqtt.connected()){
                publish(pubtopic,message);
              }  
           }            

          }

        vTaskDelay(1000 / portTICK_PERIOD_MS);  
    }
}

unsigned long getTimeStamp(void) {
  // RETURNS 10 DIGIT TIMESTAMP REPRESENTING CURRENT TIME
  time_t now;         
  time(&now); // Retrieve time[Timestamp] from system and save to &now variable
  return now;
}

void callback(char* topic, byte* payload, unsigned int length) {
  // ############## MQTT CALLBACK  ######################################
  // RUNS WHENEVER A MESSAGE IS RECEIVED ON A TOPIC SUBSCRIBED TO
  
  Serial.printf("\nMessage received : ( topic: %s ) \n",topic ); 
  char *received = new char[length + 1] {0}; 
  
  for (int i = 0; i < length; i++) { 
    received[i] = (char)payload[i];    
  }

  // PRINT RECEIVED MESSAGE
  Serial.printf("Payload : %s \n",received);

}

bool publish(const char *topic, const char *payload){   
     bool res = false;
     try{
        res = mqtt.publish(topic,payload);
        // Serial.printf("\nres : %d\n",res);
        if(!res){
          res = false;
          throw false;
        }
     }
     catch(...){
      Serial.printf("\nError (%d) >> Unable to publish message\n", res);
     }
  return res;
}

void draw_pressure_meter(){
  tft.drawCircle(126,139,80,ORANGE);
  tft.fillRect(106,219,40,30,ORANGE);
  tft.fillRect(22,249,200,50,BLUE);
  tft.fillRect(40,253,160,43,WHITE);
  tft.fillRect(0,240,30,70,ORANGE);
  tft.fillRect(210,240,30,70,ORANGE);


  tft.setTextSize(2);
  tft.setTextColor(GREEN,BLACK);
  tft.setCursor(82, 130);
  tft.print("Pressure");

  tft.setTextColor(RED,WHITE);
  tft.setCursor(70,255);
  tft.print("Altitude");
}

//========================================================================draw_bar (Temperature Bar)
void draw_bar(int x_bar, int y_bar) {
  tft.fillRoundRect(x_bar, y_bar, 35, 115, 5, BLACK);
  tft.fillCircle(x_bar+17, y_bar+140, 30, BLACK);
  tft.fillRoundRect(x_bar+4, y_bar+4, 27, 120, 2, WHITE);
  tft.fillCircle(x_bar+17, y_bar+140, 25, WHITE);
  tft.fillRect(x_bar+8, y_bar+8, 19, 120, DARKORANGE);
  tft.fillCircle(x_bar+17, y_bar+140, 21, DARKORANGE);

  //tft.fillRect(41, 58, 19, 108, RED);

  tft.drawLine(x_bar+37, y_bar+8, x_bar+42, y_bar+8, RED);
  tft.setTextSize(1);
  tft.setTextColor(RED);
  tft.setCursor(x_bar+47, y_bar+4);
  tft.println("100");
  
  tft.drawLine(x_bar+37, y_bar+115, x_bar+42, y_bar+115, RED);
  tft.setCursor(x_bar+47, y_bar+111);
  tft.println("0");
}
//========================================================================
void draw_moisture_sensor(){
  tft.fillRoundRect(0,230,240,100,10,ORANGE);
  tft.fillRoundRect(0,230,240,20,1,DKGREEN);

  for (int i = 0; i < 100; i++) { // Adjust the number of circles as needed
    int x = random(0, 240); // Generate random x coordinate within the rectangle
    int y = random(255, 330); // Generate random y coordinate within the rectangle and a bit below
    int radius = 2; // Set the radius of the circle
    tft.fillCircle(x, y, radius, YELLOW); // Draw the circle at the random coordinates
  }

  tft.fillRoundRect(48,108,104,104,10,BLACK);
  tft.fillRoundRect(50,110,100,100,10,GREEN);


  tft.fillRoundRect(58,118,84,54,5,BLACK);
  tft.fillRoundRect(60,120,80,50,5,LTBLUE);

  tft.fillRect(65,210,20,80,DKGREY);
  tft.fillRect(115,210,20,80,DKGREY);

  tft.fillTriangle(65,290,75,300,83,290,DKGREY);
  tft.fillTriangle(115,290,125,300,133,290,DKGREY);
  
  // Draw half circles instead of rectangles
  int centerX1 = 60;
  int centerY1 = 280;
  int centerX2 = 140;
  int centerY2 = 280;
  int count=0;
  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(62, 122);
  tft.print("Scanning..");
  tft.fillRect(62,145,75,20,BLACK);

  for(int L=20;L <=60;L+=20){
    tft.fillRect(64,147, (0)+L,15, GREEN);
      //for(int i = 0; i < 2; i++){
        for(int radius =5;radius<=30;radius+=10){
        // Draw left half circle
          for(float angle = PI/2; angle <= 3*PI/2; angle += 0.01){
          int x = centerX1 + radius * cos(angle);
          int y = centerY1 - radius * sin(angle);
          int x1 = centerX2 - radius * cos(angle);
          int y2 = centerY2 - radius * sin(angle);
          tft.drawPixel(x, y, BLACK);
          tft.drawPixel(x1, y2, BLACK);
        }
        }
        tft.fillRect(22,255,40,60,ORANGE);
        tft.fillRect(140,255,40,60,ORANGE);
        for (int i = 0; i < 15; i++) { // Adjust the number of circles as needed
          int x = random(26, 56); // Generate random x coordinate within the rectangle
          int y = random(259, 311); // Generate random y coordinate within the rectangle and a bit below
          int x1 = random(144, 176); // Generate random x coordinate within the rectangle
          int y2 = random(259, 311);
          int radius = 2; // Set the radius of the circle
          tft.fillCircle(x, y, radius, YELLOW); // Draw the circle at the random coordinates
          tft.fillCircle(x1, y2, radius, YELLOW); // Draw the circle at the random coordinates
        }
  }
      delay(500);
      tft.fillRoundRect(60,120,80,50,5,LTBLUE);

      tft.setTextSize(2);
      tft.setTextColor(WHITE);
      tft.setCursor(70, 125);
      tft.print("Level");
}


void showSoilData(){
  pre = map(m,wet,dry,100,0);
  tft.setTextSize(2);
  tft.setTextColor(CYAN,LTBLUE);
  tft.setCursor(65, 150);
  tft.print(pre,0);
  tft.print("% ");
}

void showBMPData(){
  i =map(prepress,0,110000,0,300);
  j =map(currpress,0,110000,0,300);

  for (i; i<=j; i=i+0.1)
    {
      float j=i-150 ;
      float angle = (j / 57.2958)  - 1.57;
      float x1= 80 + cos(angle) * 55;
      float y1 = 130+sin(angle) * 55;
      float x2= 80 + cos(angle) * 75;
      float y2 = 130+sin(angle) * 75;
      tft.drawLine(x1+45, y1+10,x2+45,y2+10, GREEN);
    }
  for (i-2; i>=j; i=i-0.05) {
    float j=i-150 ;
    float angle = (j / 57.2958)  - 1.57;
    int x1 = 80 + cos(angle) * 55;
    int y1 = 130 + sin(angle) * 55;
    int x2 = 80 + cos(angle) * 75; // Increase radius to make a thicker line
    int y2 = 130 + sin(angle) * 75; // Increase radius to make a thicker line
    tft.drawLine(x1+45, y1+10, x2+45, y2+10, BLACK);
    }
    prepress=currpress;
    currpress=p;

  tft.setTextSize(1);
  tft.setCursor(90, 160);
  tft.setTextColor(GREEN,BLACK);
  tft.print(p);
  tft.print(" Pa ");
  
  tft.setTextSize(2);
  tft.setTextColor(RED,WHITE);
  tft.setCursor(75,278);
  tft.print(E);
  tft.print(" m ");
}
//========================================================================
void ShowDHT11Data() {


  T_to_Bar = map(t, 0.0, 100.0, 105, 0);

  tft.fillRect(x_bar_t+8, (y_bar_t+8)+T_to_Bar, 19, 100-T_to_Bar, DARKORANGE);
  tft.fillRect(x_bar_t+8, y_bar_t+8, 19, T_to_Bar, WHITE);

  tft.setTextSize(2);
  tft.setTextColor(ORANGE, WHITE);
  tft.setCursor(57, 105);
  tft.print(t,1);
  if (t < 10) tft.print(" ");
  tft.setCursor(97, 105);
  tft.print((char)247);
  tft.println("C");

  
  tft.setCursor(57, 140);
  tft.print(f,1);
  if (f < 100) tft.print(" ");
  tft.setCursor(97, 140);
  tft.print((char)247);
  tft.println("F");

  tft.setTextSize(2);
  tft.setTextColor(CYAN, WHITE);
  tft.setCursor(130, 100);
  tft.print(h,1);
  tft.print("% ");

  tft.setTextSize(2);
  tft.setTextColor(GREEN, WHITE);
  tft.setCursor(130, 205);
  tft.print(hic,1);
  tft.print(" ");
  tft.print((char)247);
  tft.print("C ");

  tft.setTextSize(2);
  tft.setTextColor(GREEN, WHITE);
  tft.setCursor(130, 225);
  tft.print(hif,1);
  tft.print(" ");
  tft.print((char)247);
  tft.print("F "); 
}

void Menu_display() {
  tft.setTextSize(3);
  tft.setTextColor(BLACK);
  tft.setCursor(80, 43);
  tft.print("MENU"); 

  tft.setTextSize(2);
  tft.setTextColor(BLACK);
  tft.fillRect(20,80,200,30,ORANGE);
  tft.setCursor(23,85);
  tft.print("DHT22 Data-BTN1");
  tft.fillRect(20,135,200,30,GREEN);
  tft.setCursor(23,140);
  tft.print("Soil Data-BTN2");
  tft.fillRect(20,190,200,30,YELLOW);
  tft.setCursor(23,195);
  tft.print("BMP280 Data-BTN3"); 
}

void GetBMPData(){
  p = bmp.readPressure();
  Serial.println(p);
  //1014.8 hpa pressure is gain  from mathhew davis weather staion
  E = bmp.readAltitude(1014.8);
  Serial.println(E);
  if(isnan(p) || isnan(E)){
    return;
  }
}

void GetSoilData(){
  m = analogRead(soil);
  Serial.println(m);
  if(isnan(m)){
    return;
  }
}

//========================================================================GetDHT11Data()
void GetDHT11Data() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  Serial.println(h);
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  Serial.println(t);
  // Read temperature as Fahrenheit (isFahrenheit = true)
  f = dht.readTemperature(true);
  Serial.println(f);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  hif = dht.computeHeatIndex(f, h);
  Serial.println(hif);
  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(t, h, false);
  Serial.println(hic);
}
//========================================================================

bool isNumber(double num1){       
        char item[20];
        snprintf(item, sizeof(item), "%f\n", num1);
        if( isdigit(item[0]) )
          return true;
        return false; 
} 



