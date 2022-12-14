#include <WiFi.h>

#include<DHT.h>

#include <Firebase_ESP_Client.h>
 // Provide the token generation process info.
#include <addons/TokenHelper.h>

#include<time.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "Leonardo 2.4"
#define WIFI_PASSWORD ""

// Informe a chave da API
#define API_KEY ""

// Informe o ID do projeto no firebase
#define FIREBASE_PROJECT_ID ""

// Informe o usuário administrador previamente criado
#define USER_EMAIL "admin@gmail.com"
#define USER_PASSWORD "admin123"

#define DHTPIN 5
#define DHTTYPE DHT22
#define VIBRATIONPIN 35
#define TEMPERATUREPIN 34

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;

const char * ntpServer = "pool.ntp.org";

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime( & timeinfo)) {
    Serial.println("Failed to obtain time");
    return (0);
  }
  time( & now);
  return now;
}

// The Firestore payload upload callback function
void fcsUploadCallback(CFS_UploadStatusInfo info) {
  if (info.status == fb_esp_cfs_upload_status_init) {
    Serial.printf("\nUploading data (%d)...\n", info.size);
  } else if (info.status == fb_esp_cfs_upload_status_upload) {
    Serial.printf("Uploaded %d%s\n", (int) info.progress, "%");
  } else if (info.status == fb_esp_cfs_upload_status_complete) {
    Serial.println("Upload completed ");
  } else if (info.status == fb_esp_cfs_upload_status_process_response) {
    Serial.print("Processing the response... ");
  } else if (info.status == fb_esp_cfs_upload_status_error) {
    Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
  }
}

DHT dht(DHTPIN, DHTTYPE);

void setup() {

  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin( &config, &auth);

  Firebase.reconnectWiFi(true);

  dht.begin();

  pinMode(VIBRATIONPIN, INPUT);
  pinMode(TEMPERATUREPIN, INPUT);
  configTime(0, 3600, ntpServer);
}

void loop() {
  if (Firebase.ready() && (millis() - dataMillis > 5000 || dataMillis == 0)) {
    dataMillis = millis();
    delay(1000);

    // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create.ino
    FirebaseJson content;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));

      return;
    }

    float hic = dht.computeHeatIndex(t, h, false);

    double vibration = analogRead(VIBRATIONPIN);
    int SensorValue = analogRead(TEMPERATUREPIN);

    float voltage = SensorValue * (3.3 / 4096);

    String documentPath = "equipments/HGx3LlGQ0KCadTKOU6VH/history/";

    // double
    content.set("fields/humidity/doubleValue", h);
    content.set("fields/room_temperature/doubleValue", hic);
    content.set("fields/vibration/doubleValue", vibration);
    content.set("fields/voltageTemperature/doubleValue", voltage);
    content.set("fields/timestamp/integerValue", getTime());

    String doc_path = "projects/";
    doc_path += FIREBASE_PROJECT_ID;
    doc_path += "/databases/(default)/documents/coll_id/doc_id"; // coll_id and doc_id are your collection id and document id

    Serial.print("Create a document... ");

    if (Firebase.Firestore.createDocument( & fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw()))
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    else
      Serial.println(fbdo.errorReason());
  }
}
