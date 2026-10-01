#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN   21
#define RST_PIN  22

#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==============================");
  Serial.println("       TESTE PROFUNDO RC522");
  Serial.println("==============================");

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);

  rfid.PCD_Init();

  delay(100);

  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);

  Serial.print("VersionReg = 0x");
  Serial.println(version, HEX);

  if (version == 0x00 || version == 0xFF) {
    Serial.println("ERRO: RC522 nao responde.");
    return;
  }

  Serial.println("Comunicacao SPI: OK");

  // Mostra informacoes do RC522
  rfid.PCD_DumpVersionToSerial();

  // Liga a antena
  rfid.PCD_AntennaOn();

  Serial.println();
  Serial.println("Antena RF ativada.");
  Serial.println("Aproxime a tag/cartao.");
  Serial.println();
}

void loop() {

  if (!rfid.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }

  Serial.println("================================");
  Serial.println("TAG DETECTADA!");
  Serial.println("================================");

  if (!rfid.PICC_ReadCardSerial()) {
    Serial.println("Falha ao ler UID.");
    return;
  }

  Serial.print("UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      Serial.print("0");
    }

    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }

  Serial.println();

  Serial.print("UID possui ");
  Serial.print(rfid.uid.size);
  Serial.println(" bytes.");

  Serial.println();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1000);
}
