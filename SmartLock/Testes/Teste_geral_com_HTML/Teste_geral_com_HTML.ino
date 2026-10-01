#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// =====================================================
// WIFI
// =====================================================

const char* WIFI_SSID = "halabura";
const char* WIFI_PASSWORD = "CirnoStan";

// =====================================================
// PINOS
// =====================================================

// RFID RC522
#define RFID_SS    21
#define RFID_RST   22
#define RFID_SCK   18
#define RFID_MISO  19
#define RFID_MOSI  23

// Servo
#define SERVO_PIN 13

// Reed
#define REED_PIN 27

// Buzzer
#define BUZZER_PIN 26

// LEDs
#define LED_VERDE     25
#define LED_VERMELHO  33
#define LED_AUX       32

// =====================================================
// OBJETOS
// =====================================================

MFRC522 rfid(RFID_SS, RFID_RST);
Servo servo;
WebServer server(80);

// =====================================================
// ESTADOS
// =====================================================

bool portaFechada = false;
bool armarioTrancado = true;

String ultimoUID = "Nenhum";
String ultimoEvento = "Sistema iniciado";

// =====================================================
// POSIÇÕES DO SERVO
// =====================================================

const int SERVO_TRANCADO = 0;
const int SERVO_DESTRAVADO = 90;

// =====================================================
// CONTROLE DE TEMPO
// =====================================================

// Tempo que a porta precisa permanecer fechada
// antes de o armário ser trancado.
const unsigned long TEMPO_RETRAVAMENTO = 1500;

unsigned long momentoPortaFechou = 0;

// =====================================================
// BUZZER
// =====================================================

void beep(int quantidade, int duracao = 100) {

  for (int i = 0; i < quantidade; i++) {

    digitalWrite(BUZZER_PIN, LOW);
    delay(duracao);

    digitalWrite(BUZZER_PIN, HIGH);
    delay(duracao);
  }
}

// =====================================================
// EVENTO
// =====================================================

void registrarEvento(String evento) {

  ultimoEvento = evento;

  Serial.println(evento);
}

// =====================================================
// DESTRAVAR
// =====================================================

void destravarArmario() {

  if (!armarioTrancado) {
    return;
  }

  Serial.println("Destravando armario...");

  servo.write(SERVO_DESTRAVADO);

  armarioTrancado = false;

  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_VERDE, HIGH);

  beep(1, 150);

  registrarEvento("Armario destravado");
}

// =====================================================
// TRANCAR
// =====================================================

void trancarArmario() {

  if (armarioTrancado) {
    return;
  }

  // Segurança:
  // só tranca se a porta estiver fechada.

  if (!portaFechada) {

    registrarEvento(
      "Porta aberta - nao foi possivel trancar"
    );

    return;
  }

  Serial.println("Trancando armario...");

  servo.write(SERVO_TRANCADO);

  delay(500);

  armarioTrancado = true;

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);

  beep(2, 100);

  registrarEvento("Armario trancado");
}

// =====================================================
// REED SWITCH
// =====================================================

void verificarPorta() {

  bool novoEstado = (digitalRead(REED_PIN) == LOW);

  // Mudança de estado
  if (novoEstado != portaFechada) {

    portaFechada = novoEstado;

    if (portaFechada) {

      // -----------------------------------------------
      // PORTA FECHOU
      // -----------------------------------------------

      Serial.println("Porta FECHADA");

      momentoPortaFechou = millis();

      digitalWrite(LED_VERDE, LOW);

      if (armarioTrancado) {
        digitalWrite(LED_VERMELHO, HIGH);
      }

      registrarEvento("Porta fechada");

    } else {

      // -----------------------------------------------
      // PORTA ABRIU
      // -----------------------------------------------

      Serial.println("Porta ABERTA");

      digitalWrite(LED_VERMELHO, LOW);
      digitalWrite(LED_VERDE, HIGH);

      registrarEvento("Porta aberta");
    }
  }

  // ===================================================
  // RETRAVAMENTO AUTOMÁTICO
  // ===================================================

  if (
    portaFechada &&
    !armarioTrancado &&
    momentoPortaFechou > 0 &&
    millis() - momentoPortaFechou >= TEMPO_RETRAVAMENTO
  ) {

    trancarArmario();

    momentoPortaFechou = 0;
  }
}

// =====================================================
// RFID
// =====================================================

void verificarRFID() {

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    uid += String(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1) {
      uid += " ";
    }
  }

  uid.toUpperCase();

  ultimoUID = uid;

  Serial.print("RFID detectado: ");
  Serial.println(uid);

  registrarEvento("RFID detectado: " + uid);

  // ===================================================
  // POR ENQUANTO:
  // qualquer tag é autorizada
  // ===================================================

  if (armarioTrancado) {

    destravarArmario();

  } else {

    registrarEvento(
      "RFID detectado - armario ja esta destrancado"
    );
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(500);
}

// =====================================================
// PÁGINA WEB
// =====================================================

String gerarPagina() {

  String portaStatus;
  String travaStatus;

  String corPorta;
  String corTrava;

  if (portaFechada) {

    portaStatus = "FECHADA";
    corPorta = "#dc2626";

  } else {

    portaStatus = "ABERTA";
    corPorta = "#16a34a";
  }

  if (armarioTrancado) {

    travaStatus = "TRANCADO";
    corTrava = "#dc2626";

  } else {

    travaStatus = "DESTRANCADO";
    corTrava = "#16a34a";
  }

  String html = R"rawliteral(

<!DOCTYPE html>

<html lang="pt-BR">

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width, initial-scale=1.0">

<meta http-equiv="refresh" content="1">

<title>Armário Inteligente</title>

<style>

body {
  margin: 0;
  font-family: Arial;
  background: #111827;
  color: white;
}

.container {
  max-width: 700px;
  margin: auto;
  padding: 20px;
}

h1 {
  text-align: center;
}

.card {
  background: #1f2937;
  border-radius: 15px;
  padding: 20px;
  margin-bottom: 15px;
}

.status {
  text-align: center;
  font-size: 28px;
  font-weight: bold;
  padding: 15px;
  border-radius: 10px;
}

.info {
  font-size: 18px;
  margin: 12px 0;
}

.uid {
  background: #111827;
  padding: 12px;
  border-radius: 8px;
  font-family: monospace;
}

</style>

</head>

<body>

<div class="container">

<h1>🔐 Armário Inteligente</h1>

<div class="card">

<h2>🚪 Porta</h2>

<div class="status"
style="background:)rawliteral";

  html += corPorta;

  html += R"rawliteral(;">
)rawliteral";

  html += portaStatus;

  html += R"rawliteral(
</div>

</div>

<div class="card">

<h2>🔒 Trava</h2>

<div class="status"
style="background:)rawliteral";

  html += corTrava;

  html += R"rawliteral(;">
)rawliteral";

  html += travaStatus;

  html += R"rawliteral(
</div>

</div>

<div class="card">

<h2>📡 RFID</h2>

<div class="info">

Status:
<span style="color:#22c55e">
● ONLINE
</span>

</div>

<div class="info">

Último UID:

</div>

<div class="uid">
)rawliteral";

  html += ultimoUID;

  html += R"rawliteral(
</div>

</div>

<div class="card">

<h2>📋 Último evento</h2>

<div class="info">
)rawliteral";

  html += ultimoEvento;

  html += R"rawliteral(
</div>

</div>

<div class="card">

<h2>⚙ Sistema</h2>

<div class="info">
ESP32: 🟢 ONLINE
</div>

<div class="info">
Servidor Web: 🟢 ONLINE
</div>

</div>

</div>

</body>

</html>

)rawliteral";

  return html;
}

// =====================================================
// SERVIDOR
// =====================================================

void handleRoot() {

  server.send(
    200,
    "text/html; charset=utf-8",
    gerarPagina()
  );
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  // GPIOs

  pinMode(REED_PIN, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AUX, OUTPUT);

  digitalWrite(BUZZER_PIN, HIGH);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AUX, LOW);

  // Servo

  servo.attach(SERVO_PIN);

  servo.write(SERVO_TRANCADO);

  armarioTrancado = true;

  // RFID

  SPI.begin(
    RFID_SCK,
    RFID_MISO,
    RFID_MOSI,
    RFID_SS
  );

  rfid.PCD_Init();

  delay(100);

  rfid.PCD_AntennaOn();

  // Porta

  portaFechada =
    (digitalRead(REED_PIN) == LOW);

  if (portaFechada) {

    digitalWrite(LED_VERMELHO, HIGH);

  } else {

    digitalWrite(LED_VERDE, HIGH);
  }

  // WiFi

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.println();
  Serial.println("Conectando ao WiFi...");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado!");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Servidor

  server.on("/", handleRoot);

  server.begin();

  Serial.println("Servidor Web iniciado!");

  registrarEvento("Sistema iniciado");
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  server.handleClient();

  verificarRFID();

  verificarPorta();

  delay(20);
}
