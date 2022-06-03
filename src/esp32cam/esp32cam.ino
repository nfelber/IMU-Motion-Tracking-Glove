#include <WiFi.h>
#include <esp_now.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

const int RED_PIN = 13;
const int GREEN_PIN = 14;
const int BLUE_PIN = 15;

typedef struct {
  float roll;
  float pitch;
  float yaw;
} angle_t;

typedef struct {
  float x;
  float y;
  float z;
} Vector3;

typedef struct {
  angle_t euler0; //thumb
  angle_t euler1; //forefinger 
  angle_t euler2; //middle
  angle_t euler3; //ring
  angle_t euler4; //pinky
  angle_t euler5; //backHand
} data_t;

data_t angles;

typedef struct {
  float size;
  angle_t angle;
} Bone;

typedef struct {
  std::vector<Bone> bones;
} Finger;

Finger thumb;
Finger forefinger;
Finger middle;
Finger ring;
Finger little;

typedef struct {
  Vector3 palm_coords[1];
  Vector3 thumb_coords[3];
  Vector3 forefinger_coords[4];
  Vector3 middle_coords[4];
  Vector3 ring_coords[4];
  Vector3 little_coords[4];

  angle_t palm_euler;
  angle_t thumb_euler;
  angle_t forefinger_euler;
  angle_t middle_euler;
  angle_t ring_euler;
  angle_t little_euler;
} model_data_t;

// Bluetooth specific
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID        "87f0e59e-08a7-4a00-9c0e-ed3732924c41"
#define CHARACTERISTIC_UUID "dc931335-7019-4096-b1e7-42a29e570f8f"

std::vector<Vector3> forward_kinematics(Finger &finger) {
  Vector3 lastPoint = { 0.0, 0.0, 0.0 };
  angle_t lastAngle = { 0.0, 0.0, 0.0 };
  std::vector<Vector3> v;
  for(const Bone& bone: finger.bones) {
    lastAngle = { lastAngle.roll + bone.angle.roll,
                  lastAngle.pitch + bone.angle.pitch,
                  lastAngle.yaw + bone.angle.yaw };
    Vector3 new_vec = rotate((Vector3) { bone.size, 0.0, 0.0 }, &lastAngle);
    lastPoint = { lastPoint.x + new_vec.x, lastPoint.y + new_vec.y, lastPoint.z + new_vec.z };
    v.push_back(lastPoint);
  }
  return v;
}

Vector3 rotate(Vector3 v, angle_t *angle) {
  float cr = cosf(angle->roll);
  float cp = cosf(angle->pitch);
  float cy = cosf(angle->yaw);
  float sr = sinf(angle->roll);
  float sp = sinf(angle->pitch);
  float sy = sinf(angle->yaw);

  float cysp = cy * sp;
  float sysp = sy * sp;

  return (Vector3) { 
    cy*cp * v.x + (cysp*sr - sy*cr) * v.y + (cysp*cr + sy*sr) * v.z,
    sy*cp * v.x + (sysp*sr + cy*cr) * v.y + (sysp*cr + cy*sr) * v.z,
    -sp * v.x + cp*sr * v.y + cp*cr * v.z };
}

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  memcpy(&angles, incomingData, sizeof(data_t));

  model_data_t m_data = {0};

  thumb.bones = { (Bone) {0.0, angles.euler5 },
                  (Bone) {7.0, (angle_t) {-45.0 / 180.0 * PI, -30.0 / 180.0 * PI, -45.0 / 180.0 * PI} },
                  (Bone) {5.0, (angle_t) {0.0, angles.euler0.pitch / 2, 0.0} },
                  (Bone) {3.0, (angle_t) {0.0, angles.euler0.pitch / 2, 0.0} } };
                  
  forefinger.bones = { (Bone) {0.0, angles.euler5 },
                  (Bone) {10.0, (angle_t) {0.0, 0.0, -10.0 / 180.0 * PI} },
                  (Bone) {4.0, (angle_t) {0.0, angles.euler1.pitch / 3, 0.0} },
                  (Bone) {3.0, (angle_t) {0.0, angles.euler1.pitch / 3, 0.0}},
                  (Bone) {3.0, (angle_t) {0.0, angles.euler1.pitch / 3, 0.0}} 
                  };
                  
  middle.bones = { (Bone) {0.0, angles.euler5 },
                  (Bone) {10.0, (angle_t) {0.0, 0.0, 0.0} },
                  (Bone) {4.0, (angle_t) {0.0, angles.euler2.pitch / 3, 0.0}},
                  (Bone) {3.0, (angle_t) {0.0, angles.euler2.pitch / 3, 0.0}},
                  (Bone) {3.0, (angle_t) {0.0, angles.euler2.pitch / 3, 0.0}} };
                  
  ring.bones = { (Bone) {0.0, angles.euler5 },
                  (Bone) {10.0, (angle_t) {0.0, 0.0, 10.0 / 180.0 * PI} },
                  (Bone) {4.0, (angle_t) {0.0, angles.euler3.pitch / 3, 0.0}},
                  (Bone) {3.0, (angle_t) {0.0, angles.euler3.pitch / 3, 0.0}},
                  (Bone) {3.0, (angle_t) {0.0, angles.euler3.pitch / 3, 0.0}} };
                  
  little.bones = { (Bone) {0.0, angles.euler5 },
                  (Bone) {10.0, (angle_t) {0.0, 0.0, 20.0 / 180.0 * PI} },
                  (Bone) {4.0, (angle_t) {0.0, angles.euler4.pitch / 3, 0.0}},
                  (Bone) {3.0, (angle_t) {0.0, angles.euler4.pitch / 3, 0.0}},
                  (Bone) {3.0, (angle_t) {0.0, angles.euler4.pitch / 3, 0.0}} };
  
  std::vector<Vector3> thumb_skel = forward_kinematics(thumb);
  std::copy(++thumb_skel.begin(), thumb_skel.end(), m_data.thumb_coords);

  std::vector<Vector3> forefinger_skel = forward_kinematics(forefinger);
  std::copy(++forefinger_skel.begin(), forefinger_skel.end(), m_data.forefinger_coords);
  
  std::vector<Vector3> middle_skel = forward_kinematics(middle);
  std::copy(++middle_skel.begin(), middle_skel.end(), m_data.middle_coords);
  
  std::vector<Vector3> ring_skel = forward_kinematics(ring);
  std::copy(++ring_skel.begin(), ring_skel.end(), m_data.ring_coords);
  
  std::vector<Vector3> little_skel = forward_kinematics(little);
  std::copy(++little_skel.begin(), little_skel.end(), m_data.little_coords);

  m_data.palm_euler = angles.euler5;
  m_data.thumb_euler = angles.euler0;
  m_data.forefinger_euler = angles.euler1;
  m_data.middle_euler = angles.euler2;
  m_data.ring_euler = angles.euler3;
  m_data.little_euler = angles.euler4;

  pCharacteristic->setValue((uint8_t*)(&m_data), sizeof(m_data));
  Serial.println("Data sent");  
  pCharacteristic->notify();
}

class MyServerCallbacks: public BLEServerCallbacks
{
    void onConnect(BLEServer* pServer)
    {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer)
    {
      deviceConnected = false;
    }
};

void setup()
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  /**
   * WiFi for ESP8266 comms
   */

  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  Serial.print("Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("ESP32 ESP-Now Broadcast");
  
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);


  /**
   * Bluetooth for driver comms
   */

  BLEDevice::init("MTG");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic
  (
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x00);
  BLEDevice::startAdvertising();
}

void loop()
{
  // notify changed value
  if (deviceConnected) {
      double t = millis() / 1000.0;
      analogWrite(RED_PIN, (int) ((1+sin(t)) * 127));
      analogWrite(GREEN_PIN, (int) ((1+sin(2*t)) * 127));
      analogWrite(BLUE_PIN, (int) ((1+sin(3*t)) * 127));
      delay(5); // safest lowest
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }

  // connecting
  if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }
}
