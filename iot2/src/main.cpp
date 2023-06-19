#include <Wifi.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <../lib/ArduinoJson-v6.21.2.h>
#include <internalTemperature.h>

// Définition des broches pour la mémoire EPROM
#define EEPROM_SIZE 512
#define CONFIG_ADDRESS 0

// Variables de configuration
int tempFreq = 5; // Fréquence d'acquisition de la température en secondes
int connectionFreq = 30; // Fréquence d'envoi des données par WiFi en secondes
int connectionConfig = 2; // Protocole de connexion (1: HTTP, 2: MQTT)

// Variables pour le WiFi
const char* ssid = "esp-guigui";
const char* password = "esp-guiguiesp-guigui";


// Variables pour la communication HTTP
const char* apiHost = "http://172.20.10.5:8080";
const char* espName = "espGuiGui2";

// Méthode pour lire la configuration depuis la mémoire EPROM
void readConfigFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  // Lecture de la configuration depuis l'EEPROM
  EEPROM.get(CONFIG_ADDRESS, tempFreq);
  EEPROM.get(CONFIG_ADDRESS + sizeof(int), connectionConfig);
  EEPROM.get(CONFIG_ADDRESS + 2 * sizeof(int), connectionFreq);

  EEPROM.end();
}

// Méthode pour sauvegarder la configuration dans la mémoire EPROM
void saveConfigToEEPROMIfEmpty() {
  EEPROM.begin(EEPROM_SIZE);

  // Vérifier si l'EEPROM est vide en lisant le premier octet
  byte firstByte = EEPROM.read(CONFIG_ADDRESS);
  if (firstByte != 0xFF) {
    // L'EEPROM n'est pas vide, la configuration existe déjà
    EEPROM.end();
    return;
  }

  // L'EEPROM est vide, sauvegarde de la configuration
  EEPROM.put(CONFIG_ADDRESS, tempFreq);
  EEPROM.put(CONFIG_ADDRESS + sizeof(int), connectionConfig);
  EEPROM.put(CONFIG_ADDRESS + 2 * sizeof(int), connectionFreq);

  EEPROM.commit();
  EEPROM.end();
}

// Méthode pour envoyer les données au serveur REST API
void sendDataToAPI() {
  WiFiClient client;
  HTTPClient http;

  // Création de l'URL de l'API avec le nom de l'ESP32
  String apiUrl = String(apiHost) + "/api/esp32/" + String(espName);

  // Construction du corps de la requête JSON
  DynamicJsonDocument jsonBody(256);
  jsonBody["config"]["tempFreq"] = tempFreq;
  jsonBody["config"]["connectionConfig"] = connectionConfig;
  jsonBody["config"]["connectionFreq"] = connectionFreq;
  jsonBody["temperatures"].to<JsonArray>();
  jsonBody["temperatures"].add(readTemp2(false));
  
  String requestBody;
  serializeJson(jsonBody, requestBody);

  // Envoi de la requête PUT avec les données
  int httpResponseCode;
  http.begin(client, apiUrl);
  http.addHeader("Content-Type", "application/json");
  httpResponseCode = http.PUT(requestBody);

  if (httpResponseCode == HTTP_CODE_OK) {
    Serial.println("Données envoyées avec succès !");
  } else {
    Serial.print("Erreur lors de l'envoi des données. Code d'erreur : ");
    Serial.println(httpResponseCode);
  }

  http.end();
  client.stop();
}

void setup() {
  Serial.begin(9600);

  // Lecture de la configuration depuis l'EEPROM
  // readConfigFromEEPROM();
  // saveConfigToEEPROMIfEmpty();

  // Connexion au WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connexion au WiFi...");
  }
  Serial.println("Connecté au WiFi !");

  // Envoi des données au serveur REST API
  sendDataToAPI();
    Serial.println("POST API !");

  WiFi.disconnect();
  // Mise en veille profonde pour économiser les ressources
  esp_sleep_enable_timer_wakeup(connectionFreq * 1000000); // conversion de secondes en microsecondes
  esp_deep_sleep_start();
}

void loop() {
  // Ne sera pas exécuté car nous utilisons la mise en veille profonde
}