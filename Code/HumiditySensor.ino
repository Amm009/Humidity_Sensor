#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- Network Configuration ---
const char* ssid = "Network Name";
const char* password = "Network Password";
const char* mqtt_server = "Home Server IP";  // IP of your TrueNAS
const int mqtt_port = ;

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_SHT31 sht31 = Adafruit_SHT31();

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define THRESHOLD 50               // Greater the value, more the sensitivity

int TIME_TO_SLEEP = 3600;  // Time ESP32 will go to sleep (in seconds)
touch_pad_t touchPin;
bool timerEnable = false;
int progTimerStart;
const int timer = 10;
const int R1 = 4680;
const int R2 = 4670;
const int voltagePin = 35;
const int voltEnablePin = 27;
const int buzzerPin = 15;
const int powerEnablePin = 4;
float voltBatt;
float adcVolt;
float h;

// Values for the graph array
const int BUFFER_SIZE = 100;
RTC_DATA_ATTR float buffer[BUFFER_SIZE];
RTC_DATA_ATTR int count = 0;

esp_sleep_wakeup_cause_t wakeup_reason;

void setup() {

  //Start the serial monitor
  Serial.begin(115200);

  //Ensure WiFi is working
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Configure MQTT server address
  client.setServer(mqtt_server, mqtt_port);
  delay(1000);

  // Reconnect if you disconnect
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //Set up pins for the voltage sensor and buzzer
  pinMode(buzzerPin, OUTPUT);
  pinMode(voltagePin, INPUT);
  pinMode(voltEnablePin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(powerEnablePin, OUTPUT);

  digitalWrite(voltEnablePin, HIGH);
  digitalWrite(powerEnablePin, LOW);
  digitalWrite(buzzerPin, LOW);
  delay(100);

  //Set analog read attenuation so that the pin can read the full voltage of the connected battery
  analogSetAttenuation(ADC_11db);

  Wire.begin();

  //Stop the program if something goes wrong with the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);  // Sleep for 60s
    esp_deep_sleep_start();
  }

  //Stop the program if you can't connect to the humidity sensor
  if (!sht31.begin(0x44)) {  // Address 0x44 for SHT31-D
    Serial.println("Couldn't find SHT31 sensor!");
    esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);  // Sleep for 60s
    esp_deep_sleep_start();
  }

  touchSleepWakeUpEnable(T9, THRESHOLD);
  print_wakeup_reason();

  //Wake up due to timer, just give warning tones if necessary, and upload humidity and voltage data to the server!
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    dont_announce_info();
    add_value();

    //Wake up due to touchpad, show information and warnings!
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TOUCHPAD) {
    //Show battery voltage and humidity for 10 seconds
    Serial.println("Announcing!");
    announce_info();

    //Show Humidity history graph for 5 seconds
    make_graph();
    going_to_sleep();
  }

  //Turn off external devices
  digitalWrite(voltEnablePin, LOW);
  digitalWrite(powerEnablePin, HIGH);

  //Set the esp to wake up after a defined amount of seconds
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("ESP32 will now sleep for " + String(TIME_TO_SLEEP) + " Seconds");

  //Go slep (Honk Shoo)
  Serial.flush();
  esp_deep_sleep_start();
}





// Method to print the reason by which ESP32 has been awaken from sleep
esp_sleep_wakeup_cause_t print_wakeup_reason() {
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
  }
  return wakeup_reason;
}

void wakeTone() {
  int frequency = 50;  // Frequency of Middle C in Hz
  int duration = 100;  // Duration in milliseconds

  tone(buzzerPin, frequency, duration);
  delay(duration);  // Wait for the note to finish

  noTone(buzzerPin);  // Stop the tone
}

void sleepTone() {
  int frequency = 50;  // Frequency of Middle C in Hz
  int duration = 100;  // Duration in milliseconds

  tone(buzzerPin, frequency, duration);
  delay(duration);  // Wait for the note to finish

  noTone(buzzerPin);  // Stop the tone
  delay(duration);

  tone(buzzerPin, frequency, duration);
  delay(duration);  // Wait for the note to finish

  noTone(buzzerPin);  // Stop the tone
  delay(duration);

  tone(buzzerPin, frequency, duration);
  delay(duration);  // Wait for the note to finish

  noTone(buzzerPin);  // Stop the tone
  delay(duration);
}

void warning_tone() {
  int frequencyh = 50;  // Frequency of Middle C in Hz
  int frequencyl = 20;  // Frequency of Middle C in Hz
  int duration = 100;   // Duration in milliseconds

  tone(buzzerPin, frequencyh, duration);
  delay(duration);  // Wait for the note to finish

  noTone(buzzerPin);  // Stop the tone
  delay(duration);

  tone(buzzerPin, frequencyl, duration);
  delay(duration);  // Wait for the note to finish

  noTone(buzzerPin);  // Stop the tone
  delay(duration);

  tone(buzzerPin, frequencyh, duration);
  delay(duration);  // Wait for the note to finish

  noTone(buzzerPin);  // Stop the tone
  delay(duration);

  delay(3000);
}
void add_value() {
  if (count < BUFFER_SIZE) {
    // Not full yet → just append
    buffer[count++] = h;
  } else {
    // Full → shift left by 1
    for (int i = 1; i < BUFFER_SIZE; i++) {
      buffer[i - 1] = buffer[i];
    }
    // Add newest value to the end
    buffer[BUFFER_SIZE - 1] = h;
  }
}

void make_graph() {
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(2, 0);
  display.println("Hum. Hist.");
  display.setTextSize(1);
  display.setCursor(0, 16);
  display.println("100");
  display.setCursor(5, 35);
  display.println("50");
  display.setCursor(10, 57);
  display.println("0");
  display.drawLine(20, 63, 120, 63, SSD1306_WHITE);
  display.drawLine(20, 63, 20, 16, SSD1306_WHITE);
  display.drawLine(20, 16, 24, 16, SSD1306_WHITE);
  display.drawLine(20, 39, 24, 39, SSD1306_WHITE);
  display.drawLine(20, 51, 22, 51, SSD1306_WHITE);
  display.drawLine(20, 28, 22, 28, SSD1306_WHITE);
  for (int i = 0; i < BUFFER_SIZE; i++) {
    Serial.print(buffer[i]);
    Serial.print(", ");
  }
  Serial.println();  // end the line

  for (int i = 0; i < 100; i++) {
    int x = i + 20;
    int y = map(buffer[i], 0, 100, 63, 13);

    display.drawPixel(x, y, SSD1306_WHITE);
  }
  display.display();
  delay(5000);
}

void get_info() {
  delay(100);
  h = sht31.readHumidity();
  adcVolt = analogReadMilliVolts(voltagePin) / 1000.0;
  voltBatt = adcVolt * (R1 + R2) / R2;
}


void announce_info() {
  get_info();
  //Give a warning if the battery is low
  if (voltBatt <= 4) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println("Battery Low");
    display.display();
    warning_tone();
    delay(1000);
  }
  //Give a warning if the humidity is high
  if (h > 25) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(40, 16);
    display.println("High");
    display.setCursor(18, 38);
    display.println("Humidity");
    display.display();
    warning_tone();
    delay(1000);
  }
  //Display battery and humidity info for 10 seconds
  wakeTone();
  startCountdown();
  while (timerEnable) {
    get_info();
    display.setTextColor(SSD1306_WHITE);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 0);
    display.println("Bt: " + String(voltBatt) + "V");
    display.setTextSize(3);
    display.setCursor(11, 30);
    display.print(String(h) + "%");
    display.display();
    delay(100);
    if (timer - (millis() - progTimerStart) / 1000 <= 0) {
      endCountdown();
    }
  }
}

void going_to_sleep() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(35, 16);
  display.println("Going");
  display.setCursor(18, 40);
  display.println("to Sleep");
  display.display();
  sleepTone();
  delay(2000);
  display.clearDisplay();
  display.display();
}

//Only give warning tones if necessary, no one's there to look!
void dont_announce_info() {
  get_info();
  if (voltBatt <= 4 || h > 25) {
    warning_tone();
    TIME_TO_SLEEP = 60;
  } else {
    TIME_TO_SLEEP = 60;
  }
  //Send results to MQTT
  char hum[50];
  char volt[50];
  snprintf(hum, 50, "%.0f", h);
  snprintf(volt, 50, "%.2f", voltBatt);
  client.publish("Humidity", hum);
  client.publish("Battery Voltage", volt);
  client.loop();
  delay(100);
}

void startCountdown() {
  if (timerEnable == false) {
    progTimerStart = millis();
    timerEnable = true;
  }
}

void endCountdown() {
  if (timerEnable) {
    timerEnable = false;
  }
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect (Client ID can be anything unique)
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//We not using this...
void loop() {
  // put your main code here, to run repeatedly:
}