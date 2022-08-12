/* 
  board package: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
*/

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include "MPU9250.h"
#include <AutoPID.h>
#include <Wire.h>

// Replace with your network credentials
const char* ssid = "CRI-MAKERLAB";
const char* password = "--criMAKER--";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object

AsyncWebSocket ws("/ws");

AsyncWebSocketClient * globalClient = NULL;

/*    Roll
       ^ 
       |         _ Yaw
                 /| 
   -   |   -    / 
  |1|     |2|
   -\     /-
     \ - /
     |   |   - - > Pitch
     /[_]\
   -/     \-
  |4|     |3|
   -       -


*/

//without usb cable
const int motor1 = 26;
const int motor2 = 27;
const int motor3 = 14;
const int motor4 = 12;
/*
//with usb cable
const int motor1 = 27;
const int motor2 = 14;
const int motor3 = 12;
const int motor4 = 26;*/

bool isOff = true;

String message = "";
String sliderValue1 = "0";
String sliderValue2 = "0";
String sliderValue3 = "0";
String sliderValue4 = "0";
String sliderValue5 = "0";

int dutyCycle1, dutyCycle2, dutyCycle3, dutyCycle4;
int speed1, speed2, speed3, speed4;

//setting speed properties (values are percentages)
const int primaryThrust = 90;    //229.5
const int secondaryThrust = 45;  //114.75
const int tertiaryThrust = 15;   //38.25

//setting angle properties (Pitch/Roll/Yaw)
//movement
#define SET_POINT_ROLL 150
#define SET_POINT_PITCH 150
#define SET_POINT_YAW 90

//idle
#define SET_POINT_ROLL_IDLE 180
#define SET_POINT_PITCH_IDLE 180
#define SET_POINT_YAW_IDLE 180

//setting PWM properties
const int freq = 5000;
const int motorChannel1 = 0; //MOTOR_1R
const int motorChannel2 = 1; //MOTOR_2R
const int motorChannel3 = 2; //MOTOR_2L
const int motorChannel4 = 3; //MOTOR_1L

const int resolution = 8;

int initialSpeed = 196;


// MPU variables
MPU9250 mpu;
float Acc_angle[3];
float Gyro_angle[3];
float Mag_angle[3];
float RPY[3];

// PID settings and gains
#define OUTPUT_MIN 0
#define OUTPUT_MAX 80
#define KP 0.12
#define KI .0003
#define KD 0.03
#define UPDATE_DELAY 10

double currentRoll, setPointR, outputRoll, delta;
double currentPitch, setPointP, outputPitch;
double currentYaw, setPointY, outputYaw;

//input/output variables passed by reference, so they are updated automatically
AutoPID RollPID(&currentRoll, &setPointR, &outputRoll, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);
AutoPID PitchPID(&currentPitch, &setPointP, &outputPitch, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);
AutoPID YawPID(&currentYaw, &setPointY, &outputYaw, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

float elapsedTime, currTime, prevTime; //Loop execution time

//Json Variables to Hold Slider and MPU Values
JSONVar sliderValues;
JSONVar mpuValues;


//Get Slider Values
String getSliderValues(){
  sliderValues["sliderValue1"] = String(sliderValue1);
  sliderValues["sliderValue2"] = String(sliderValue2);
  sliderValues["sliderValue3"] = String(sliderValue3);
  sliderValues["sliderValue4"] = String(sliderValue4);
  sliderValues["sliderValue5"] = String(sliderValue5);

  String jsonString = JSON.stringify(sliderValues);
  return jsonString;
}

//Get MPU Values
String getMpuValues(){
  mpuValues["AccX"] = String(Acc_angle[0]);
  mpuValues["AccY"] = String(Acc_angle[1]);
  mpuValues["AccZ"] = String(Acc_angle[2]);
  
  mpuValues["GyroX"] = String(Gyro_angle[0]);
  mpuValues["GyroY"] = String(Gyro_angle[1]);
  mpuValues["GyroZ"] = String(Gyro_angle[2]);
  
  mpuValues["MagX"] = String(Mag_angle[0]);
  mpuValues["MagY"] = String(Mag_angle[1]);
  mpuValues["MagZ"] = String(Mag_angle[2]);

  String jsonString2 = JSON.stringify(mpuValues);
  return jsonString2;
}

// Initialize SPIFFS
void initFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
   Serial.println("SPIFFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  delay(5000);
  Serial.println(WiFi.localIP());
}

void notifyClients(String mpuValues) {
  ws.textAll(mpuValues);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;

    //Controler Commands
    if (message.indexOf("ko") >= 0) { //turn off copter
      isOff = true;
      //sliderValue5 = message.substring(2);
      dutyCycle1 = map(0, 0, 100, 0, 255);
      dutyCycle2 = map(0, 0, 100, 0, 255);
      dutyCycle3 = map(0, 0, 100, 0, 255);
      dutyCycle4 = map(0, 0, 100, 0, 255);
      Serial.println("Turn off");
      //notifyClients(getSliderValues());
    }
    
    if (message.indexOf("ho") >= 0) { //hover/idle
      //sliderValue5 = message.substring(2);
      isOff = false;
      setPointR = SET_POINT_ROLL_IDLE;
      setPointP = SET_POINT_PITCH_IDLE;
      
      
      dutyCycle1 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle2 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle3 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle4 = map(secondaryThrust, 0, 100, 0, 255);
      Serial.println("Hover");
      //notifyClients(getSliderValues());
    }

    //left controler
    if (message.indexOf("ll") >= 0) {
      isOff = false;
      //sliderValue5 = message.substring(2);
      setPointR = SET_POINT_ROLL_IDLE;
      setPointP = SET_POINT_PITCH;
      setPointY = SET_POINT_PITCH_IDLE;
      
      dutyCycle1 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle2 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle3 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle4 = map(secondaryThrust, 0, 100, 0, 255);
      Serial.println("Straight left");
      //notifyClients(getSliderValues());
    }
    if (message.indexOf("lu") >= 0) {
      isOff = false;
      //sliderValue5 = message.substring(2);
      setPointR = SET_POINT_ROLL;
      setPointP = SET_POINT_PITCH_IDLE;
      setPointY = SET_POINT_YAW_IDLE;
      
      dutyCycle1 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle2 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle3 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle4 = map(primaryThrust, 0, 100, 0, 255);
      Serial.println("Straight forward");
      //notifyClients(getSliderValues());
    }
    if (message.indexOf("lr") >= 0) {
      isOff = false;
      setPointR = SET_POINT_ROLL_IDLE;
      setPointP = SET_POINT_PITCH;
      setPointY = SET_POINT_YAW_IDLE;
      //sliderValue5 = message.substring(2);
      dutyCycle1 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle2 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle3 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle4 = map(primaryThrust, 0, 100, 0, 255);
      Serial.println("Straight right");
      //notifyClients(getSliderValues());
    }
    if (message.indexOf("ld") >= 0) {
      isOff = false;
      setPointR = SET_POINT_ROLL;
      setPointP = SET_POINT_PITCH_IDLE;
      setPointY = SET_POINT_YAW_IDLE;
      //sliderValue5 = message.substring(2);
      dutyCycle1 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle2 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle3 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle4 = map(secondaryThrust, 0, 100, 0, 255);
      Serial.println("Straight reverse");
      //notifyClients(getSliderValues());
    }
    
    //right controler
    if (message.indexOf("rl") >= 0) {
      isOff = false;
      setPointR = SET_POINT_ROLL_IDLE;
      setPointP = SET_POINT_PITCH_IDLE;
      setPointY = SET_POINT_YAW;
      //sliderValue5 = message.substring(2);
      dutyCycle1 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle2 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle3 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle4 = map(secondaryThrust, 0, 100, 0, 255);
      Serial.println("Right rotation");
      //notifyClients(getSliderValues());
    }
    if (message.indexOf("ru") >= 0) {
      isOff = false;
      setPointR = SET_POINT_ROLL_IDLE;
      setPointP = SET_POINT_PITCH_IDLE;
      setPointY = SET_POINT_YAW_IDLE;
      //sliderValue5 = message.substring(2);
      dutyCycle1 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle2 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle3 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle4 = map(primaryThrust, 0, 100, 0, 255);
      Serial.println("Altitude gain");
      //notifyClients(getSliderValues());
    }
    if (message.indexOf("rr") >= 0) {
      isOff = false;
      setPointR = SET_POINT_ROLL_IDLE;
      setPointP = SET_POINT_PITCH_IDLE;
      setPointY = SET_POINT_YAW;
      //sliderValue5 = message.substring(2);
      dutyCycle1 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle2 = map(primaryThrust, 0, 100, 0, 255);
      dutyCycle3 = map(secondaryThrust, 0, 100, 0, 255);
      dutyCycle4 = map(secondaryThrust, 0, 100, 0, 255);
      Serial.println("Left rotation");
      //notifyClients(getSliderValues());
    }
    if (message.indexOf("rd") >= 0) {
      isOff = false;
      setPointR = SET_POINT_ROLL_IDLE;
      setPointP = SET_POINT_PITCH_IDLE;
      setPointY = SET_POINT_YAW_IDLE;
      //sliderValue5 = message.substring(2);
      dutyCycle1 = map(tertiaryThrust, 0, 100, 0, 255);
      dutyCycle2 = map(tertiaryThrust, 0, 100, 0, 255);
      dutyCycle3 = map(tertiaryThrust, 0, 100, 0, 255);
      dutyCycle4 = map(tertiaryThrust, 0, 100, 0, 255);
      Serial.println("Altitude loss");
      //notifyClients(getSliderValues());
    }



    //Testing Commands

    if (message.indexOf("5s") >= 0) {
      isOff = false;
      sliderValue5 = message.substring(2);
      dutyCycle1 = map(sliderValue5.toInt(), 0, 100, 0, 255);
      dutyCycle2 = map(sliderValue5.toInt(), 0, 100, 0, 255);
      dutyCycle3 = map(sliderValue5.toInt(), 0, 100, 0, 255);
      dutyCycle4 = map(sliderValue5.toInt(), 0, 100, 0, 255);
      //Serial.println(dutyCycle4);
      //Serial.print(getSliderValues());
      notifyClients(getSliderValues());
    } else{
          isOff = false;
          if (message.indexOf("1s") >= 0) {
            sliderValue1 = message.substring(2);
            dutyCycle1 = map(sliderValue1.toInt(), 0, 100, 0, 255);
            //Serial.println(dutyCycle1);
            //Serial.print(getSliderValues());
            notifyClients(getSliderValues());
          }
          if (message.indexOf("2s") >= 0) {
            sliderValue2 = message.substring(2);
            dutyCycle2 = map(sliderValue2.toInt(), 0, 100, 0, 255);
            //Serial.println(dutyCycle2);
            //Serial.print(getSliderValues());
            notifyClients(getSliderValues());
          }    
          if (message.indexOf("3s") >= 0) {
            sliderValue3 = message.substring(2);
            dutyCycle3 = map(sliderValue3.toInt(), 0, 100, 0, 255);
            //Serial.println(dutyCycle3);
            //Serial.print(getSliderValues());
            notifyClients(getSliderValues());
          }
          if (message.indexOf("4s") >= 0) {
            sliderValue4 = message.substring(2);
            dutyCycle4 = map(sliderValue4.toInt(), 0, 100, 0, 255);
            //Serial.println(dutyCycle4);
            //Serial.print(getSliderValues());
            notifyClients(getSliderValues());
          }
    }   

    
    if (strcmp((char*)data, "getValues") == 0) {
      notifyClients(getSliderValues());
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


void setup() {
  Serial.begin(115200);
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(motor3, OUTPUT);
  pinMode(motor4, OUTPUT);
  initFS();
  
  initWiFi();

  // configure LED PWM functionalitites
  ledcSetup(motorChannel1, freq, resolution);
  ledcSetup(motorChannel2, freq, resolution);
  ledcSetup(motorChannel3, freq, resolution);
  ledcSetup(motorChannel4, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(motor1, motorChannel1);
  ledcAttachPin(motor2, motorChannel2);
  ledcAttachPin(motor3, motorChannel3);
  ledcAttachPin(motor4, motorChannel4);


  initWebSocket();
  
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/app.js", "text/javascript");
  });
  
  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();

  Wire.begin();
    delay(2000);

    if (!mpu.setup(0x69)) {  // change to your own address
        while (1) {
            Serial.println("MPU connection failed. Please check your connection with `connection_check` example.");
            delay(5000);
        }
    }

    //MPU calibration
    mpu.setAccBias(28.61, 41.64, -30.58);
    mpu.setGyroBias(0.43, 0.16, -0.04);
    mpu.setMagBias(84.66, 308.93, -130.66);
  
        //if currentRoll is more than x degrees below or above setpoint, OUTPUT will be set to min or max respectively
    RollPID.setBangBang(15);
    //set PID update interval to 1000ms
    RollPID.setTimeStep(UPDATE_DELAY);
    setPointR = 180; //This is the angle in which we whant the balance to stay steady
    
    //if currentPitch is more than x degrees below or above setpoint, OUTPUT will be set to min or max respectively
    PitchPID.setBangBang(30);
    //set PID update interval to 1000ms
    PitchPID.setTimeStep(UPDATE_DELAY);
    setPointP = 180;

    //if currentPitch is more than x degrees below or above setpoint, OUTPUT will be set to min or max respectively
    YawPID.setBangBang(30);
    //set PID update interval to 1000ms
    YawPID.setTimeStep(UPDATE_DELAY);
    setPointY = 0;
  
    delay(2000);
    Serial.println("Setup Done!");
}

void loop() {
  static uint32_t lastUpdate = millis();
  if(isOff == false){
    if(mpu.update()){
      if(millis() > lastUpdate + 100){
       get_raw_mpu_values();
       lastUpdate = millis();
       notifyClients(getMpuValues());
       }
      roll_pid();
      pitch_pid();
      yaw_pid();
    }
  } 
  ledcWrite(motorChannel1, dutyCycle1);
  ledcWrite(motorChannel2, dutyCycle2);
  ledcWrite(motorChannel3, dutyCycle3);
  ledcWrite(motorChannel4, dutyCycle4);

  ws.cleanupClients();
  
}

void get_raw_mpu_values(){
     // Accelerometer //
     Acc_angle[0] = mpu.getAccX();
     //Serial.print("AccX raw value: ");
     //Serial.println(Acc_angle[0]);
     Acc_angle[1] = mpu.getAccY();
     //Serial.print("AccY raw value: ");
     //Serial.println(Acc_angle[1]);
     Acc_angle[2] = mpu.getAccZ();
     //Serial.print("AccZ raw value: ");
     //Serial.println(Acc_angle[2]);

     // Gyroscope //
     Gyro_angle[0] = mpu.getGyroX();
     //Serial.print("Gx raw value: ");
     //Serial.println(Gyro_angle[0]);
     Gyro_angle[1] = mpu.getGyroY();
     //Serial.print("Gy raw value: ");
     //Serial.println(Gyro_angle[1]);
     Gyro_angle[2] = mpu.getGyroZ();
     //Serial.print("Gz raw value: ");
     //Serial.println(Gyro_angle[2]);
     
     // Magnetometre //
     Mag_angle[0] = mpu.getMagX();
     //Serial.print("MagX raw value: ");
     //Serial.println(Mag_angle[0]);
     Mag_angle[1] = mpu.getMagY();
     //Serial.print("MagY raw value: ");
     //Serial.println(Mag_angle[1]);
     Mag_angle[2] = mpu.getMagZ();
     //Serial.print("MagZ raw value: ");
     //Serial.println(Mag_angle[2]);
          
     RPY[0] = mpu.getRoll();
     RPY[1] = mpu.getPitch();
     RPY[2] = mpu.getYaw();

}

void roll_pid(){
  if(RPY[0]<0){
    currentRoll = RPY[0] + 180;
    RollPID.run(); 
    delta = outputRoll;
  }
  else{
    currentRoll = -RPY[0] + 180;
    RollPID.run(); 
    delta = -outputRoll;
  }
  /*Serial.print("Current Roll: ");
  Serial.println(currentRoll);
  Serial.print("OutputRoll: ");
  Serial.println(outputRoll);*/

  speed1 = dutyCycle1 + delta;
  speed2 = dutyCycle2 - delta;
  speed3 = dutyCycle3 - delta;
  speed4 = dutyCycle4 + delta;

  ledcWrite(0, speed1);
  ledcWrite(1, speed2);
  ledcWrite(2, speed3);
  ledcWrite(3, speed4);
}

void pitch_pid(){
  if(RPY[1]>0){
    currentPitch = - RPY[1] + 180;
    PitchPID.run();
    delta = outputPitch;
  }
  else{
    currentPitch = RPY[1] + 180;
    PitchPID.run();
    delta = - outputPitch;
  }
  
  /*Serial.print("Current Pitch: ");
  Serial.println(currentPitch);
  Serial.print("OutputPitch: ");
  Serial.println(outputPitch);*/

  speed1 = dutyCycle1 - delta;
  speed2 = dutyCycle2 - delta;
  speed3 = dutyCycle3 + delta;
  speed4 = dutyCycle4 + delta;
  
  ledcWrite(0, speed1);
  ledcWrite(1, speed2);
  ledcWrite(2, speed3);
  ledcWrite(3, speed4);
}

void yaw_pid(){
  if(RPY[2]>0){
    currentPitch = - RPY[2] + 180;
    YawPID.run();
    delta = outputYaw;
  }
  else{
    currentYaw = RPY[2] + 180;
    YawPID.run();
    delta = - outputYaw;
  }
  
 /*Serial.print("Current Yaw: ");
  Serial.println(currentYaw);
  Serial.print("OutputYaw: ");
  Serial.println(outputYaw);*/

  speed1 = dutyCycle1 - delta;
  speed2 = dutyCycle2 + delta;
  speed3 = dutyCycle3 + delta;
  speed4 = dutyCycle4 - delta;

  ledcWrite(0, speed1);
  ledcWrite(1, speed2);
  ledcWrite(2, speed3);
  ledcWrite(3, speed4);
}
