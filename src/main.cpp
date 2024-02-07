#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Network credentials and Firebase configuration
const char* ssid = "UW MPSK";
const char* password = "QjM[+9iSs5"; // Replace with your network password
#define DATABASE_URL "https://esp32-firebase-jassie-default-rtdb.firebaseio.com/" // Replace with your Firebase database URL
#define API_KEY "AIzaSyBj9JZuTLDF8LhF-8jNGMk7FH8eYOP7E18" // Replace with your Firebase API key

// HC-SR04 Pins
const int trigPin = 2;
const int echoPin = 3;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

// Set the deep sleep and wake-up conditions
const unsigned long DEEP_SLEEP_DURATION = 30e6; // 30 seconds deep sleep
const float DISTANCE_THRESHOLD = 50.0; // 50 cm threshold for detecting presence
const unsigned long DETECTION_PERIOD = 30000; // 30 seconds to confirm presence

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool shouldSleep = false;

// Function prototypes
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);
float measureDistance();
void goToDeepSleep();

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  connectToWiFi();
  initFirebase();

  // Run initial measurement
  if (measureDistance() > DISTANCE_THRESHOLD) {
    shouldSleep = true;
  } else {
    unsigned long startMillis = millis();
    while (millis() - startMillis < DETECTION_PERIOD) {
      if (measureDistance() > DISTANCE_THRESHOLD) {
        shouldSleep = true;
        break;
      }
      delay(100); // Delay between measurements
    }
  }

  if (shouldSleep) {
    Serial.println("No movement detected. Entering deep sleep.");
    goToDeepSleep();
  } else {
    Serial.println("Movement detected. Sending data to Firebase.");
    sendDataToFirebase(measureDistance());
  }
}

void loop() {
  // Empty loop because we use deep sleep
}

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int retries = 0;

  while (WiFi.status() != WL_CONNECTED && retries < 5) {
    delay(1000);
    Serial.print(".");
    retries++;
  }

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi. Going to deep sleep.");
    goToDeepSleep();
  }

  Serial.println("Connected to WiFi");
}

void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void sendDataToFirebase(float distance) {
  if (Firebase.ready()) {
    if (Firebase.RTDB.pushFloat(&fbdo, "/sensor_data/distance", distance)) {
      Serial.println("Sent distance to Firebase");
    } else {
      Serial.println("Failed to send distance to Firebase");
    }
  }
}

void goToDeepSleep() {
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION);
  esp_deep_sleep_start();
}
