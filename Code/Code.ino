
// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                      VOICE Assistant - Demo (Code snippets, examples)                    ------------------
// ----------------                                       July 22, 2024                                        ------------------
// ------------------                                                                                          ------------------
// ------------------              Voice RECORDING with variable length [native I2S code]                      ------------------
// ------------------                   SpeechToText [using Deepgram API service]                              ------------------
// ------------------                                                                                          ------------------
// ------------------                    HW: ESP32 with connected Micro SD Card                                ------------------
// ------------------                    SD Card: using VSPI Default pins 5,18,19,23                           ------------------
// ------------------------------------------------------------------------------------------------------------------------------/*


/*
Connections 

SD Card Module       ESP32 
GND                  GND
Vcc                  VIn
MISO                 D19 
MOSI                 D23 
SCK                  D18 
CS                   D5


I2S MIC              ESP32
GND                  GND
VDD                  3.3V
SD                   D35                  
SCK                  D33
WS                   D22
L/R                  3.3V


*/

// *** HINT: in case of an 'Sketch too Large' Compiler Warning/ERROR in Arduino IDE (ESP32 Dev Module):
// -> select a larger 'Partition Scheme' via menu > tools: e.g. using 'No OTA (2MB APP / 2MB SPIFFS) ***


#define VERSION "\n=== KALO ESP32 Voice Assistant (last update: July 22, 2024) ======================"

#include <WiFi.h>  // only included here
#include <SD.h>    // also needed in other tabs (.ino)

#include <Audio.h>  // needed for PLAYING Audio (via I2S Amplifier, e.g. MAX98357) with ..
                    // Audio.h library from Schreibfaul1: https://github.com/schreibfaul1/ESP32-audioI2S
                    // .. ensure you have actual version (July 18, 2024 or newer needed for 8bit wav files!)
#define AUDIO_FILE        "/Audio.wav"    // mandatory, filename for the AUDIO recording

// --- PRIVATE credentials -----

const char* ssid = "SSID";         // ## INSERT your wlan ssid
const char* password = "PASS";  // ## INSERT your password

 // mandatory, filename for the AUDIO recording


// --- PIN assignments ---------

#define pin_RECORD_BTN 36
#define LED 2

// --- global Objects ----------

Audio audio_play;


// declaration of functions in other modules (not mandatory but ensures compiler checks correctly)
// splitting Sketch into multiple tabs see e.g. here: https://www.youtube.com/watch?v=HtYlQXt14zU

bool I2S_Record_Init();
bool Record_Start(String filename);
bool Record_Available(String filename, float* audiolength_sec);

String SpeechToText_Deepgram(String filename);
void Deepgram_KeepAlive();



// ------------------------------------------------------------------------------------------------------------------------------
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.setTimeout(100);  // 10 times faster reaction after CR entered (default is 1000ms)

  // Pin assignments:
  pinMode(LED, OUTPUT);
  pinMode(pin_RECORD_BTN, INPUT);  // use INPUT_PULLUP if no external Pull-Up connected ##


  // Hello World
  Serial.println(VERSION);

  // Connecting to WLAN
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WLAN ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(". Done, device connected.");
  digitalWrite(LED, LOW);

  // Initialize SD card
  if (!SD.begin()) {
    Serial.println("ERROR - SD Card initialization failed!");
    return;
  }

  // initialize KALO I2S Recording Services (don't forget!)
  I2S_Record_Init();

  // INIT done, starting user interaction
  Serial.println("> HOLD button for recording AUDIO .. RELEASE button for REPLAY & Deepgram transcription");
}



// ------------------------------------------------------------------------------------------------------------------------------
void loop() {
  if (digitalRead(pin_RECORD_BTN) == LOW)  // Recording started (ongoing)
  {
    digitalWrite(LED, HIGH);
    delay(30);  // unbouncing & suppressing button 'click' noise in begin of audio recording

    //Start Recording
    Record_Start(AUDIO_FILE);
  }

  if (digitalRead(pin_RECORD_BTN) == HIGH)  // Recording not started yet .. OR stopped now (on release button)
  {
    digitalWrite(LED, LOW);

    float recorded_seconds;
    if (Record_Available(AUDIO_FILE, &recorded_seconds))  //  true once when recording finalized (.wav file available)
    {
      if (recorded_seconds > 0.4)  // ignore short btn TOUCH (e.g. <0.4 secs, used for 'audio_play.stopSong' only)
      {

        // [SpeechToText] - Transcript the Audio (waiting here until done)
        digitalWrite(LED, HIGH);

        String transcription = SpeechToText_Deepgram(AUDIO_FILE);

        digitalWrite(LED, LOW);
        Serial.println(transcription);
      }
    }
  }




  // [Optional]: Stabilize WiFiClientSecure.h + Improve Speed of STT Deepgram response (~1 sec faster)
  // Idea: Connect once, then sending each 5 seconds dummy bytes (to overcome Deepgram auto-closing 10 secs after last request)
  // keep in mind: WiFiClientSecure.h still not 100% reliable (assuming RAM heap issue, rarely freezes after e.g. 10 mins)

  if (digitalRead(pin_RECORD_BTN) == HIGH && !audio_play.isRunning())  // but don't do it during recording or playing
  {
    static uint32_t millis_ping_before;
    if (millis() > (millis_ping_before + 5000)) {
      millis_ping_before = millis();
      digitalWrite(LED, HIGH);  // short LED OFF means: 'Reconnection server, can't record in moment'
      Deepgram_KeepAlive();
    }
  }
}
