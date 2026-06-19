#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "DFRobot_BloodOxygen_S.h"
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_Sensor.h>


const char* ssid = "WIFINAME";
const char* password = "PASSWORD";


WebServer server(80);
DFRobot_BloodOxygen_S_I2C max30102(&Wire, 0x57);
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();


int currentBPM = 0;
int currentSPO2 = 0;
float currentMovement = 0.0;
float hrvValue = 0.0;

const int EPOCH_DURATION_SEC = 30;
int bpmSamples[30];
float moveSamples[30];
int sampleCount = 0;
unsigned long lastSampleTime = 0;
int epochNumber = 1;


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="utf-8">
  <title>Monitor Snu ESP32</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI', Arial, sans-serif; text-align: center; background-color: #0B101E; color: #E2E8F0; margin-top: 40px;}
    .container { display: flex; flex-wrap: wrap; justify-content: center; max-width: 850px; margin: auto; }
    .card { background-color: #1A233A; border-radius: 20px; padding: 30px; margin: 15px; width: 220px; box-shadow: 0px 8px 20px rgba(0,0,0,0.4); border: 1px solid #2D3A56; transition: transform 0.3s ease; }
    .card:hover { transform: translateY(-5px); }
    h1 { color: #818CF8; margin-bottom: 5px; font-weight: 300; letter-spacing: 2px;}
    h3 { color: #64748B; margin-bottom: 40px; font-weight: 400;}
    .value { font-size: 3.5rem; font-weight: bold; margin: 15px 0; }
    .label { font-size: 1.1rem; color: #94A3B8; text-transform: uppercase; letter-spacing: 1px;}
    
    /* Kolory dobrane pod motyw nocny / senny */
    .bpm-color { color: #F472B6; } /* Delikatny różowy */
    .spo-color { color: #38BDF8; } /* Błękitny */
    .hrv-color { color: #A78BFA; } /* Wyciszający fiolet */
    .mov-color { color: #34D399; } /* Miętowa zieleń */
  </style>
  <script>
    setInterval(function() {
      fetch('/data').then(response => response.json()).then(data => {
        document.getElementById("bpm").innerHTML = data.bpm > 0 ? data.bpm : "---";
        document.getElementById("spo2").innerHTML = data.spo2 > 0 ? data.spo2 + "%" : "---";
        document.getElementById("hrv").innerHTML = data.hrv > 0 ? data.hrv.toFixed(1) : "---";
        document.getElementById("mov").innerHTML = data.movement.toFixed(2);
        document.getElementById("epoch").innerHTML = data.epoch;
      });
    }, 1000);
  </script>
</head>
<body>
  <h1>MONITOR SNU</h1>
  <h3>Aktywna epoka badawcza: <span id="epoch">1</span> (Zapis co 30s)</h3>
  <div class="container">
    <div class="card"><div class="label">Tętno Snu</div><div class="value bpm-color" id="bpm">---</div></div>
    <div class="card"><div class="label">Saturacja (SpO2)</div><div class="value spo-color" id="spo2">---</div></div>
    <div class="card"><div class="label">Zmienność (HRV)</div><div class="value hrv-color" id="hrv">---</div></div>
    <div class="card"><div class="label">Aktywność Ruchowa</div><div class="value mov-color" id="mov">---</div></div>
  </div>
</body>
</html>)rawliteral";

void handleRoot() { server.send(200, "text/html", index_html); }
void handleData() {
  String json = "{\"bpm\":"; json += currentBPM;
  json += ",\"spo2\":"; json += currentSPO2;
  json += ",\"hrv\":"; json += hrvValue;
  json += ",\"movement\":"; json += currentMovement;
  json += ",\"epoch\":"; json += epochNumber;
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  Serial.print("\nLaczenie z WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nPolaczono!");
  Serial.print("=== ADRES DO PRZEGLADARKI: "); Serial.print(WiFi.localIP()); Serial.println(" ===");

  max30102.begin();
  max30102.sensorStartCollect();
  lsm.begin();
  lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Serial.println("\n--- POCZATEK PLIKU CSV ---");
  Serial.println("Epoka,BPM_Srednie,SPO2,HRV,Energia_Ruchu");
}

void loop() {
  server.handleClient();

  if (millis() - lastSampleTime >= 1000) {
    lastSampleTime = millis();


    max30102.getHeartbeatSPO2();
    int rawBPM = max30102._sHeartbeatSPO2.Heartbeat;
    int rawSPO2 = max30102._sHeartbeatSPO2.SPO2;

    lsm.read();
    sensors_event_t a, m, g, temp;
    lsm.getEvent(&a, &m, &g, &temp);
    float totalAccel = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
    currentMovement = abs(totalAccel - 9.81);

    if (rawBPM > 40 && rawBPM < 200) {
      bpmSamples[sampleCount] = rawBPM;
      moveSamples[sampleCount] = currentMovement;
      currentSPO2 = rawSPO2;
      sampleCount++;
    }

    if (sampleCount >= EPOCH_DURATION_SEC) {
      

      long bpmSum = 0;
      for (int i = 0; i < EPOCH_DURATION_SEC; i++) bpmSum += bpmSamples[i];
      currentBPM = bpmSum / EPOCH_DURATION_SEC;


      float varianceSum = 0;
      for (int i = 0; i < EPOCH_DURATION_SEC; i++) {
        varianceSum += pow(bpmSamples[i] - currentBPM, 2);
      }
      hrvValue = sqrt(varianceSum / EPOCH_DURATION_SEC);


      float totalMoveEnergy = 0;
      for (int i = 0; i < EPOCH_DURATION_SEC; i++) totalMoveEnergy += moveSamples[i];

      Serial.print(epochNumber); Serial.print(",");
      Serial.print(currentBPM);  Serial.print(",");
      Serial.print(currentSPO2); Serial.print(",");
      Serial.print(hrvValue);    Serial.print(",");
      Serial.println(totalMoveEnergy);


      sampleCount = 0;
      epochNumber++;
    }
  }
}