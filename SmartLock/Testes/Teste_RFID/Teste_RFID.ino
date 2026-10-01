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
  Serial.println("================================");
  Serial.println("       TESTE DO RC522");
  Serial.println("================================");

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);

  rfid.PCD_Init();

  delay(100);

  Serial.println("RC522 inicializado.");
  Serial.println();

  // Lê o registro de versão do RC522
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);

  Serial.print("VersionReg = 0x");

  if (version < 0x10) {
    Serial.print("0");
  }

  Serial.println(version, HEX);

  Serial.println();

  if (version == 0x00 || version == 0xFF) {

    Serial.println("ERRO: RC522 nao respondeu!");
    Serial.println("Verifique alimentacao e fios.");

  } else {

    Serial.println("RC522 respondeu corretamente!");
    Serial.println("Aproxime uma tag RFID...");

  }
}

void loop() {

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println();
  Serial.println("TAG DETECTADA!");

  Serial.print("UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      Serial.print("0");
    }

    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }

  Serial.println();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(500);
}
