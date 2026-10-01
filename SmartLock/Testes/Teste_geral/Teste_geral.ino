#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// =====================================================
// PINOS
// =====================================================

// RFID RC522
#define RFID_SS    21
#define RFID_RST   22
#define RFID_SCK   18
#define RFID_MISO  19
#define RFID_MOSI  23

// Servo MG90S
#define SERVO_PIN  13

// Reed Switch
#define REED_PIN   27

// Buzzer
#define BUZZER_PIN 26

// LEDs
#define LED_VERDE    25
#define LED_VERMELHO 33
#define LED_AUX      32

// =====================================================
// OBJETOS
// =====================================================

MFRC522 rfid(RFID_SS, RFID_RST);
Servo servo;

// =====================================================
// POSIÇÕES DO SERVO
// =====================================================

// Vamos usar estas posições apenas para TESTE.
// Depois ajustaremos de acordo com a sua trava.
#define SERVO_TRANCADO   0
#define SERVO_DESTRAVADO 90

// =====================================================
// VARIÁVEIS
// =====================================================

bool portaAnterior = false;

// =====================================================
// FUNÇÕES
// =====================================================

void beep(int quantidade, int duracao = 100) {

  for (int i = 0; i < quantidade; i++) {

    digitalWrite(BUZZER_PIN, HIGH);
    delay(duracao);

    digitalWrite(BUZZER_PIN, LOW);
    delay(duracao);
  }
}


// -----------------------------------------------------
// Teste dos LEDs
// -----------------------------------------------------

void testarLEDs() {

  Serial.println();
  Serial.println("TESTANDO LEDs...");

  digitalWrite(LED_VERDE, HIGH);
  delay(500);
  digitalWrite(LED_VERDE, LOW);

  digitalWrite(LED_VERMELHO, HIGH);
  delay(500);
  digitalWrite(LED_VERMELHO, LOW);

  digitalWrite(LED_AUX, HIGH);
  delay(500);
  digitalWrite(LED_AUX, LOW);

  Serial.println("LEDs: OK");
}


// -----------------------------------------------------
// Teste do buzzer
// -----------------------------------------------------

void testarBuzzer() {

  Serial.println();
  Serial.println("TESTANDO BUZZER...");

  beep(1, 200);

  Serial.println("Buzzer: OK");
}


// -----------------------------------------------------
// Teste do servo
// -----------------------------------------------------

void testarServo() {

  Serial.println();
  Serial.println("TESTANDO SERVO...");

  Serial.println("Servo -> 0 graus");
  servo.write(0);
  delay(1000);

  Serial.println("Servo -> 90 graus");
  servo.write(90);
  delay(1000);

  Serial.println("Servo -> 180 graus");
  servo.write(180);
  delay(1000);

  Serial.println("Servo -> 90 graus");
  servo.write(90);
  delay(1000);

  Serial.println("Servo: OK");
}


// -----------------------------------------------------
// Teste do RFID
// -----------------------------------------------------

void verificarRFID() {

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println();
  Serial.println("================================");
  Serial.println("TAG RFID DETECTADA!");
  Serial.println("================================");

  Serial.print("UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      Serial.print("0");
    }

    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }

  Serial.println();

  Serial.print("Tamanho do UID: ");
  Serial.print(rfid.uid.size);
  Serial.println(" bytes");

  Serial.println("================================");

  beep(1, 100);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(500);
}


// -----------------------------------------------------
// Verificação do Reed
// -----------------------------------------------------

void verificarReed() {

  bool portaFechada = (digitalRead(REED_PIN) == LOW);

  if (portaFechada != portaAnterior) {

    portaAnterior = portaFechada;

    Serial.println();

    if (portaFechada) {

      Serial.println("PORTA: FECHADA");

      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_VERMELHO, HIGH);

    } else {

      Serial.println("PORTA: ABERTA");

      digitalWrite(LED_VERMELHO, LOW);
      digitalWrite(LED_VERDE, HIGH);
    }
  }
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("     ARMARIO INTELIGENTE - TESTE");
  Serial.println("========================================");

  // ---------------------------------------------------
  // Configuração dos pinos
  // ---------------------------------------------------

  pinMode(REED_PIN, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AUX, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AUX, LOW);

  Serial.println();
  Serial.println("[OK] GPIOs configurados");


  // ---------------------------------------------------
  // Inicialização do SPI
  // ---------------------------------------------------

  SPI.begin(
    RFID_SCK,
    RFID_MISO,
    RFID_MOSI,
    RFID_SS
  );

  Serial.println("[OK] SPI iniciado");


  // ---------------------------------------------------
  // Inicialização do RFID
  // ---------------------------------------------------

  rfid.PCD_Init();

  delay(100);

  Serial.println("[OK] RC522 inicializado");


  // ---------------------------------------------------
  // Inicialização do servo
  // ---------------------------------------------------

  servo.attach(SERVO_PIN);

  delay(500);

  servo.write(SERVO_TRANCADO);

  Serial.println("[OK] Servo inicializado");


  // ---------------------------------------------------
  // Testes automáticos
  // ---------------------------------------------------

  testarLEDs();

  testarBuzzer();

  testarServo();


  // ---------------------------------------------------
  // Estado inicial da porta
  // ---------------------------------------------------

  portaAnterior = (digitalRead(REED_PIN) == LOW);

  Serial.println();
  Serial.println("========================================");
  Serial.println("        TESTE CONCLUIDO");
  Serial.println("========================================");

  Serial.println();
  Serial.println("Sistema em modo de monitoramento.");
  Serial.println();

  if (portaAnterior) {
    Serial.println("Porta atualmente: FECHADA");
    digitalWrite(LED_VERMELHO, HIGH);
  } else {
    Serial.println("Porta atualmente: ABERTA");
    digitalWrite(LED_VERDE, HIGH);
  }

  Serial.println();
  Serial.println("Aproxime uma tag RFID...");
  Serial.println("Abra/feche a porta para testar o reed.");
  Serial.println();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // Verifica RFID
  verificarRFID();

  // Verifica reed switch
  verificarReed();

  delay(50);
}
