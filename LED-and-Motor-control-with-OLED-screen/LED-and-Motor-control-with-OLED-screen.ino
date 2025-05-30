#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- WiFi Credentials ---
const char* ssid = "Nanda wifi";         // ***** REPLACE WITH YOUR WIFI SSID *****
const char* password = "Nanda@41148"; // ***** REPLACE WITH YOUR WIFI PASSWORD *****

WebServer server(80);

// --- OLED Display ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_I2C_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Single LED ---
const int singleLedPin = 13;
const int ledPwmFreq = 5000;    // Frequency for LED PWM
const int ledPwmResolution = 8; // 8-bit resolution (0-255)
bool singleLedState = false;
int singleLedBrightness = 128;

// --- DC Motor (L298N) ---
const int motorEnablePin = 32;    // ENA on L298N (PWM pin)
const int motorIn1Pin = 25;       // IN1 on L298N
const int motorIn2Pin = 26;       // IN2 on L298N
const int motorPwmFreq = 1000;    // Frequency for Motor PWM (as in your example)
const int motorPwmResolution = 8; // 8-bit resolution
// Motor control variables (from your working example)
int motorSpeed = 128;        // Speed: 0-255 (current speed applied)
int motorDirection = 0;    // 1 = forward, -1 = reverse, 0 = stop (current direction applied)
                             // Note: motorIsOn is effectively handled by motorDirection != 0 and motorSpeed > 0

unsigned long previousOledMillis = 0;
const long oledUpdateInterval = 1000;

// --- Function Prototypes ---
void handleRoot();
void handleSetLed();
// Motor handlers from your working example
void handleMotorForward();
void handleMotorReverse();
void handleMotorStop();
void handleSetMotorSpeed(); // Renamed from handleSpeed for clarity
void handleGetStatus();     // For AJAX updates from webpage
void updateOLEDDisplay();
void applyLedControl();
void applyMotorControl(); // Renamed from setMotorDirection to be more general

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 Smart Control System Booting...");

  // --- Initialize Single LED (using ledcAttach and ledcWrite with PIN) ---
  pinMode(singleLedPin, OUTPUT);
  ledcAttach(singleLedPin, ledPwmFreq, ledPwmResolution); // Attach PIN directly
  applyLedControl(); // Set initial state (off)

  // --- Initialize Motor Pins (using ledcAttach and ledcWrite with PIN) ---
  pinMode(motorIn1Pin, OUTPUT);
  pinMode(motorIn2Pin, OUTPUT);
  pinMode(motorEnablePin, OUTPUT);
  ledcAttach(motorEnablePin, motorPwmFreq, motorPwmResolution); // Attach PIN directly
  applyMotorControl(); // Set initial state (stopped)

  // --- Initialize OLED ---
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Connecting to WiFi..."));
  display.display();
  Serial.println("OLED Initialized.");

  // --- Connect to WiFi ---
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20) {
    delay(500);
    Serial.print(".");
    wifi_attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("WiFi Connected!"));
    display.print(F("IP: "));
    display.println(WiFi.localIP());
    display.display();
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("WiFi Failed!"));
    display.display();
  }

  // --- Setup Web Server Routes ---
  server.on("/", HTTP_GET, handleRoot);
  server.on("/setLed", HTTP_GET, handleSetLed);
  // Motor routes from your working example
  server.on("/motor/forward", HTTP_GET, handleMotorForward);
  server.on("/motor/reverse", HTTP_GET, handleMotorReverse);
  server.on("/motor/stop", HTTP_GET, handleMotorStop);
  server.on("/motor/setSpeed", HTTP_GET, handleSetMotorSpeed); // Use consistent name
  server.on("/status", HTTP_GET, handleGetStatus);

  server.onNotFound([](){
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("HTTP server started.");
  updateOLEDDisplay();
}

void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();
  if (currentMillis - previousOledMillis >= oledUpdateInterval) {
    previousOledMillis = currentMillis;
    updateOLEDDisplay();
  }
  delay(10);
}

// --- HTML and Web Page Handlers ---
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP32 Control</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, Helvetica, sans-serif; margin: 0; padding: 0; background-color: #f0f2f5; display: flex; flex-direction: column; align-items: center; color: #333; }";
  html += ".header { background-color: #4A90E2; color: white; padding: 20px; text-align: center; width: 100%; box-shadow: 0 2px 4px rgba(0,0,0,0.1); margin-bottom: 20px; }";
  html += ".container { display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; padding: 20px; width: 100%; max-width: 900px; }";
  html += ".control-group { background-color: white; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.15); padding: 20px; min-width: 280px; flex-basis: 280px; margin-bottom: 20px; }";
  html += "h2 { color: #4A90E2; margin-top: 0; border-bottom: 1px solid #eee; padding-bottom: 10px;}";
  html += "label { display: block; margin: 10px 0 5px 0; font-weight: bold; }";
  html += "input[type=range] { width: 100%; cursor: pointer; }";
  html += "input[type=checkbox] { transform: scale(1.2); margin-right: 8px; }";
  html += "button { background-color: #5cb85c; color: white; border: none; padding: 10px 15px; margin: 5px; border-radius: 5px; cursor: pointer; font-size: 0.9em; transition: background-color 0.2s ease; }";
  html += "button:hover { opacity: 0.85; }";
  html += ".btn-stop { background-color: #d9534f; }";
  html += ".btn-dir { background-color: #f0ad4e; }";
  html += ".value-display { font-weight: bold; color: #28a745; display: inline-block; min-width: 30px; text-align: right; }";
  html += ".status-text { font-style: italic; color: #555; }";
  html += "</style>";
  html += "<script>";
  html += "function sendCmd(url, callback) { fetch(url).then(res => { if(res.ok) { console.log('OK: '+url); if(callback) callback(true); } else { console.error('Fail: '+url); if(callback) callback(false); } }).catch(err => { console.error('Error: '+err); if(callback) callback(false); }); }";
  html += "function updateSliderDisplay(sliderId, displayId) { document.getElementById(displayId).textContent = document.getElementById(sliderId).value; }";
  // LED
  html += "function setLed() { let S = document.getElementById('ledState').checked ? 'on' : 'off'; let B = document.getElementById('ledBright').value; sendCmd('/setLed?state='+S+'&brightness='+B, updateStatusDisplays); }";
  // Motor
  html += "function motorCmd(cmd) { sendCmd('/motor/'+cmd, updateStatusDisplays); }";
  html += "function setMotorSpeed() { let V = document.getElementById('motorSpeedSlider').value; sendCmd('/motor/setSpeed?value='+V, updateStatusDisplays); }"; // Changed ID to motorSpeedSlider
  // Status Update
  html += "function updateStatusDisplays() {";
  html += " fetch('/status').then(response => response.json()).then(data => {";
  // LED Status
  html += "   document.getElementById('ledStatusText').textContent = data.led.state ? 'ON' : 'OFF';";
  html += "   document.getElementById('ledBrightStatus').textContent = data.led.brightness;";
  html += "   document.getElementById('ledState').checked = data.led.state;";
  html += "   document.getElementById('ledBright').value = data.led.brightness;";
  html += "   updateSliderDisplay('ledBright', 'ledBrightVal');";
  // Motor Status
  html += "   let motorDirText = 'Stopped'; if(data.motor.direction === 1) motorDirText = 'Forward'; else if(data.motor.direction === -1) motorDirText = 'Reverse';";
  html += "   document.getElementById('motorDirStatus').textContent = motorDirText;";
  html += "   document.getElementById('motorSpeedStatus').textContent = data.motor.speed;";
  html += "   document.getElementById('motorSpeedSlider').value = data.motor.speed;"; // Changed ID
  html += "   updateSliderDisplay('motorSpeedSlider', 'motorSpeedVal');"; // Changed ID
  html += " }).catch(err => console.error('Status update error:', err));";
  html += "}";
  html += "window.onload = function() { updateSliderDisplay('ledBright', 'ledBrightVal'); updateSliderDisplay('motorSpeedSlider', 'motorSpeedVal'); updateStatusDisplays(); setInterval(updateStatusDisplays, 3000); };";
  html += "</script></head><body>";
  html += "<div class='header'><h1>ESP32 Universal Controller</h1></div>";
  html += "<div class='container'>";
  // Single LED Control
  html += "<div class='control-group'><h2>LED Control</h2>";
  html += "<label><input type='checkbox' id='ledState' onchange='setLed()'> LED ON/OFF</label>";
  html += "<p class='status-text'>Status: <span id='ledStatusText'>OFF</span>, Brightness: <span id='ledBrightStatus' class='value-display'>0</span>%</p>";
  html += "<label for='ledBright'>Brightness:</label>";
  html += "<input type='range' id='ledBright' min='0' max='100' value='" + String(map(singleLedBrightness, 0, 255, 0, 100)) + "' oninput='updateSliderDisplay(\"ledBright\", \"ledBrightVal\")' onchange='setLed()'>";
  html += "<div style='text-align:right;'><span id='ledBrightVal' class='value-display'>0</span>%</div></div>";
  // Motor Control
  html += "<div class='control-group'><h2>Motor Control</h2>";
  html += "<button class='btn-dir' onclick='motorCmd(\"forward\")'>Forward</button>";
  html += "<button class='btn-dir' onclick='motorCmd(\"reverse\")'>Reverse</button>";
  html += "<button class='btn-stop' onclick='motorCmd(\"stop\")'>Stop</button>";
  html += "<p class='status-text'>Direction: <span id='motorDirStatus'>Stopped</span>, Speed: <span id='motorSpeedStatus' class='value-display'>0</span>/255</p>";
  html += "<label for='motorSpeedSlider'>Speed:</label>"; // Changed ID
  html += "<input type='range' id='motorSpeedSlider' min='0' max='255' value='" + String(motorSpeed) + "' oninput='updateSliderDisplay(\"motorSpeedSlider\", \"motorSpeedVal\")' onchange='setMotorSpeed()'>"; // Changed ID
  html += "<div style='text-align:right;'><span id='motorSpeedVal' class='value-display'>0</span>/255</div></div>";
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleSetLed() {
  bool stateChanged = false;
  if (server.hasArg("state")) {
    singleLedState = (server.arg("state") == "on");
    stateChanged = true;
  }
  if (server.hasArg("brightness")) {
    singleLedBrightness = map(server.arg("brightness").toInt(), 0, 100, 0, 255);
    if (singleLedBrightness < 0) singleLedBrightness = 0;
    if (singleLedBrightness > 255) singleLedBrightness = 255;
    stateChanged = true;
  }
  if (stateChanged) applyLedControl();
  Serial.print("LED state: "); Serial.print(singleLedState ? "ON" : "OFF");
  Serial.print(", Brightness: "); Serial.println(singleLedBrightness);
  server.send(200, "text/plain", "LED OK");
}

// Motor Handlers from your working code
void handleMotorForward() {
  motorDirection = 1;
  applyMotorControl(); // This now sets speed too
  server.send(200, "text/plain", "Motor moving forward");
  Serial.println("Motor: Forward");
}

void handleMotorReverse() {
  motorDirection = -1;
  applyMotorControl(); // This now sets speed too
  server.send(200, "text/plain", "Motor moving reverse");
  Serial.println("Motor: Reverse");
}

void handleMotorStop() {
  motorDirection = 0;
  // motorSpeed = 0; // Speed is maintained, just direction is stopped. Let applyMotorControl handle PWM.
  applyMotorControl();
  server.send(200, "text/plain", "Motor stopped");
  Serial.println("Motor: Stopped");
}

void handleSetMotorSpeed() { // Renamed from handleSpeed for consistency
  if (server.hasArg("value")) {
    int newSpeed = server.arg("value").toInt();
    if (newSpeed >= 0 && newSpeed <= 255) {
      motorSpeed = newSpeed;
      // If speed is set and motor was stopped, it won't move until a direction is chosen
      // Or, if you want it to start in last known/default direction:
      if (motorSpeed > 0 && motorDirection == 0) {
          // motorDirection = 1; // Optional: default to forward if speed is given from stop
      }
      applyMotorControl(); // Apply new speed with current/new direction
      server.send(200, "text/plain", "Speed set to " + String(motorSpeed));
      Serial.println("Speed set to: " + String(motorSpeed));
    } else {
      server.send(400, "text/plain", "Invalid speed value. Use 0-255");
    }
  } else {
    server.send(400, "text/plain", "Missing speed value");
  }
}

void handleGetStatus() {
  String json = "{";
  json += "\"led\":{";
  json += "\"state\":" + String(singleLedState ? "true" : "false") + ",";
  json += "\"brightness\":" + String(map(singleLedBrightness, 0, 255, 0, 100));
  json += "},";
  json += "\"motor\":{";
  json += "\"direction\":" + String(motorDirection) + ",";
  json += "\"speed\":" + String(motorSpeed);
  json += "}";
  json += "}";
  server.send(200, "application/json", json);
}

// --- Control Logic Functions ---
void applyLedControl() {
  if (singleLedState) {
    ledcWrite(singleLedPin, singleLedBrightness); // Use PIN with ledcWrite
  } else {
    ledcWrite(singleLedPin, 0); // Use PIN with ledcWrite
  }
}

void applyMotorControl() { // Based on your setMotorDirection
  if (motorDirection == 1) { // Forward
    digitalWrite(motorIn1Pin, HIGH);
    digitalWrite(motorIn2Pin, LOW);
    ledcWrite(motorEnablePin, motorSpeed); // Use PIN with ledcWrite
  } else if (motorDirection == -1) { // Reverse
    digitalWrite(motorIn1Pin, LOW);
    digitalWrite(motorIn2Pin, HIGH);
    ledcWrite(motorEnablePin, motorSpeed); // Use PIN with ledcWrite
  } else { // Stop (motorDirection == 0)
    digitalWrite(motorIn1Pin, LOW);
    digitalWrite(motorIn2Pin, LOW);
    ledcWrite(motorEnablePin, 0); // Use PIN with ledcWrite
  }
}

// --- OLED Update Function ---
void updateOLEDDisplay() {
  if (WiFi.status() != WL_CONNECTED) return;
  display.clearDisplay();
  display.setCursor(0,0);

  display.print(F("IP: ")); display.println(WiFi.localIP());
  display.drawLine(0, 10, SCREEN_WIDTH-1, 10, SSD1306_WHITE);

  display.setCursor(0, 12);
  display.print(F("LED: ")); display.print(singleLedState ? F("ON ") : F("OFF"));
  if(singleLedState) {
    display.print(map(singleLedBrightness, 0, 255, 0, 100)); display.print(F("%"));
  }
  display.println();

  display.setCursor(0, 22);
  display.print(F("Motor: "));
  if (motorDirection == 0) display.print(F("STOP"));
  else if (motorDirection == 1) display.print(F("FWD "));
  else if (motorDirection == -1) display.print(F("REV "));
  
  // Only show speed if motor is not stopped by direction
  if (motorDirection != 0) {
      display.print(motorSpeed); display.print(F("/255")); // Show actual PWM value
  } else {
      display.print(F("---")); // Indicate stopped
  }
  display.println();

  display.display();
}