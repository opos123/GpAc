#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include <ArduinoJson.h>

// GPS settings
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

// MPU6050 settings
MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;
struct MyData {
  float Roll;
  float Pitch;
  float Yaw;
};
MyData data;
float filtered_ax = 0, filtered_ay = 0, filtered_az = 0;
const float alpha = 0.1;
unsigned long previousTime = 0;
float gz_offset = 0;
const float gyroscopeThreshold = 2; // Sesuaikan threshold giroskop
const float accelerometerThreshold = 2; // Sesuaikan threshold akselerometer
const float yawDriftThreshold = 0.5; // Threshold untuk drift yaw
float initialRoll = 0, initialPitch = 0, initialYaw = 0;
bool initialYawSet = false; // Flag untuk menyimpan nilai yaw awal

// GPS data averaging
const int numReadings = 10;
float latitudeReadings[numReadings];
float longitudeReadings[numReadings];
int readIndex = 0;
float totalLat = 0;
float totalLng = 0;
float averagedLat = 0;
float averagedLng = 0;

void setup() {
  Serial.begin(9600); // Menginisialisasi komunikasi serial dengan baud rate 9600
  ss.begin(GPSBaud);  // Menginisialisasi komunikasi serial dengan modul GPS

  Wire.begin();       // Menginisialisasi komunikasi I2C
  mpu.initialize();   // Menginisialisasi sensor MPU6050

  if (mpu.testConnection()) {
    Serial.println("MPU6050 terhubung"); // Menampilkan pesan jika sensor terhubung
  } else {
    Serial.println("MPU6050 tidak terhubung"); // Menampilkan pesan jika sensor tidak terhubung
    while (1); // Menghentikan program jika sensor tidak terhubung
  }

  Serial.println("Kalibrasi giroskop, harap jangan gerakkan sensor...");

  int numSamples = 100; // Jumlah sampel untuk kalibrasi
  long gz_sum = 0;

  for (int i = 0; i < numSamples; i++) {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // Membaca data sensor
    gz_sum += gz; // Menjumlahkan nilai gz
    delay(10); // Menunggu selama 10 milidetik
  }

  gz_offset = gz_sum / numSamples / 131.0; // Menghitung offset giroskop

  Serial.println("Kalibrasi selesai");

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // Membaca data sensor
  filtered_ax = ax; // Menginisialisasi nilai filter
  filtered_ay = ay;
  filtered_az = az;

  initialRoll = atan2(filtered_ay, filtered_az) * 180 / PI; // Menghitung roll awal
  initialPitch = atan2(-filtered_ax, sqrt(filtered_ay * filtered_ay + filtered_az * filtered_az)) * 180 / PI; // Menghitung pitch awal
  initialYaw = 0;  // Asumsi yaw awal adalah 0
  initialYawSet = true; // Mengaktifkan flag nilai yaw awal

  // Initialize GPS data averaging
  for (int i = 0; i < numReadings; i++) {
    latitudeReadings[i] = 0;
    longitudeReadings[i] = 0;
  }
}

void loop() {
  // Menangani data GPS
  if (ss.available() > 0) {
    char c = ss.read(); // Membaca data dari modul GPS
    gps.encode(c);
  }

  if (gps.location.isValid()) {
    // Membaca data GPS
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();

    // Update GPS data averaging
    totalLat -= latitudeReadings[readIndex];
    totalLng -= longitudeReadings[readIndex];
    latitudeReadings[readIndex] = latitude;
    longitudeReadings[readIndex] = longitude;
    totalLat += latitude;
    totalLng += longitude;
    readIndex = (readIndex + 1) % numReadings;
    averagedLat = totalLat / numReadings;
    averagedLng = totalLng / numReadings;

    // Membaca data MPU6050
    bacaMPU6050();

    // Menyiapkan data JSON
    StaticJsonDocument<100> doc;
    doc["latitude"] = averagedLat;
    doc["longitude"] = averagedLng;
    doc["roll"] = data.Roll;
    doc["pitch"] = data.Pitch;
    doc["yaw"] = data.Yaw;

    // Men-serialisasi JSON ke string dan mencetaknya
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println(jsonString); // Menampilkan data JSON melalui serial
  }

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("Tidak ada GPS terdeteksi: periksa kabel.");
    while (true); // Menghentikan program jika GPS tidak terdeteksi
  }
}

void bacaMPU6050() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // Membaca data dari sensor MPU6050

  // Memfilter data akselerometer
  filtered_ax = alpha * ax + (1 - alpha) * filtered_ax;
  filtered_ay = alpha * ay + (1 - alpha) * filtered_ay;
  filtered_az = alpha * az + (1 - alpha) * filtered_az;

  // Menghitung sudut Roll dan Pitch
  float currentRoll = atan2(filtered_ay, filtered_az) * 180 / PI;
  float currentPitch = atan2(-filtered_ax, sqrt(filtered_ay * filtered_ay + filtered_az * filtered_az)) * 180 / PI;

  // Memeriksa perubahan signifikan pada Roll dan Pitch
  if (abs(currentRoll - initialRoll) < accelerometerThreshold) {
    data.Roll = 0; // Mengembalikan ke nol jika di posisi awal
  } else {
    data.Roll = currentRoll;
  }

  if (abs(currentPitch - initialPitch) < accelerometerThreshold) {
    data.Pitch = 0; // Mengembalikan ke nol jika di posisi awal
  } else {
    data.Pitch = currentPitch;
  }

  // Menghitung deltaTime untuk integrasi Yaw
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - previousTime) / 1000.0;
  previousTime = currentTime;

  // Mengkalibrasi Giroskop
  float gz_calibrated = gz / 131.0 - gz_offset;

  // Menerapkan threshold untuk mengabaikan perubahan kecil
  if (abs(gz_calibrated) > gyroscopeThreshold) {
    // Mengintegrasikan kecepatan sudut untuk mendapatkan Yaw
    data.Yaw += gz_calibrated * deltaTime;

    // Menormalkan sudut Yaw ke [-180, 180]
    while (data.Yaw > 180) data.Yaw -= 360;
    while (data.Yaw < -180) data.Yaw += 360;
  }

  // Menghitung total perubahan yaw dari nilai awal
  float deltaYaw = data.Yaw - initialYaw;

  // Memeriksa jika Yaw kembali ke posisi awal
  if (abs(deltaYaw) < yawDriftThreshold) {
    data.Yaw = initialYaw; // Mengembalikan ke nilai awal jika di posisi awal
  }
}
