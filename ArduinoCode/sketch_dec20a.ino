#include <Arduino.h>
#include <RCSwitch.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// CONFIGURATION
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

const char* FIREBASE_HOST = "your-project-id.firebaseio.com"; 
#define FIREBASE_API_KEY "YOUR_FIREBASE_API_KEY"

const String FIREBASE_NODE_URL = "/gate_control.json?auth=" FIREBASE_API_KEY;
const String FIREBASE_RESET_URL = "/gate_control.json?auth=" FIREBASE_API_KEY;

// PINS
const int CC1101_CSN_PIN = 5; 
const int CC1101_GDO0_PIN = 4; 
const int BUTTON_PIN = 27;     
const int LED_PIN = 2;

const char* PREF_NAMESPACE = "rf_storage";
const char* PREF_CODE_KEY = "rf_code";
const char* PREF_BITS_KEY = "rf_bits";
const char* PREF_PROTO_KEY = "rf_proto";

RCSwitch mySwitch = RCSwitch();
Preferences preferences;
WiFiClientSecure client;

unsigned long savedCode = 0;
int savedBitLength = 0;
int savedProtocol = 0;

bool isLearningMode = false;
unsigned long buttonPressStart = 0;
const long LEARNING_MODE_HOLD_TIME = 3000;
unsigned long lastFirebaseCheck = 0;
const long CHECK_INTERVAL = 500;

void flashLed(int count, int delayMs) {
    for (int i = 0; i < count; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}

void saveCodeToPreferences(unsigned long code, int bits, int protocol) {
    preferences.begin(PREF_NAMESPACE, false);
    preferences.putULong(PREF_CODE_KEY, code);
    preferences.putInt(PREF_BITS_KEY, bits);
    preferences.putInt(PREF_PROTO_KEY, protocol);
    preferences.end();

    savedCode = code;
    savedBitLength = bits;
    savedProtocol = protocol;
    
    mySwitch.setProtocol(savedProtocol);
}

void loadCodeFromPreferences() {
    preferences.begin(PREF_NAMESPACE, true);
    savedCode = preferences.getULong(PREF_CODE_KEY, 0);
    savedBitLength = preferences.getInt(PREF_BITS_KEY, 0);
    savedProtocol = preferences.getInt(PREF_PROTO_KEY, 1);
    preferences.end();

    if (savedCode != 0) {
        mySwitch.setProtocol(savedProtocol);
    }
}

void transmitCode() {
    if (savedCode != 0) {
        ELECHOUSE_cc1101.SetTx();
        
        mySwitch.disableReceive();
        mySwitch.enableTransmit(CC1101_GDO0_PIN);
        mySwitch.setProtocol(savedProtocol);
        
        digitalWrite(LED_PIN, HIGH);
        unsigned long startTime = millis();

        while (millis() - startTime < 4000) {
            mySwitch.send(savedCode, savedBitLength);
            delay(5);
        }
        
        digitalWrite(LED_PIN, LOW);
        
        mySwitch.disableTransmit();
        ELECHOUSE_cc1101.SetRx(); 
        pinMode(CC1101_GDO0_PIN, INPUT); 
        
        flashLed(2, 100);
    }
}

void resetFirebaseCommand() {
    if (!client.connect(FIREBASE_HOST, 443)) return;
    String resetBody = "0";
    client.print(String("PUT ") + FIREBASE_RESET_URL + " HTTP/1.1\r\n" +
                 "Host: " + FIREBASE_HOST + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + resetBody.length() + "\r\n" +
                 "Connection: close\r\n\r\n" + resetBody);
    unsigned long timeout = millis();
    while (client.connected() && (millis() - timeout < 1000)) {
        if (client.available()) client.read();
    }
    client.stop();
}

void checkFirebaseCommand() {
    if ((WiFi.status() != WL_CONNECTED) || (millis() - lastFirebaseCheck < CHECK_INTERVAL)) return;
    lastFirebaseCheck = millis();

    if (!client.connect(FIREBASE_HOST, 443)) return;

    client.print(String("GET ") + FIREBASE_NODE_URL + " HTTP/1.1\r\n" +
                 "Host: " + FIREBASE_HOST + "\r\n" +
                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.connected() && (millis() - timeout < 2000)) {
        if (client.available()) {
            String response = client.readStringUntil('\n');
            if (response.startsWith("HTTP/1.1 200 OK")) {
                if (client.find("\r\n\r\n")) {
                    String payload = client.readString();
                    payload.trim();
                    if (payload == "1") {
                        transmitCode();
                        resetFirebaseCommand();
                    }
                }
            }
            break;
        }
    }
    client.stop();
}

void setup() {
    Serial.begin(115200);
    client.setInsecure();
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    
    ELECHOUSE_cc1101.setSpiPin(18, 19, 23, CC1101_CSN_PIN);
    if (ELECHOUSE_cc1101.getCC1101()) {
        Serial.println("CC1101 OK");
    } else {
        Serial.println("CC1101 Error");
        while(1);
    }
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setMHZ(433.92);
    ELECHOUSE_cc1101.setModulation(2); 
    ELECHOUSE_cc1101.setPA(10);        
    ELECHOUSE_cc1101.SetRx();          
    
    pinMode(CC1101_GDO0_PIN, INPUT);   

    loadCodeFromPreferences();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    resetFirebaseCommand();
}

void loop() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (buttonPressStart == 0) buttonPressStart = millis();
        if (!isLearningMode && (millis() - buttonPressStart >= LEARNING_MODE_HOLD_TIME)) {
            isLearningMode = true;
            digitalWrite(LED_PIN, HIGH);
            mySwitch.enableReceive(CC1101_GDO0_PIN);
        }
    } else {
        buttonPressStart = 0;
    }

    if (isLearningMode && mySwitch.available()) {
        unsigned long value = mySwitch.getReceivedValue();
        int bitlength = mySwitch.getReceivedBitlength();
        int protocol = mySwitch.getReceivedProtocol();

        if (value != 0) {
            saveCodeToPreferences(value, bitlength, protocol);
            flashLed(3, 150);
            isLearningMode = false;
            digitalWrite(LED_PIN, LOW);
            mySwitch.disableReceive(); 
        }
        mySwitch.resetAvailable();
    }

    if (!isLearningMode) {
        checkFirebaseCommand();
    }
}