#include <TinyGPS++.h> // ไลบรารีสำหรับแปลภาษา GPS
#include <SD.h>        // ไลบรารีสำหรับ SD Card
#include <SPI.h>       // ไลบรารีการสื่อสาร

// ==========================================
// 🔌 การตั้งค่าขา (Wiring - ESP32 Devkit V1)
// ==========================================

// 1. ตั้งค่า GPS (NEO-6M)
static const int RXPin = 16; // สาย TX ของ GPS --> เสียบขา 16 (RX2) ของ ESP32
static const int TXPin = 17; // สาย RX ของ GPS --> เสียบขา 17 (TX2) ของ ESP32
static const uint32_t GPSBaud = 9600; // ความเร็วมาตรฐานของ NEO-6M

// 2. ตั้งค่า SD Card
#define CS_PIN 5  // สาย CS ของ SD Card --> เสียบขา 5 (D5) ของ ESP32

// 3. ไฟสถานะ (บนบอร์ด)
#define LED_PIN 2 // ไฟสีฟ้าบนบอร์ด เอาไว้กระพริบโชว์

// ==========================================
// ตัวแปรระบบ
// ==========================================
TinyGPSPlus gps;
HardwareSerial neogps(1); // ใช้ Hardware Serial ช่อง 1
File myFile;
String filename = "/football_data.csv"; // ตั้งชื่อไฟล์ที่นี่
unsigned long lastLog = 0;

void setup() {
  // เริ่มต้น Serial Monitor (เอาไว้ดูในคอม ถ้าเสียบสาย)
  Serial.begin(115200);
  
  // เริ่มต้นรับค่า GPS
  neogps.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);
  
  // ตั้งค่าไฟ LED เป็น Output
  pinMode(LED_PIN, OUTPUT);

  Serial.println("\n--- Wolfsburg GPS System Starting ---");

  // --- 1. เริ่มต้น SD Card ---
  Serial.print("Initializing SD card... ");
  
  // ถ้าหาการ์ดไม่เจอ ให้ไฟกระพริบรัวๆ (SOS Mode)
  if (!SD.begin(CS_PIN)) {
    Serial.println("FAILED! (Check Connection)");
    while (1) { 
      digitalWrite(LED_PIN, HIGH); delay(50);
      digitalWrite(LED_PIN, LOW); delay(50);
    }
  }
  Serial.println("DONE.");

  // --- 2. สร้างไฟล์และเขียนหัวตาราง (Header) ---
  // เช็คว่ามีไฟล์นี้ยัง? ถ้าไม่มีให้สร้างใหม่
  if (!SD.exists(filename)) {
    myFile = SD.open(filename, FILE_WRITE);
    if (myFile) {
      // เขียนหัวคอลัมน์ (สำคัญมาก! โปรแกรม Python จะอ่านบรรทัดนี้)
      myFile.println("Date,Time,Lat,Lon"); 
      myFile.close();
      Serial.println("Created new file & Header written.");
    } else {
      Serial.println("Error creating file!");
    }
  } else {
    Serial.println("File exists. Appending new data...");
  }
  
  // กระพริบยาว 1 ที บอกว่าพร้อมลุย!
  digitalWrite(LED_PIN, HIGH); delay(1000); digitalWrite(LED_PIN, LOW);
}

void loop() {
  // --- 1. อ่านข้อมูลดิบจาก GPS ตลอดเวลา ---
  // (ต้องมี Loop นี้ ห้ามลบ! ไม่งั้นค่าไม่อัปเดต)
  while (neogps.available() > 0) {
    gps.encode(neogps.read());
  }

  // --- 2. ตั้งเวลาบันทึกทุก 1 วินาที (1000 ms) ---
  if (millis() - lastLog >= 1000) {
    lastLog = millis();

    // ดึงค่าวันที่และเวลา
    // (ถ้า GPS ยังไม่จับสัญญาณ วันที่อาจจะเป็น 0/0/0 ไม่ต้องตกใจ)
    String d = String(gps.date.day()) + "/" + String(gps.date.month()) + "/" + String(gps.date.year());
    String t = String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());
    
    // ดึงค่าพิกัด (Lat, Lon)
    // (ถ้า GPS ยังไม่จับสัญญาณ ค่าจะเป็น 0.000000)
    double lat = gps.location.lat();
    double lon = gps.location.lng();

    // --- 3. บันทึกลงการ์ด (Force Write) ---
    // เปิดไฟล์แบบต่อท้าย (FILE_APPEND)
    myFile = SD.open(filename, FILE_APPEND);
    
    if (myFile) {
      // เขียนข้อมูลลงไปบรรทัดใหม่
      myFile.print(d);
      myFile.print(",");
      myFile.print(t);
      myFile.print(",");
      myFile.print(lat, 6); // ทศนิยม 6 ตำแหน่ง
      myFile.print(",");
      myFile.println(lon, 6);
      
      myFile.close(); // ปิดไฟล์ทันทีเพื่อบันทึก

      // --- 4. แสดงสถานะว่า "จดแล้ว" ---
      // ไฟกระพริบแว๊บนึง
      digitalWrite(LED_PIN, HIGH);
      delay(50); 
      digitalWrite(LED_PIN, LOW);

      // ปริ้นบอกในคอม (เผื่อดู)
      Serial.print("Saved: "); Serial.print(t);
      Serial.print(" | Lat: "); Serial.print(lat, 6);
      Serial.print(" | Lon: "); Serial.println(lon, 6);
      
    } else {
      Serial.println("Error writing to file!");
      // ถ้าเขียนไม่ได้ ให้ไฟติดค้าง
      digitalWrite(LED_PIN, HIGH);
    }
  }
}