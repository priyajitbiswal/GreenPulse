/**
 * Plant Health Monitoring System - Team 4
 * ESP32-based sensor node for monitoring plant health
 * 
 * Sensors:
 * - DHT11 (Temperature & Humidity) on pin 15
 * - Soil Moisture Sensor (Analog) on pin 34
 * 
 * MQTT Topics:
 * - /smart_garden/team4/temp
 * - /smart_garden/team4/hum
 * - /smart_garden/team4/mois
 */

#include <WiFi.h>  
#include <PubSubClient.h>
#include <DHTesp.h>

// Pin definitions
const int DHT_PIN = 15;
const int SOIL_PIN = 34;
const int LED_PIN = 2;

// WiFi credentials
const char* ssid = "SNUC";
const char* password = "snu12345";

// MQTT configuration
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* team_id = "team4";

// MQTT topic prefixes
const char* topic_temp = "/smart_garden/team4/temp";
const char* topic_hum = "/smart_garden/team4/hum";
const char* topic_mois = "/smart_garden/team4/mois";
const char* topic_status = "/smart_garden/team4/status";

// Sensor objects
DHTesp dht;
WiFiClient espClient;
PubSubClient client(espClient);

// Timing variables
unsigned long lastMsg = 0;
const unsigned long publish_interval = 2000; // Publish every 2 seconds

/**
 * Setup WiFi connection
 */
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/**
 * MQTT message callback handler
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("] ");
  
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
}

/**
 * Reconnect to MQTT broker if connection is lost
 */
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Create unique client ID
    String clientId = "ESP32-";
    clientId += team_id;
    clientId += "-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT connected!");
      
      // Publish connection status
      client.publish(topic_status, "online");
      
      // Subscribe to control topics if needed
      String subscribe_topic = "/smart_garden/";
      subscribe_topic += team_id;
      subscribe_topic += "/control";
      client.subscribe(subscribe_topic.c_str());
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

/**
 * Read and publish sensor data
 */
void publish_sensor_data() {
  // Read DHT11 sensor
  TempAndHumidity data = dht.getTempAndHumidity();
  
  if (data.temperature != -999 && data.humidity != -999) {
    // Publish temperature
    String temp_str = String(data.temperature, 2);
    client.publish(topic_temp, temp_str.c_str());
    Serial.print("Temperature: ");
    Serial.print(temp_str);
    Serial.println(" °C");
    
    // Publish humidity
    String hum_str = String(data.humidity, 1);
    client.publish(topic_hum, hum_str.c_str());
    Serial.print("Humidity: ");
    Serial.print(hum_str);
    Serial.println(" %");
  } else {
    Serial.println("DHT11 read error!");
  }
  
  // Read soil moisture sensor
  int soil_raw = analogRead(SOIL_PIN);
  // Convert to percentage (inverted: higher raw = drier soil)
  float soil_percent = (4095 - soil_raw) * 100.0 / 4095.0;
  String soil_str = String(soil_percent, 1);
  
  client.publish(topic_mois, soil_str.c_str());
  Serial.print("Soil Moisture: ");
  Serial.print(soil_str);
  Serial.println(" %");
  
  Serial.println("---");
}

/**
 * Arduino setup function
 */
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Plant Health Monitoring System - Team 4");
  Serial.println("Initializing...");
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize sensors
  dht.setup(DHT_PIN, DHTesp::DHT11);
  pinMode(SOIL_PIN, INPUT);
  
  // Setup WiFi
  setup_wifi();
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
  
  Serial.println("Setup complete!");
  Serial.println("Starting sensor readings...");
}

/**
 * Arduino main loop
 */
void loop() {
  // Maintain MQTT connection
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  
  // Publish sensor data at regular intervals
  unsigned long now = millis();
  if (now - lastMsg > publish_interval) {
    lastMsg = now;
    digitalWrite(LED_PIN, HIGH); // Indicate data transmission
    publish_sensor_data();
    digitalWrite(LED_PIN, LOW);
  }
}
