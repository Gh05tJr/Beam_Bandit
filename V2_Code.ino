#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <vector>

#include "webfiles.h"

const uint16_t Recv_Pin = 14;                //D5 (GPIO 14)
const uint16_t Send_Pin = 4;                 //D2 (GPIO 4)
const uint16_t Button_Capture_Pin = 12;      //D6 (GPIO 12)
const uint16_t Button_Send_Pin = 13;         //D7 (GPIO 13)
const uint16_t Led_Pin = 2;                  //D4 (GPIO 2)

const uint16_t Capture_Buffer_Size = 1200; 
const uint8_t  Timeout = 50;              
const uint16_t Min_Unknown_Size = 12;          //Ignores stray ambient IR noise
const uint32_t Frequency = 38;           

const char* SSID = "JioFiber_CtHw41";
const char* Password = "I am Gh05tJr";

IRrecv IR_Recv(Recv_Pin,Capture_Buffer_Size,Timeout,true);
IRsend IR_Send(Send_Pin);
decode_results Results;
std::vector<uint16_t> Captured_Signal;
bool New_Signal_Available = false;

AsyncWebServer server(80);

class Button{
  private:
    uint8_t Pin; bool State; bool Last_State; unsigned long Last_Debounce_Time; unsigned long Debounce_Delay;
  public:
    Button(uint8_t P, unsigned long D = 50): 
    Pin(P), Debounce_Delay(D), State(HIGH), Last_State(HIGH), Last_Debounce_Time(0){
        pinMode(Pin, INPUT_PULLUP);
    }
    bool Is_Pressed(){
        bool Reading = digitalRead(Pin); 
        bool Pressed = false;
        if (Reading != Last_State){
                Last_Debounce_Time = millis();
            }
        if ((millis() - Last_Debounce_Time) > Debounce_Delay){
                if (Reading != State) 
                    { 
                        State = Reading; 
                        if (State == LOW) 
                            {
                                Pressed = true; 
                            }
                    }
            }
        Last_State = Reading; return Pressed;
    }
};

class LedController{
  private:
    uint8_t Pin; unsigned long Last_Update; bool State; bool Active; unsigned long Interval;
  public:
    LedController(uint8_t P) : 
    Pin(P), Last_Update(0), State(HIGH), Active(false), Interval(0){ 
        pinMode(Pin, OUTPUT); 
        Off(); 
    }
    void On(){ 
        Active = false; 
        digitalWrite(Pin, LOW); 
    }
    void Off(){ 
        Active = false; 
        digitalWrite(Pin, HIGH); 
    }
    void startBlink(unsigned long MS){ 
        Active = true; 
        Interval = MS; 
    }
    void update() {
        if (!Active) {
            return;
        }
        if (millis() - Last_Update >= Interval){ 
            Last_Update = millis(); 
            State = !State; 
            digitalWrite(Pin, State ? HIGH : LOW); 
        }
    }
};

Button Button_Capture(Button_Capture_Pin);
Button Button_Send(Button_Send_Pin);
LedController Status_Led(Led_Pin);

enum SystemState{ 
    STATE_IDLE, STATE_CAPTURING, STATE_SENDING 
};
SystemState Current_State = STATE_IDLE;

void Setup_Web_Server(){
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String Response = "{\"state\":";
        if (Current_State == STATE_IDLE){ 
            Response += "\"IDLE\"";
        }
        else if (Current_State == STATE_CAPTURING){
            Response += "\"CAPTURING\"";
        }
        else if (Current_State == STATE_SENDING){ 
            Response += "\"SENDING\"";
        }
        Response += ",\"signal\":[";
        for (size_t i = 0; i < Captured_Signal.size(); i++) {
            Response += String(Captured_Signal[i]);
            if (i < Captured_Signal.size() - 1){
                Response += ",";
            }
        }
        Response += "],\"new_data\":";
        Response += New_Signal_Available ? "true" : "false";
        Response += "}";
        
        if(New_Signal_Available){
            New_Signal_Available = false;
        } 
        request->send(200, "application/json", Response);
    });

    server.on("/api/capture", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (Current_State == STATE_IDLE){
            Captured_Signal.clear();
            IR_Recv.enableIRIn();
            Status_Led.startBlink(100);
            Current_State = STATE_CAPTURING;
            request->send(200, "text/plain", "Capture Started");
        } 
        else if (Current_State == STATE_CAPTURING) {
            IR_Recv.disableIRIn();
            Status_Led.Off();
            Current_State = STATE_IDLE;
            request->send(200, "text/plain", "Capture Cancelled");
        }
        else{
            request->send(400, "text/plain", "Busy");
        }
    });

    // INSTANT HARDWARE MEMORY TRIGGER ENDPOINT
    server.on("/api/fire_live", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!Captured_Signal.empty() && Current_State == STATE_IDLE) {
            Current_State = STATE_SENDING;
            request->send(200, "text/plain", "Hardware Transmission Fired");
        } 
        else{
            request->send(400, "text/plain", "Buffer Empty or Device Busy");
        }
    });

    server.on("/api/send", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("data", true)) {
            String Raw_Input = request->getParam("data", true)->value();
            Raw_Input.replace("[", "");
            Raw_Input.replace("]", "");
            Raw_Input.trim();

            if (Raw_Input.length() > 0) {
                Captured_Signal.clear();
                int Start_Index = 0;
                int End_Index = Raw_Input.indexOf(',');
                
                while (End_Index >= 0) {
                    String Token = Raw_Input.substring(Start_Index,End_Index);
                    Token.trim();
                    if(Token.length() > 0) {
                        Captured_Signal.push_back((uint16_t)Token.toInt());
                    }
                    Start_Index = End_Index + 1;
                    End_Index = Raw_Input.indexOf(',', Start_Index);
                }
                String Final_Token = Raw_Input.substring(Start_Index);
                Final_Token.trim();
                if(Final_Token.length() > 0) {
                    Captured_Signal.push_back((uint16_t)Final_Token.toInt());
                }
            }
        }

        if (!Captured_Signal.empty()) {
            Current_State = STATE_SENDING;
            request->send(200, "text/plain", "Transmission Sequenced");
        } 
        else {
            request->send(400, "text/plain", "Empty Data");
        }
    });

    server.begin();
}

void setup() {
    Serial.begin(115200);
    WiFi.softAP(SSID,Password);
    IR_Recv.setUnknownThreshold(Min_Unknown_Size);
    IR_Send.begin();
    Setup_Web_Server();
}

void loop() {
    Status_Led.update();

    switch (Current_State) {
        case STATE_IDLE:
            if (Button_Capture.Is_Pressed()) {
                Captured_Signal.clear();
                IR_Recv.enableIRIn();
                Status_Led.startBlink(100);
                Current_State = STATE_CAPTURING;
            } else if (Button_Send.Is_Pressed()){
                if (!Captured_Signal.empty()) {
                    Current_State = STATE_SENDING;
                }
            }
            break;

        case STATE_CAPTURING:
            if (Button_Capture.Is_Pressed()) {
                    IR_Recv.disableIRIn();      //Used to cancel the capture mode
                    Status_Led.Off();           
                    Current_State = STATE_IDLE; 
                    break;                      
                }

            if (IR_Recv.decode(&Results)) {
                uint16_t Count = Results.rawlen;
                if (Count > 0) {
                    Captured_Signal.reserve(Count - 1);
                    for (uint16_t i = 1; i < Count; i++) {
                        uint32_t usecs = Results.rawbuf[i] * kRawTick;
                        if (usecs > 65535) {
                            usecs = 65535;
                        }
                        Captured_Signal.push_back((uint16_t)usecs);
                    }
                    New_Signal_Available = true; 
                }
                IR_Recv.disableIRIn();
                Status_Led.Off();
                Current_State = STATE_IDLE;
            }
            break;

        case STATE_SENDING:
            Status_Led.On();
            IR_Send.sendRaw(Captured_Signal.data(), Captured_Signal.size(),Frequency);
            Status_Led.Off();
            Current_State = STATE_IDLE;
            break;
    }
}