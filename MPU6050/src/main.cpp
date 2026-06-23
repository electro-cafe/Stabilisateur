// renomer le nom du fichier en main.cpp (et pas .txt) pour le réactiver
//  Basic demo for accelerometer readings from Adafruit MPU6050

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h> //ajout pour la partie espnow
#include <Wire.h> // cLibrary for I2C communication
#include <esp_now.h>
#include <esp_wifi.h> //ajout pour la partie espnow

// accelerometer
Adafruit_MPU6050 mpu;

// Mac adress du reccepteur (0x indique que le nbr est en hexadécimal)
uint8_t broadcastAddress[] = {0xf4, 0x12, 0xfa, 0xCB, 0x20, 0xDC};

// Define variables to store MPU6050 readings that will be sent
float accelX, accelY, accelZ;
float gyroX, gyroY, gyroZ;

// Define variables to store incoming readings
float incomingAccelX, incomingAccelY, incomingAccelZ;
float incomingGyroX, incomingGyroY, incomingGyroZ;

const int kUpdateDelay = 100; // Delay between updates in milliseconds

// Structure example to send data
// typedef est un mot clé qui permet de créer un alias pour un type de données.
// C'est à dire donner un nom qui représente un type de donnés. Ainsi on écrira
// struct_message myStruct à la place de struct struct_message myStruct
typedef struct struct_message {
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
} struct_message;

// Create a struct_message called MPU6050Readings to hold sensor readings
struct_message MPU6050Readings;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

esp_now_peer_info_t peerInfo;

// Variable to store if sending data was successful
bool transmissionStatus = false;

void printTransmissionStatus(bool success) {
  if (success) {
    Serial.println("Delivery Success :)");
  } else {
    Serial.println("Delivery Fail :(");
  }
}

// Callback when data is sent. l'ESP32 fourni l'adresse MAC du module avec
// lequel il a communiqué et le status de la transmission (mac_dress, status)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  transmissionStatus = (status == ESP_NOW_SEND_SUCCESS);
  printTransmissionStatus(transmissionStatus);
}

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingAccelX = incomingReadings.accelX;
  incomingAccelY = incomingReadings.accelY;
  incomingAccelZ = incomingReadings.accelZ;
  incomingGyroX = incomingReadings.gyroX;
  incomingGyroY = incomingReadings.gyroY;
  incomingGyroZ = incomingReadings.gyroZ;
}

// --------------------------------------SETUP---------------------------------------

// Uncomment to see the MAC address of the ESP32
// void readMacAddress() {
//  uint8_t baseMac[6];
//  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
//  if (ret == ESP_OK) {
//    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n", baseMac[0], baseMac[1],
//                  baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
//  } else {
//    Serial.println("Failed to read MAC address");
//  }
//}

void setup(void) {
  Serial.begin(115200);

  while (!Serial)
    delay(10);
  Serial.println("Adafruit MPU6050 test!");

  // active l'antenne WiFi de l'ESP32 pour pouvoir utiliser ESP-NOW
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  // Serial.print("[DEFAULT] ESP32 Board MAC Address: ");
  // readMacAddress();

  // Présente et affiche les modes de sensibilité aux accélérations. Sensibilité
  // basse = plus précis sur les petits mouvements mais moins sur les grands.
  // ici il est set sur 8G.
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  // Présente et affiche les modes de sensibilité aux rotations. Sensibilité
  // basse = plus précis sur les rotations "lentes" mais moins sur les rapides.
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }
  // Présente et affiche les modes de sensibilité au bruit. Sensibilité haute =
  // lecture de toute les valeurs, même les vibrations des moteurs. Sensibilité
  // basse = lecture des mouvements "lissés" mais moins précis sur les petits
  // mouvements.
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  // saute une ligne pour plus de lisibilité dans le moniteur série
  Serial.println("");
  delay(100);
}

void loop() {

  /* Get new sensor events with the readings */
  // ça lit les valeurs des capteurs et les stocke dans les variables a, g et
  // temp (pour acceleration, gyro et temperature)
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Set values to send
  MPU6050Readings.accelX = a.acceleration.x;
  MPU6050Readings.accelY = a.acceleration.y;
  MPU6050Readings.accelZ = a.acceleration.z;
  MPU6050Readings.gyroX = g.gyro.x;
  MPU6050Readings.gyroY = g.gyro.y;
  MPU6050Readings.gyroZ = g.gyro.z;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&MPU6050Readings,
                                  sizeof(MPU6050Readings));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }
  delay(kUpdateDelay);

  /* Print out the values for Debug */
  // Serial.print("Acceleration X: ");
  // Serial.print(a.acceleration.x);
  // Serial.print(", Y: ");
  // Serial.print(a.acceleration.y);
  // Serial.print(", Z: ");
  // Serial.print(a.acceleration.z);
  // Serial.println(" m/s^2");
  //
  // Serial.print("Rotation X: ");
  // Serial.print(g.gyro.x);
  // Serial.print(", Y: ");
  // Serial.print(g.gyro.y);
  // Serial.print(", Z: ");
  // Serial.print(g.gyro.z);
  // Serial.println(" rad/s");
  //
  // Serial.print("Temperature: ");
  // Serial.print(temp.temperature);
  // Serial.println(" degC");
  //
  // Serial.println("");

  delay(500);
}
