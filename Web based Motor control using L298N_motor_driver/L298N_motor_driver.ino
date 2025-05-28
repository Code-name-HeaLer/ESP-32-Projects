#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "Nanda wifi";
const char* password = "Nanda@41148";

// Motor control pins
const int ENA = 5;   // PWM pin for speed control (Connect to L298N ENA)
const int IN1 = 18;  // Direction control pin 1 (Connect to L298N IN1)
const int IN2 = 19;  // Direction control pin 2 (Connect to L298N IN2)

// PWM properties
const int freq = 1000;     // PWM frequency
const int pwmChannel = 0;  // PWM channel
const int resolution = 8;  // PWM resolution (8-bit = 0-255)

// Motor control variables
int motorSpeed = 0;        // Speed: 0-255
int motorDirection = 1;    // 1 = forward, -1 = reverse, 0 = stop

WebServer server(80);

void setup() {
  Serial.begin(115200);
  
  // Initialize motor control pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  
  // Configure PWM - For newer ESP32 core (3.x)
  ledcAttach(ENA, freq, resolution);
  
  // Stop motor initially
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  ledcWrite(ENA, 0);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Define web server routes
  server.on("/", handleRoot);
  server.on("/motor/forward", handleForward);
  server.on("/motor/reverse", handleReverse);
  server.on("/motor/stop", handleStop);
  server.on("/motor/speed", handleSpeed);
  server.on("/status", handleStatus);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

// Web page HTML
void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>ESP32 Motor Control</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body {";
  html += "  font-family: Arial, sans-serif;";
  html += "  text-align: center;";
  html += "  margin: 50px;";
  html += "  background-color: #f0f0f0;";
  html += "}";
  html += ".container {";
  html += "  max-width: 400px;";
  html += "  margin: 0 auto;";
  html += "  background: white;";
  html += "  padding: 30px;";
  html += "  border-radius: 10px;";
  html += "  box-shadow: 0 4px 6px rgba(0,0,0,0.1);";
  html += "}";
  html += "h1 {";
  html += "  color: #333;";
  html += "  margin-bottom: 30px;";
  html += "}";
  html += ".control-group {";
  html += "  margin: 20px 0;";
  html += "}";
  html += "button {";
  html += "  padding: 15px 25px;";
  html += "  margin: 10px;";
  html += "  font-size: 16px;";
  html += "  border: none;";
  html += "  border-radius: 5px;";
  html += "  cursor: pointer;";
  html += "  color: white;";
  html += "  min-width: 120px;";
  html += "}";
  html += ".forward { background-color: #4CAF50; }";
  html += ".reverse { background-color: #FF9800; }";
  html += ".stop { background-color: #f44336; }";
  html += ".forward:hover { background-color: #45a049; }";
  html += ".reverse:hover { background-color: #e68900; }";
  html += ".stop:hover { background-color: #da190b; }";
  html += ".speed-control {";
  html += "  margin: 20px 0;";
  html += "}";
  html += "input[type=\"range\"] {";
  html += "  width: 100%;";
  html += "  margin: 10px 0;";
  html += "}";
  html += ".status {";
  html += "  margin: 20px 0;";
  html += "  padding: 15px;";
  html += "  background-color: #e7f3ff;";
  html += "  border-radius: 5px;";
  html += "}";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"container\">";
  html += "<h1>Motor Control Panel</h1>";
  
  html += "<div class=\"control-group\">";
  html += "<button class=\"forward\" onclick=\"motorForward()\">Forward</button>";
  html += "<button class=\"reverse\" onclick=\"motorReverse()\">Reverse</button>";
  html += "<button class=\"stop\" onclick=\"motorStop()\">Stop</button>";
  html += "</div>";
  
  html += "<div class=\"speed-control\">";
  html += "<label for=\"speedSlider\">Speed Control:</label><br>";
  html += "<input type=\"range\" id=\"speedSlider\" min=\"0\" max=\"255\" value=\"128\" onchange=\"setSpeed(this.value)\">";
  html += "<div>Speed: <span id=\"speedValue\">128</span>/255</div>";
  html += "</div>";
  
  html += "<div class=\"status\" id=\"status\">";
  html += "Direction: Stopped<br>";
  html += "Speed: 128/255";
  html += "</div>";
  
  html += "<button onclick=\"updateStatus()\" style=\"background-color: #2196F3;\">Refresh Status</button>";
  html += "</div>";

  html += "<script>";
  html += "function motorForward() {";
  html += "  fetch('/motor/forward')";
  html += "    .then(() => updateStatus())";
  html += "    .catch(err => console.error('Error:', err));";
  html += "}";
  
  html += "function motorReverse() {";
  html += "  fetch('/motor/reverse')";
  html += "    .then(() => updateStatus())";
  html += "    .catch(err => console.error('Error:', err));";
  html += "}";
  
  html += "function motorStop() {";
  html += "  fetch('/motor/stop')";
  html += "    .then(() => updateStatus())";
  html += "    .catch(err => console.error('Error:', err));";
  html += "}";
  
  html += "function setSpeed(speed) {";
  html += "  document.getElementById('speedValue').textContent = speed;";
  html += "  fetch('/motor/speed?value=' + speed)";
  html += "    .then(() => updateStatus())";
  html += "    .catch(err => console.error('Error:', err));";
  html += "}";
  
  html += "function updateStatus() {";
  html += "  fetch('/status')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      let direction = 'Stopped';";
  html += "      if (data.direction === 1) direction = 'Forward';";
  html += "      else if (data.direction === -1) direction = 'Reverse';";
  html += "      document.getElementById('status').innerHTML = ";
  html += "        'Direction: ' + direction + '<br>Speed: ' + data.speed + '/255';";
  html += "    })";
  html += "    .catch(err => console.error('Error:', err));";
  html += "}";
  
  html += "setInterval(updateStatus, 2000);";
  html += "</script>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handleForward() {
  motorDirection = 1;
  setMotorDirection();
  server.send(200, "text/plain", "Motor moving forward");
  Serial.println("Motor: Forward");
}

void handleReverse() {
  motorDirection = -1;
  setMotorDirection();
  server.send(200, "text/plain", "Motor moving reverse");
  Serial.println("Motor: Reverse");
}

void handleStop() {
  motorDirection = 0;
  motorSpeed = 0;
  setMotorDirection();
  ledcWrite(ENA, 0);
  server.send(200, "text/plain", "Motor stopped");
  Serial.println("Motor: Stopped");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    int newSpeed = server.arg("value").toInt();
    if (newSpeed >= 0 && newSpeed <= 255) {
      motorSpeed = newSpeed;
      ledcWrite(ENA, motorSpeed);
      server.send(200, "text/plain", "Speed set to " + String(motorSpeed));
      Serial.println("Speed set to: " + String(motorSpeed));
    } else {
      server.send(400, "text/plain", "Invalid speed value. Use 0-255");
    }
  } else {
    server.send(400, "text/plain", "Missing speed value");
  }
}

void handleStatus() {
  String json = "{";
  json += "\"speed\":" + String(motorSpeed) + ",";
  json += "\"direction\":" + String(motorDirection);
  json += "}";
  
  server.send(200, "application/json", json);
}

void setMotorDirection() {
  if (motorDirection == 1) {
    // Forward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, motorSpeed);
  } else if (motorDirection == -1) {
    // Reverse
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    ledcWrite(ENA, motorSpeed);
  } else {
    // Stop
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, 0);
  }
}