
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <time.h>
#include <vector>

// WIFI
const char* WIFI_SSID = "Nome da rede";
const char* WIFI_PASSWORD = "Senha da rede";

// CREDENCIAIS
const char* USER_EMAIL = "usuario@gmail.com";
const char* USER_PASSWORD = "123";
const char* ADMIN_EMAIL = "admin@gmail.com";
const char* ADMIN_PASSWORD = "1234";

// UIDS
const char* USER_TAG_UID = "39 3D DD 06";
const char* ADMIN_TAG_UID = "ED 60 BC 06";

// PINOS
#define RFID_SS       21
#define RFID_RST      22
#define RFID_SCK      18
#define RFID_MISO     19
#define RFID_MOSI     23
#define SERVO_PIN     13
#define REED_PIN      27
#define BUZZER_PIN    26
#define LED_VERDE     25
#define LED_VERMELHO  33
#define LED_AMARELO   32

// CONFIGURACOES
const int SERVO_TRANCADO = 0;
const int SERVO_DESTRAVADO = 90;

const unsigned long TEMPO_RETRAVAMENTO = 1500;
const unsigned long INTERVALO_ALARME = 250;
const unsigned long DURACAO_BIP_TAG = 120;
const unsigned long SESSAO_TIMEOUT_MS = 30UL * 60UL * 1000UL;

const int MAX_SESSOES = 8;
const int MAX_LOGS = 40;

// OBJETOS
MFRC522 rfid(RFID_SS, RFID_RST);
Servo servo;
WebServer server(80);
Preferences prefs;

// ESTADOS
bool portaFechada = false;
bool armarioTrancado = true;
bool alarmeAtivo = false;
bool estadoBuzzerAlarme = false;

String ultimoUID = "Nenhum";
String ultimoEvento = "Sistema iniciado";

unsigned long momentoPortaFechou = 0;
unsigned long ultimaMudancaAlarme = 0;

bool wifiConectadoAnteriormente = false;

// ALERTA DE TAG DESCONHECIDA
bool alertaTagAtivo = false;
int etapaAlertaTag = 0;
unsigned long instanteEtapaAlertaTag = 0;
bool estadoLedAmarelo = false;

// SESSOES
struct Sessao {
  String token;
  String papel;
  unsigned long ultimaAtividade;
};

std::vector<Sessao> sessoes;

// LOGS
struct LogInfo {
  String quando;
  String nivel;
  String evento;
};

std::vector<LogInfo> logs;


// ======================================
// FUNCOES AUXILIARES
// ======================================

String escaparHTML(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}

String escaparJSON(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", "\\n");
  s.replace("\r", "\\r");
  s.replace("\t", "\\t");
  return s;
}

String normalizarUID(String uid) {
  uid.trim();
  uid.toUpperCase();

  while (uid.indexOf("  ") >= 0) {
    uid.replace("  ", " ");
  }

  return uid;
}

String formatarUID(MFRC522::Uid *uid) {
  String resultado = "";

  for (byte i = 0; i < uid->size; i++) {
    if (uid->uidByte[i] < 0x10) {
      resultado += "0";
    }

    resultado += String(uid->uidByte[i], HEX);

    if (i < uid->size - 1) {
      resultado += " ";
    }
  }

  resultado.toUpperCase();
  return resultado;
}

String horaAtual() {
  struct tm timeinfo;

  if (getLocalTime(&timeinfo, 50)) {
    char buffer[24];

    strftime(
      buffer,
      sizeof(buffer),
      "%d/%m/%Y %H:%M:%S",
      &timeinfo
    );

    return String(buffer);
  }

  return "Uptime " + String(millis() / 1000) + "s";
}


// ======================================
// LOGS PERSISTENTES NVS
// ======================================

void registrarEvento(String nivel, String evento) {
  if (nivel != "INFO" &&
      nivel != "AVISO" &&
      nivel != "ERRO") {
    nivel = "INFO";
  }

  ultimoEvento = evento;

  String quando = horaAtual();

  Serial.print("[");
  Serial.print(quando);
  Serial.print("][");
  Serial.print(nivel);
  Serial.print("] ");
  Serial.println(evento);

  LogInfo novo;

  novo.quando = quando;
  novo.nivel = nivel;
  novo.evento = evento;

  logs.push_back(novo);

  while ((int)logs.size() > MAX_LOGS) {
    logs.erase(logs.begin());
  }

  prefs.begin("smartlogs", false);

  prefs.putUInt(
    "count",
    (uint32_t)logs.size()
  );

  for (int i = 0; i < (int)logs.size(); i++) {
    String chave = "l" + String(i);

    String linha =
      logs[i].quando + "|" +
      logs[i].nivel + "|" +
      logs[i].evento;

    prefs.putString(
      chave.c_str(),
      linha
    );
  }

  for (int i = logs.size(); i < MAX_LOGS; i++) {
    String chave = "l" + String(i);
    prefs.remove(chave.c_str());
  }

  prefs.end();
}

void carregarLogs() {
  prefs.begin("smartlogs", true);

  uint32_t count = prefs.getUInt("count", 0);

  if (count > MAX_LOGS) {
    count = MAX_LOGS;
  }

  for (uint32_t i = 0; i < count; i++) {
    String chave = "l" + String(i);

    String linha = prefs.getString(
      chave.c_str(),
      ""
    );

    int primeiro = linha.indexOf('|');

    if (primeiro < 0) continue;

    LogInfo item;

    item.quando = linha.substring(0, primeiro);

    int segundo = linha.indexOf(
      '|',
      primeiro + 1
    );

    if (segundo >= 0) {
      item.nivel = linha.substring(
        primeiro + 1,
        segundo
      );

      item.evento = linha.substring(
        segundo + 1
      );

      if (item.nivel != "INFO" &&
          item.nivel != "AVISO" &&
          item.nivel != "ERRO") {
        item.nivel = "INFO";
      }
    } else {
      // Compatibilidade com registros antigos
      item.nivel = "INFO";
      item.evento = linha.substring(primeiro + 1);
    }

    logs.push_back(item);
  }

  prefs.end();

  if (!logs.empty()) {
    ultimoEvento = logs.back().evento;
  }
}


// ======================================
// BUZZER E LED AMARELO
// ======================================

void beep(int quantidade, int duracao = 100) {
  for (int i = 0; i < quantidade; i++) {
    digitalWrite(BUZZER_PIN, LOW);
    delay(duracao);

    digitalWrite(BUZZER_PIN, HIGH);
    delay(duracao);
  }
}

void pararPiscaDesconhecido() {
  alertaTagAtivo = false;
  etapaAlertaTag = 0;
  estadoLedAmarelo = false;

  digitalWrite(LED_AMARELO, LOW);

  if (!alarmeAtivo) {
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

void iniciarPiscaDesconhecido() {
  if (alarmeAtivo) {
    pararPiscaDesconhecido();
    return;
  }

  alertaTagAtivo = true;
  etapaAlertaTag = 0;
  instanteEtapaAlertaTag = millis();

  estadoLedAmarelo = true;

  digitalWrite(LED_AMARELO, HIGH);
  digitalWrite(BUZZER_PIN, LOW);
}

void atualizarLEDDesconhecido() {
  if (alarmeAtivo) {
    pararPiscaDesconhecido();
    return;
  }

  if (!alertaTagAtivo) return;

  unsigned long agora = millis();

  if (agora - instanteEtapaAlertaTag < DURACAO_BIP_TAG) {
    return;
  }

  instanteEtapaAlertaTag = agora;
  etapaAlertaTag++;

  if (etapaAlertaTag >= 6) {
    pararPiscaDesconhecido();
    return;
  }

  bool ligado = (etapaAlertaTag % 2 == 0);

  estadoLedAmarelo = ligado;

  digitalWrite(
    LED_AMARELO,
    ligado ? HIGH : LOW
  );

  digitalWrite(
    BUZZER_PIN,
    ligado ? LOW : HIGH
  );
}


// ======================================
// ALARME
// ======================================

void iniciarAlarme(String motivo) {
  if (alarmeAtivo) return;

  pararPiscaDesconhecido();

  alarmeAtivo = true;
  estadoBuzzerAlarme = false;
  ultimaMudancaAlarme = 0;

  digitalWrite(BUZZER_PIN, HIGH);

  registrarEvento(
    "AVISO",
    "ALARME ATIVADO: " + motivo
  );
}

void silenciarAlarme(String origem) {
  if (!alarmeAtivo) {
    registrarEvento(
      "AVISO",
      "Pedido de silenciamento sem alarme ativo (" +
      origem + ")"
    );

    return;
  }

  alarmeAtivo = false;
  estadoBuzzerAlarme = false;

  digitalWrite(BUZZER_PIN, HIGH);

  pararPiscaDesconhecido();

  registrarEvento(
    "INFO",
    "Alarme silenciado por " + origem
  );

  // Se a porta estiver fechada e a trava liberada,
  // inicia a contagem para o travamento automatico.
  if (portaFechada && !armarioTrancado) {
    momentoPortaFechou = millis();
  } else {
    momentoPortaFechou = 0;
  }
}

void atualizarAlarme() {
  if (!alarmeAtivo) return;

  unsigned long agora = millis();

  if (agora - ultimaMudancaAlarme >= INTERVALO_ALARME) {
    ultimaMudancaAlarme = agora;

    estadoBuzzerAlarme = !estadoBuzzerAlarme;

    digitalWrite(
      BUZZER_PIN,
      estadoBuzzerAlarme ? LOW : HIGH
    );
  }
}


// ======================================
// CONTROLE DA TRAVA
// ======================================

void destravarArmario(String origem) {
  if (!armarioTrancado) {
    registrarEvento(
      "AVISO",
      "Comando de destravamento ignorado: ja destravado (" +
      origem + ")"
    );

    return;
  }

  servo.write(SERVO_DESTRAVADO);

  armarioTrancado = false;

  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_VERDE, HIGH);

  registrarEvento(
    "INFO",
    "Armario destravado por " + origem
  );

  if (!alarmeAtivo) {
    beep(1, 100);
  }
}

// Liberacao especial quando ocorre abertura forcada.
// Nao silencia o alarme.
void liberarTravaPorViolacao() {
  if (!armarioTrancado) return;

  servo.write(SERVO_DESTRAVADO);

  armarioTrancado = false;

  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_VERDE, HIGH);

  momentoPortaFechou = 0;

  registrarEvento(
    "AVISO",
    "Trava liberada automaticamente devido a abertura forcada"
  );
}

void trancarArmario(String origem) {
  if (armarioTrancado) return;

  if (!portaFechada) {
    registrarEvento(
      "AVISO",
      "Retravamento bloqueado: porta aberta (" +
      origem + ")"
    );

    return;
  }

  if (alarmeAtivo) {
    return;
  }

  servo.write(SERVO_TRANCADO);
  delay(250);

  armarioTrancado = true;

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);

  registrarEvento(
    "INFO",
    "Armario trancado por " + origem
  );

  beep(2, 80);
}


// ======================================
// SENSOR DA PORTA
// ======================================

void verificarPorta() {
  bool novoEstado = (digitalRead(REED_PIN) == LOW);

  if (novoEstado != portaFechada) {
    portaFechada = novoEstado;

    if (portaFechada) {
      momentoPortaFechou = millis();

      registrarEvento(
        "INFO",
        "Porta fechada"
      );

      if (armarioTrancado) {
        digitalWrite(LED_VERDE, LOW);
        digitalWrite(LED_VERMELHO, HIGH);
      }
    } else {
      registrarEvento(
        "INFO",
        "Porta aberta"
      );

      if (armarioTrancado) {
        // Abertura sem destravamento autorizado.
        iniciarAlarme(
          "porta aberta enquanto a trava estava trancada"
        );

        // Libera a trava, mas mantem o alarme ativo.
        liberarTravaPorViolacao();
      } else {
        digitalWrite(LED_VERMELHO, LOW);
        digitalWrite(LED_VERDE, HIGH);
      }

      momentoPortaFechou = 0;
    }
  }

  // Nao permite retravar enquanto o alarme estiver ativo.
  if (portaFechada &&
      !armarioTrancado &&
      !alarmeAtivo &&
      momentoPortaFechou > 0 &&
      millis() - momentoPortaFechou >= TEMPO_RETRAVAMENTO) {

    trancarArmario("retravamento automatico");

    momentoPortaFechou = 0;
  }
}


// ======================================
// RFID
// ======================================

String papelDaTag(String uid) {
  uid = normalizarUID(uid);

  String uidUsuario = normalizarUID(
    String(USER_TAG_UID)
  );

  String uidAdmin = normalizarUID(
    String(ADMIN_TAG_UID)
  );

  if (uidUsuario != "UID_USUARIO_A_DEFINIR" &&
      uid == uidUsuario) {
    return "usuario";
  }

  if (uidAdmin != "UID_ADMIN_A_DEFINIR" &&
      uid == uidAdmin) {
    return "admin";
  }

  return "";
}

void verificarRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = formatarUID(&rfid.uid);

  ultimoUID = uid;

  String papel = papelDaTag(uid);

  registrarEvento(
    papel.length() ? "INFO" : "AVISO",
    "Tag detectada UID=" + uid +
    (papel.length()
      ? " papel=" + papel
      : " NAO AUTORIZADA")
  );

  if (papel == "admin" || papel == "usuario") {
    if (alarmeAtivo) {
      silenciarAlarme(
        "tag " + papel + " (" + uid + ")"
      );
    }

    if (armarioTrancado) {
      destravarArmario(
        "tag " + papel + " (" + uid + ")"
      );
    } else {
      registrarEvento(
        "INFO",
        "Tag autorizada apresentada; armario ja destravado"
      );
    }
  } else {
    registrarEvento(
      "AVISO",
      "Acesso negado para UID=" + uid
    );

    iniciarPiscaDesconhecido();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}


// ======================================
// SESSOES WEB
// ======================================

String gerarToken() {
  char token[17];

  snprintf(
    token,
    sizeof(token),
    "%08lx%08lx",
    (unsigned long)esp_random(),
    (unsigned long)esp_random()
  );

  return String(token);
}

String tokenDoCookie() {
  if (!server.hasHeader("Cookie")) return "";

  String cookie = server.header("Cookie");

  int pos = cookie.indexOf("session=");

  if (pos < 0) return "";

  int inicio = pos + 8;

  int fim = cookie.indexOf(';', inicio);

  if (fim < 0) {
    fim = cookie.length();
  }

  return cookie.substring(inicio, fim);
}

int indiceSessao(String token) {
  if (token.length() == 0) return -1;

  unsigned long agora = millis();

  for (int i = 0; i < (int)sessoes.size(); i++) {
    if (agora - sessoes[i].ultimaAtividade >
        SESSAO_TIMEOUT_MS) {
      sessoes.erase(sessoes.begin() + i);
      i--;
      continue;
    }

    if (sessoes[i].token == token) {
      return i;
    }
  }

  return -1;
}

String papelSessao() {
  int idx = indiceSessao(tokenDoCookie());

  if (idx < 0) return "";

  sessoes[idx].ultimaAtividade = millis();

  return sessoes[idx].papel;
}

bool exigirLogin() {
  if (papelSessao() != "") return true;

  redirecionar("/login");

  return false;
}

bool exigirAdmin() {
  if (!exigirLogin()) return false;

  if (papelSessao() == "admin") {
    return true;
  }

  server.send(
    403,
    "text/html; charset=utf-8",
    "<h1>403 - Acesso restrito</h1>"
    "<p>Esta pagina e exclusiva do administrador.</p>"
    "<a href='/user'>Voltar</a>"
  );

  return false;
}

void criarSessao(String papel) {
  if ((int)sessoes.size() >= MAX_SESSOES) {
    sessoes.erase(sessoes.begin());
  }

  Sessao s;

  s.token = gerarToken();
  s.papel = papel;
  s.ultimaAtividade = millis();

  sessoes.push_back(s);

  server.sendHeader(
    "Set-Cookie",
    "session=" + s.token +
    "; HttpOnly; SameSite=Strict; Path=/"
  );
}

void redirecionar(String destino) {
  server.sendHeader("Location", destino);

  server.send(
    303,
    "text/plain; charset=utf-8",
    ""
  );
}


// ======================================
// LITTLEFS E PAGINAS
// ======================================

String lerArquivo(String caminho) {
  File arquivo = LittleFS.open(caminho, "r");

  if (!arquivo || arquivo.isDirectory()) {
    if (arquivo) arquivo.close();

    return "";
  }

  String conteudo = arquivo.readString();

  arquivo.close();

  return conteudo;
}

String paginaLogin(String erro = "") {
  String html = lerArquivo("/login.html");

  if (html.length() == 0) {
    return "<h1>Erro ao carregar login.html do LittleFS</h1>";
  }

  html.replace(
    "{{ERRO}}",
    escaparHTML(erro)
  );

  return html;
}

String paginaUsuario() {
  String html = lerArquivo("/user.html");

  if (html.length() == 0) {
    return "<h1>Erro ao carregar user.html do LittleFS</h1>";
  }

  html.replace(
    "{{PORTA}}",
    portaFechada ? "Fechada" : "Aberta"
  );

  html.replace(
    "{{PORTA_CLASSE}}",
    portaFechada ? "ok" : "warn"
  );

  html.replace(
    "{{TRAVA}}",
    armarioTrancado ? "Trancada" : "Destravada"
  );

  html.replace(
    "{{TRAVA_CLASSE}}",
    armarioTrancado ? "ok" : "warn"
  );

  html.replace(
    "{{ALARME}}",
    alarmeAtivo ? "ATIVO" : "Inativo"
  );

  html.replace(
    "{{ALARME_CLASSE}}",
    alarmeAtivo ? "warn" : "ok"
  );

  html.replace(
    "{{BOTAO_SILENCIAR}}",
    alarmeAtivo
      ? "<form action='/silence' method='POST'><button class='danger' type='submit'>Silenciar alarme</button></form>"
      : ""
  );

  return html;
}

String montarLogsHTML() {
  String saida = "";

  if (logs.empty()) {
    return "<p class='muted'>Nenhum evento registrado.</p>";
  }

  for (int i = (int)logs.size() - 1; i >= 0; i--) {
    String classeNivel =
      logs[i].nivel == "ERRO"
        ? "erro"
        : (logs[i].nivel == "AVISO" ? "aviso" : "info");

    saida +=
      "<div class='log'><span class='nivel " +
      classeNivel + "'>" +
      escaparHTML(logs[i].nivel) +
      "</span> <b>" +
      escaparHTML(logs[i].quando) +
      "</b> - " +
      escaparHTML(logs[i].evento) +
      "</div>";
  }

  return saida;
}

String paginaAdmin() {
  String html = lerArquivo("/admin.html");

  if (html.length() == 0) {
    return "<h1>Erro ao carregar admin.html do LittleFS</h1>";
  }

  html.replace(
    "{{PORTA}}",
    portaFechada ? "Fechada" : "Aberta"
  );

  html.replace(
    "{{PORTA_CLASSE}}",
    portaFechada ? "ok" : "warn"
  );

  html.replace(
    "{{TRAVA}}",
    armarioTrancado ? "Trancada" : "Destravada"
  );

  html.replace(
    "{{TRAVA_CLASSE}}",
    armarioTrancado ? "ok" : "warn"
  );

  html.replace(
    "{{ALARME}}",
    alarmeAtivo ? "ATIVO" : "Inativo"
  );

  html.replace(
    "{{ALARME_CLASSE}}",
    alarmeAtivo ? "warn" : "ok"
  );

  html.replace(
    "{{BOTAO_SILENCIAR}}",
    alarmeAtivo
      ? "<form action='/silence' method='POST'><button class='danger' type='submit'>Silenciar alarme</button></form>"
      : ""
  );

  html.replace(
    "{{ULTIMO_UID}}",
    escaparHTML(ultimoUID)
  );

  html.replace(
    "{{ULTIMO_EVENTO}}",
    escaparHTML(ultimoEvento)
  );

  html.replace(
    "{{LOGS}}",
    montarLogsHTML()
  );

  html.replace(
    "{{MAX_LOGS}}",
    String(MAX_LOGS)
  );

  html.replace(
    "{{WIFI_STATUS}}",
    WiFi.status() == WL_CONNECTED
      ? "Conectado"
      : "Desconectado"
  );

  html.replace(
    "{{WIFI_STATUS_CLASSE}}",
    WiFi.status() == WL_CONNECTED
      ? "ok"
      : "warn"
  );

  html.replace(
    "{{IP_LOCAL}}",
    WiFi.status() == WL_CONNECTED
      ? WiFi.localIP().toString()
      : "Indisponivel"
  );

  html.replace(
    "{{MAC_ADDRESS}}",
    WiFi.macAddress()
  );

  html.replace(
    "{{HTTP_PORT}}",
    "80"
  );

  return html;
}


// ======================================
// API DE STATUS
// ======================================

void handleApiStatus() {
  if (!exigirLogin()) return;

  String json = "{";

  json += "\"porta\":\"";
  json += portaFechada ? "fechada" : "aberta";
  json += "\",";

  json += "\"trava\":\"";
  json += armarioTrancado ? "trancada" : "destravada";
  json += "\",";

  json += "\"alarme\":";
  json += alarmeAtivo ? "true" : "false";
  json += ",";

  json += "\"wifi\":\"";
  json += WiFi.status() == WL_CONNECTED
    ? "conectado"
    : "desconectado";
  json += "\",";

  json += "\"ip\":\"";
  json += WiFi.status() == WL_CONNECTED
    ? WiFi.localIP().toString()
    : "indisponivel";
  json += "\",";

  json += "\"mac\":\"";
  json += WiFi.macAddress();
  json += "\",";

  json += "\"porta_http\":80,";

  json += "\"ultimo_uid\":\"";
  json += escaparJSON(ultimoUID);
  json += "\",";

  json += "\"ultimo_evento\":\"";
  json += escaparJSON(ultimoEvento);
  json += "\"";

  json += "}";

  server.sendHeader("Cache-Control", "no-store");

  server.send(
    200,
    "application/json; charset=utf-8",
    json
  );
}


// ======================================
// ROTAS HTTP
// ======================================

void handleRoot() {
  String papel = papelSessao();

  if (papel == "admin") {
    redirecionar("/admin");
  } else if (papel == "usuario") {
    redirecionar("/user");
  } else {
    redirecionar("/login");
  }
}

void handleLoginGet() {
  String papel = papelSessao();

  if (papel == "admin") {
    redirecionar("/admin");
    return;
  }

  if (papel == "usuario") {
    redirecionar("/user");
    return;
  }

  server.send(
    200,
    "text/html; charset=utf-8",
    paginaLogin()
  );
}

void handleLoginPost() {
  String email = server.arg("email");
  String senha = server.arg("password");

  String papel = "";

  if (email == ADMIN_EMAIL && senha == ADMIN_PASSWORD) {
    papel = "admin";
  } else if (email == USER_EMAIL && senha == USER_PASSWORD) {
    papel = "usuario";
  }

  if (papel == "") {
    registrarEvento(
      "AVISO",
      "Falha de login para email=" + email
    );

    server.send(
      401,
      "text/html; charset=utf-8",
      paginaLogin("E-mail ou senha incorretos.")
    );

    return;
  }

  criarSessao(papel);

  registrarEvento(
    "INFO",
    "Login web bem-sucedido: " +
    email + " papel=" + papel
  );

  if (papel == "admin") {
    redirecionar("/admin");
  } else {
    redirecionar("/user");
  }
}

void handleUser() {
  String papel = papelSessao();

  if (papel == "admin") {
    redirecionar("/admin");
    return;
  }

  if (papel != "usuario") {
    redirecionar("/login");
    return;
  }

  server.send(
    200,
    "text/html; charset=utf-8",
    paginaUsuario()
  );
}

void handleAdmin() {
  if (!exigirAdmin()) return;

  server.send(
    200,
    "text/html; charset=utf-8",
    paginaAdmin()
  );
}

void handleUnlock() {
  String papel = papelSessao();

  if (papel == "") {
    redirecionar("/login");
    return;
  }

  registrarEvento(
    "INFO",
    "Destravamento remoto solicitado por " + papel
  );

  destravarArmario("web " + papel);

  redirecionar(
    papel == "admin" ? "/admin" : "/user"
  );
}

void handleSilence() {
  String papel = papelSessao();

  if (papel == "") {
    redirecionar("/login");
    return;
  }

  silenciarAlarme("sessao web " + papel);

  redirecionar(
    papel == "admin" ? "/admin" : "/user"
  );
}

void handleLogout() {
  String token = tokenDoCookie();

  int idx = indiceSessao(token);

  if (idx >= 0) {
    registrarEvento(
      "INFO",
      "Logout web: papel=" + sessoes[idx].papel
    );

    sessoes.erase(sessoes.begin() + idx);
  }

  server.sendHeader(
    "Set-Cookie",
    "session=; Max-Age=0; HttpOnly; SameSite=Strict; Path=/"
  );

  redirecionar("/login");
}

void handleNotFound() {
  server.send(
    404,
    "text/plain; charset=utf-8",
    "Pagina nao encontrada."
  );
}


// ======================================
// WIFI E NTP
// ======================================

void configurarHorarioNTP() {
  configTime(
    -3 * 3600,
    0,
    "pool.ntp.org",
    "time.nist.gov"
  );

  registrarEvento(
    "INFO",
    "Sincronizacao de horario NTP solicitada"
  );
}

void atualizarConexaoWiFi() {
  bool conectadoAgora =
    (WiFi.status() == WL_CONNECTED);

  if (wifiConectadoAnteriormente && !conectadoAgora) {
    registrarEvento(
      "AVISO",
      "Conexao Wi-Fi perdida"
    );

    wifiConectadoAnteriormente = false;
  }

  if (!wifiConectadoAnteriormente && conectadoAgora) {
    wifiConectadoAnteriormente = true;

    registrarEvento(
      "INFO",
      "Conexao Wi-Fi restabelecida; IP=" +
      WiFi.localIP().toString()
    );

    configurarHorarioNTP();
  }

  static unsigned long ultimaTentativaWiFi = 0;

  if (!conectadoAgora &&
      millis() - ultimaTentativaWiFi >= 10000) {

    ultimaTentativaWiFi = millis();

    WiFi.begin(
      WIFI_SSID,
      WIFI_PASSWORD
    );
  }
}


// ======================================
// SETUP
// ======================================

void setup() {
  Serial.begin(115200);

  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);

  digitalWrite(BUZZER_PIN, HIGH);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, LOW);

  bool littleFsOk = LittleFS.begin(false);

  if (!littleFsOk) {
    Serial.println("ERRO: falha ao montar LittleFS.");
  } else {
    Serial.println("LittleFS montado com sucesso.");
  }

  carregarLogs();

  if (!littleFsOk) {
    registrarEvento(
      "ERRO",
      "Falha ao montar LittleFS; paginas web podem nao funcionar"
    );
  }

  registrarEvento(
    "INFO",
    "Inicializando SmartLocker"
  );

  registrarEvento("ERRO", "Teste de registro de erro");

  // SERVO
  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, 500, 2400);

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

  delay(50);

  rfid.PCD_AntennaOn();

  // PORTA
  portaFechada = (digitalRead(REED_PIN) == LOW);

  if (portaFechada) {
    digitalWrite(LED_VERMELHO, HIGH);

    registrarEvento(
      "INFO",
      "Inicializacao: porta detectada fechada"
    );
  } else {
    digitalWrite(LED_VERDE, HIGH);

    registrarEvento(
      "AVISO",
      "Inicializacao: porta detectada aberta"
    );

    iniciarAlarme(
      "porta encontrada aberta durante inicializacao"
    );

    liberarTravaPorViolacao();
  }

  // WIFI
  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print("Conectando ao Wi-Fi");

  unsigned long inicioWiFi = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - inicioWiFi < 20000) {

    delay(300);

    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConectadoAnteriormente = true;

    Serial.print("IP do SmartLocker: ");
    Serial.println(WiFi.localIP());

    registrarEvento(
      "INFO",
      "Wi-Fi conectado no inicio; IP=" +
      WiFi.localIP().toString()
    );

    configurarHorarioNTP();
  } else {
    wifiConectadoAnteriormente = false;

    registrarEvento(
      "AVISO",
      "Wi-Fi indisponivel durante a inicializacao"
    );
  }

  // ROTAS
  const char* headerKeys[] = {"Cookie"};

  server.collectHeaders(headerKeys, 1);

  server.serveStatic(
    "/style.css",
    LittleFS,
    "/style.css"
  );

  server.on("/", HTTP_GET, handleRoot);

  server.on("/login", HTTP_GET, handleLoginGet);
  server.on("/login", HTTP_POST, handleLoginPost);

  server.on("/user", HTTP_GET, handleUser);
  server.on("/admin", HTTP_GET, handleAdmin);

  server.on("/unlock", HTTP_POST, handleUnlock);
  server.on("/silence", HTTP_POST, handleSilence);
  server.on("/logout", HTTP_POST, handleLogout);

  // NOVA API
  server.on("/api/status", HTTP_GET, handleApiStatus);

  server.onNotFound(handleNotFound);

  server.begin();

  registrarEvento(
    "INFO",
    "Servidor HTTP iniciado na porta 80"
  );
}


// ======================================
// LOOP
// ======================================

void loop() {
  server.handleClient();

  verificarRFID();
  verificarPorta();

  atualizarLEDDesconhecido();
  atualizarAlarme();

  atualizarConexaoWiFi();

  delay(10);
}
