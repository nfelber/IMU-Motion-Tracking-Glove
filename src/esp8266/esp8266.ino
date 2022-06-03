#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"
#include "Wire.h"
#include <Adafruit_Sensor_Calibration.h>
#include <Adafruit_AHRS.h>
#include <Adafruit_FXAS21002C.h>
#include <Adafruit_FXOS8700.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

Adafruit_Sensor *accelerometer, *gyroscope, *magnetometer;
Adafruit_NXPSensorFusion filter;
#if defined(ADAFRUIT_SENSOR_CALIBRATION_USE_EEPROM)
  Adafruit_Sensor_Calibration_EEPROM cal;
#else
  Adafruit_Sensor_Calibration_SDFat cal;
#endif

#define FILTER_UPDATE_RATE_HZ 25
//#define FILTER_UPDATE_RATE_HZ 34
Adafruit_FXOS8700 fxos = Adafruit_FXOS8700(0x8700A, 0x8700B);
Adafruit_FXAS21002C fxas = Adafruit_FXAS21002C(0x0021002C);


const int offsets[30] = {
  -2250,  733,  360,  37, 21, -167,
  -3392,  347,  176,  -39,  39, -45,
  -1518,  2031, 1416, 62, -6, 52,
  -312, -3249,  1576, -354, 33, 66,
  -348, 523,  848,  67, 128,  9
};

const int I2C_BUS0_SDA = 12;
const int I2C_BUS0_SCL = 14;
const int I2C_BUS12_SDA = 2;
const int I2C_BUS12_SCL = 0;
const int I2C_BUS34_SDA = 4;
const int I2C_BUS34_SCL = 5;

const int I2C_FREQ = 400000;
const int DMP_PACKET_SIZE = 28;

MPU6050 sensor0(0x68);
MPU6050 sensor1(0x68);
MPU6050 sensor2(0x69);
MPU6050 sensor3(0x68);
MPU6050 sensor4(0x69);

bool dmpReady = false;  // set true if DMP init was successful
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

uint8_t broadcastAddress[] = {0xB4, 0xE6, 0x2D, 0xF9, 0x90, 0x45};

typedef struct {
  float euler0[3];
  float euler1[3];
  float euler2[3];
  float euler3[3];
  float euler4[3];
  float euler5[3];
} data_t;

data_t angles;
Quaternion nxp_q, zRotation0, zRotation1, zRotation2, zRotation3, zRotation4;

void setup() {
  Serial.begin(115200);
  Serial.println("Setting up wifi...");
  
  WiFi.mode(WIFI_STA);
  
  // Get Mac Add
  Serial.print("Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("ESP-Now Sender");
  
  // Initializing the ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Problem during ESP-NOW init");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  Serial.println("Added peer ESP32.");
  
  esp_now_register_send_cb(onSent);
  
  Serial.println("Successfully initialized wifi.");
  
  Serial.println(F("\nInitializing I2C devices..."));
  Serial.println(F("Sensor0:"));
  init_sensor(sensor0, &offsets[0], I2C_BUS0_SDA, I2C_BUS0_SCL);
  Serial.println(F("Sensor1:"));
  init_sensor(sensor1, &offsets[6], I2C_BUS12_SDA, I2C_BUS12_SCL);
  Serial.println(F("Sensor2:"));
  init_sensor(sensor2, &offsets[12], I2C_BUS12_SDA, I2C_BUS12_SCL);
  Serial.println(F("Sensor3:"));
  init_sensor(sensor3, &offsets[18], I2C_BUS34_SDA, I2C_BUS34_SCL);
  Serial.println(F("Sensor4:"));
  init_sensor(sensor4, &offsets[24], I2C_BUS34_SDA, I2C_BUS34_SCL);
  Serial.println(F("Sensor5:"));
  init_NXP(I2C_BUS34_SDA, I2C_BUS34_SCL);

  Serial.println(F("\nInitializing I2C devices..."));
  Serial.println(F("Sensor0:"));
  program_sensor(sensor0, I2C_BUS0_SDA, I2C_BUS0_SCL);
  Serial.println(F("Sensor1:"));
  program_sensor(sensor1, I2C_BUS12_SDA, I2C_BUS12_SCL);
  Serial.println(F("Sensor2:"));
  program_sensor(sensor2, I2C_BUS12_SDA, I2C_BUS12_SCL);
  Serial.println(F("Sensor3:"));
  program_sensor(sensor3, I2C_BUS34_SDA, I2C_BUS34_SCL);
  Serial.println(F("Sensor4:"));
  program_sensor(sensor4, I2C_BUS34_SDA, I2C_BUS34_SCL);
  
  filter.begin(FILTER_UPDATE_RATE_HZ);
  unsigned long t;
  while (true) {
    t = millis();
    read_sensors();
    print_euler(angles.euler0);
    Serial.print(" | ");
    print_euler(angles.euler1);
    Serial.print(" | ");
    print_euler(angles.euler2);
    Serial.print(" | ");
    print_euler(angles.euler3);
    Serial.print(" | ");
    print_euler(angles.euler4);
    Serial.print(" | ");
    print_euler(angles.euler5);
    
    Serial.print(" - ");
    Serial.println(millis() - t);

    send_data();
  }
}

void loop() {
//  Serial.println(F("\ni=init, p=program, c=calibrate, r=read: "));
//  while (!Serial.available());
//  char c = Serial.read();
//
//  if (c == 'i') {
//    Serial.println(F("\nInitializing I2C devices..."));
//    Serial.println(F("Sensor0:"));
//    init_sensor(sensor0, &offsets[0], I2C_BUS0_SDA, I2C_BUS0_SCL);
//    Serial.println(F("Sensor1:"));
//    init_sensor(sensor1, &offsets[6], I2C_BUS12_SDA, I2C_BUS12_SCL);
//    Serial.println(F("Sensor2:"));
//    init_sensor(sensor2, &offsets[12], I2C_BUS12_SDA, I2C_BUS12_SCL);
//    Serial.println(F("Sensor3:"));
//    init_sensor(sensor3, &offsets[18], I2C_BUS34_SDA, I2C_BUS34_SCL);
//    Serial.println(F("Sensor4:"));
//    init_sensor(sensor4, &offsets[24], I2C_BUS34_SDA, I2C_BUS34_SCL);
//    Serial.println(F("Sensor5:"));
//    init_NXP(I2C_BUS34_SDA, I2C_BUS34_SCL);
//  } else if (c == 'p') {
//    Serial.println(F("\nInitializing I2C devices..."));
//    Serial.println(F("Sensor0:"));
//    program_sensor(sensor0, I2C_BUS0_SDA, I2C_BUS0_SCL);
//    Serial.println(F("Sensor1:"));
//    program_sensor(sensor1, I2C_BUS12_SDA, I2C_BUS12_SCL);
//    Serial.println(F("Sensor2:"));
//    program_sensor(sensor2, I2C_BUS12_SDA, I2C_BUS12_SCL);
//    Serial.println(F("Sensor3:"));
//    program_sensor(sensor3, I2C_BUS34_SDA, I2C_BUS34_SCL);
//    Serial.println(F("Sensor4:"));
//    program_sensor(sensor4, I2C_BUS34_SDA, I2C_BUS34_SCL);
//  } else if (c == 'c') {
//    Serial.println(F("\nInitializing I2C devices..."));
//    Serial.println(F("Sensor0:"));
//    calibrate_sensor(sensor0, I2C_BUS0_SDA, I2C_BUS0_SCL);
//    Serial.println(F("Sensor1:"));
//    calibrate_sensor(sensor1, I2C_BUS12_SDA, I2C_BUS12_SCL);
//    Serial.println(F("Sensor2:"));
//    calibrate_sensor(sensor2, I2C_BUS12_SDA, I2C_BUS12_SCL);
//    Serial.println(F("Sensor3:"));
//    calibrate_sensor(sensor3, I2C_BUS34_SDA, I2C_BUS34_SCL);
//    Serial.println(F("Sensor4:"));
//    calibrate_sensor(sensor4, I2C_BUS34_SDA, I2C_BUS34_SCL);
//  } else if (c == 'r') {
//    filter.begin(FILTER_UPDATE_RATE_HZ);
//    unsigned long t;
//    while (true) {
//      t = millis();
//      read_sensors();
//      print_euler(angles.euler0);
//      Serial.print(" | ");
//      print_euler(angles.euler1);
//      Serial.print(" | ");
//      print_euler(angles.euler2);
//      Serial.print(" | ");
//      print_euler(angles.euler3);
//      Serial.print(" | ");
//      print_euler(angles.euler4);
//      Serial.print(" | ");
//      print_euler(angles.euler5);
//      
////      Serial.println();
//      Serial.print(" - ");
//      Serial.println(millis() - t);
//
//      send_data();
//    }
//  }
}

void print_euler(float* euler) {
  Serial.print(euler[0] * 180/M_PI);
  Serial.print(" ");
  Serial.print(euler[1] * 180/M_PI);
  Serial.print(" ");
  Serial.print(euler[2] * 180/M_PI);
}

void mpu_read_sensor(MPU6050 &sensor, float *euler, Quaternion &zRotation) {
  Quaternion q;
  float euler_tmp[3];
  getFIFOReadingAtReset(sensor, fifoBuffer);
  
  sensor.dmpGetQuaternion(&q, fifoBuffer);
//  Quaternion qf = nxp_q.getConjugate().getProduct(q);
//  sensor.dmpGetEuler(euler_tmp, &qf);

  if (abs(angles.euler5[0]) < 0.1 && abs(angles.euler5[1]) < 0.1) {
    float theta_z0 = atan2(q.z, q.w);
    zRotation = {cos(theta_z0), 0, 0, sin(theta_z0)};
  }
  float theta_z1= atan2(nxp_q.z, nxp_q.w);
  Quaternion zRotation5 = {cos(theta_z1), 0, 0, sin(theta_z1)};
  Quaternion qt = zRotation.getConjugate().getProduct(q);
  qt = zRotation5.getProduct(qt);
  Quaternion qf = nxp_q.getConjugate().getProduct(qt);
  
  sensor.dmpGetEuler(euler_tmp, &qf);
  
  euler[0] = -euler_tmp[1];
  euler[1] = euler_tmp[2];
  euler[2] = euler_tmp[0];
}

void nxp_read_sensor(float *euler) {
  sensors_event_t accel, gyro, mag;
  accelerometer->getEvent(&accel);
  gyroscope->getEvent(&gyro);
  magnetometer->getEvent(&mag);
  cal.calibrate(mag);
  cal.calibrate(accel);
  cal.calibrate(gyro);
  float gx, gy, gz;
  gx = gyro.gyro.x * SENSORS_RADS_TO_DPS;
  gy = gyro.gyro.y * SENSORS_RADS_TO_DPS;
  gz = gyro.gyro.z * SENSORS_RADS_TO_DPS;
  filter.update(gx, gy, gz, 
                accel.acceleration.x, accel.acceleration.y, accel.acceleration.z, 
                mag.magnetic.x, mag.magnetic.y, mag.magnetic.z);

  filter.getQuaternion(&nxp_q.w, &nxp_q.x, &nxp_q.y, &nxp_q.z);
  float euler_tmp[3];
  sensor0.dmpGetEuler(euler_tmp, &nxp_q);
  euler[0] = -euler_tmp[1];
  euler[1] = euler_tmp[2];
  euler[2] = euler_tmp[0];
}

void read_sensors() {
  Wire.begin(I2C_BUS0_SDA, I2C_BUS0_SCL, I2C_FREQ);
  sensor0.resetFIFO();
  Wire.begin(I2C_BUS12_SDA, I2C_BUS12_SCL, I2C_FREQ);
  sensor1.resetFIFO();
  sensor2.resetFIFO();
  Wire.begin(I2C_BUS34_SDA, I2C_BUS34_SCL, I2C_FREQ);
  sensor3.resetFIFO();
  sensor4.resetFIFO();

  nxp_read_sensor(angles.euler5);
  
  Wire.begin(I2C_BUS0_SDA, I2C_BUS0_SCL, I2C_FREQ);
  
  mpu_read_sensor(sensor0, angles.euler0, zRotation0);
  
  Wire.begin(I2C_BUS12_SDA, I2C_BUS12_SCL, I2C_FREQ);
  
  mpu_read_sensor(sensor1, angles.euler1, zRotation1);
  
  mpu_read_sensor(sensor2, angles.euler2, zRotation2);
  
  Wire.begin(I2C_BUS34_SDA, I2C_BUS34_SCL, I2C_FREQ);
  
  mpu_read_sensor(sensor3, angles.euler3, zRotation3);

  mpu_read_sensor(sensor4, angles.euler4, zRotation4);
}

void calibrate_sensor(MPU6050 &sensor, int sda, int scl) {
  Wire.begin(sda, scl, I2C_FREQ);

  sensor.CalibrateAccel(6);
  sensor.CalibrateGyro(6);
  sensor.PrintActiveOffsets();
}

void program_sensor(MPU6050 &sensor, int sda, int scl) {
  Wire.begin(sda, scl, I2C_FREQ);
  
  // load and configure the DMP
  Serial.println(F("Initializing DMP..."));
  int devStatus = sensor.dmpInitialize();


  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
      
      // turn on the DMP, now that it's ready
      Serial.println(F("Enabling DMP..."));
      sensor.setDMPEnabled(true);

      // get expected DMP packet size for later comparison
      packetSize = sensor.dmpGetFIFOPacketSize();
  } else {
      // ERROR!
      // 1 = initial memory load failed
      // 2 = DMP configuration updates failed
      // (if it's going to break, usually the code will be 1)
      Serial.print(F("DMP Initialization failed (code "));
      Serial.print(devStatus);
      Serial.println(F(")"));
  }
}

void send_data() {
  esp_now_send(NULL, (uint8_t *) &angles, sizeof(data_t));
}

void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
//  Serial.print("Status: ");
//  Serial.println(sendStatus);
}

void init_sensor(MPU6050 &sensor, const int *offsets, int sda, int scl) {
  Wire.begin(sda, scl, I2C_FREQ);

  // initialize device
  sensor.initialize();
  sensor.setRate(7);

  // verify connection
  Serial.println(F("Testing device connection..."));
  Serial.println(sensor.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  sensor1.setXAccelOffset(offsets[0]);
  sensor1.setYAccelOffset(offsets[1]);
  sensor1.setZAccelOffset(offsets[2]);
  sensor1.setXGyroOffset(offsets[3]);
  sensor1.setYGyroOffset(offsets[4]);
  sensor1.setZGyroOffset(offsets[5]);
  Serial.println(F("Calibration loaded."));
}

void init_NXP(int sda, int scl) {
  Wire.begin(sda, scl, I2C_FREQ);
  cal.begin();
  if (! cal.loadCalibration()) {
    Serial.println("No calibration loaded/found");
  } else {
    Serial.println("Calibration loaded.");
  }
  if (!fxos.begin() || !fxas.begin()) {
    Serial.println("NXP connection failed");
    return;
  }
  
  accelerometer = fxos.getAccelerometerSensor();
  gyroscope = &fxas;
  magnetometer = fxos.getMagnetometerSensor();

  Serial.println("NXP connection successful");
}

void getFIFOReadingAtReset(MPU6050 &sensor, uint8_t *data) {
  while (sensor.getFIFOCount() < DMP_PACKET_SIZE) {}
  sensor.getFIFOBytes(data, DMP_PACKET_SIZE);
}
