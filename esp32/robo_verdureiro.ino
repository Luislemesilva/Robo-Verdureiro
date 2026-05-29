#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ==========================================
// WIFI
// ==========================================

const char* SSID = "";
const char* PASSWORD = "";

// ==========================================
// SERVIDOR IA
// ==========================================

const char* SERVIDOR_URL =
"https://luisleme-robo-verdureiro.hf.space/prever";

// ==========================================
// RELÉ DA BOMBA
// ==========================================

const int PINO_RELE = 23;

// ==========================================
// GUILHOTINA
// ==========================================

const int DIR_GUILHOTINA  = 18;
const int STEP_GUILHOTINA = 26;

// ==========================================
// ESTEIRA
// ==========================================

const int DIR_ESTEIRA  = 13;
const int STEP_ESTEIRA = 27;

// ==========================================
// MODO TESTE
// ==========================================

bool MODO_TESTE = true;

// ==========================================
// CONFIGURAÇÕES
// ==========================================

int passosGuilhotina = 300;
int passosEsteira = 180;

int velocidadeDescida = 3000;
int velocidadeSubida  = 3000;
int velocidadeEsteira = 3000;

int quantidadePlantas = 4;

// ==========================================

struct DadosClima {

  float precipitacao;
  float temperatura;
  float orvalho;
  float umidade;
  float vento;

  bool valido = false;
};

// ==========================================

DadosClima buscarClima();
String consultarModelo(DadosClima c);
void decidirIrrigacao();

// ==========================================

void setup() {

  Serial.begin(115200);

  // ======================================
  // RELÉ
  // ======================================

  pinMode(PINO_RELE, OUTPUT);

  // RELÉ ACTIVE LOW
  // HIGH = desligado
  digitalWrite(PINO_RELE, HIGH);

  // ======================================
  // GUILHOTINA
  // ======================================

  pinMode(DIR_GUILHOTINA, OUTPUT);
  pinMode(STEP_GUILHOTINA, OUTPUT);

  // ======================================
  // ESTEIRA
  // ======================================

  pinMode(DIR_ESTEIRA, OUTPUT);
  pinMode(STEP_ESTEIRA, OUTPUT);

  // ======================================
  // WIFI
  // ======================================

  WiFi.begin(SSID, PASSWORD);

  Serial.print("Conectando WiFi");

  while(WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");

  delay(2000);

  decidirIrrigacao();
}

// ==========================================

void loop() {

  delay(86400000UL);

  decidirIrrigacao();
}

// ==========================================

void decidirIrrigacao() {

  Serial.println("\n======================");
  Serial.println("INICIANDO CICLO");
  Serial.println("======================");

  DadosClima clima = buscarClima();

  String resultado;

  // ======================================
  // MODO TESTE
  // ======================================

  if(MODO_TESTE) {

    resultado = "SIM";

    Serial.println("\n[MODO TESTE ATIVADO]");
  }
  else {

    resultado = consultarModelo(clima);
  }

  Serial.print("\nResultado: ");

  Serial.println(resultado);

  // ======================================
  // EXECUTA CICLO
  // ======================================

  if(resultado == "SIM") {

    for(int planta = 0; planta < quantidadePlantas; planta++) {

      Serial.print("\nPLANTA ");
      Serial.println(planta + 1);

      delay(1000);

      // ==================================
      // GUILHOTINA DESCE
      // ==================================

      Serial.println("Descendo guilhotina...");

      digitalWrite(DIR_GUILHOTINA, HIGH);

      for(int i = 0; i < passosGuilhotina; i++) {

        digitalWrite(STEP_GUILHOTINA, HIGH);
        delayMicroseconds(velocidadeDescida);

        digitalWrite(STEP_GUILHOTINA, LOW);
        delayMicroseconds(velocidadeDescida);
      }

      delay(1000);

      // ==================================
      // BOMBA LIGA
      // ==================================

      Serial.println("Ligando bomba...");

      // ACTIVE LOW
      digitalWrite(PINO_RELE, LOW);

      delay(500);

      // ==================================
      // BOMBA DESLIGA
      // ==================================

      Serial.println("Desligando bomba...");

      digitalWrite(PINO_RELE, HIGH);

      delay(1500);

      // ==================================
      // GUILHOTINA SOBE
      // ==================================

      Serial.println("Subindo guilhotina...");

      digitalWrite(DIR_GUILHOTINA, LOW);

      for(int i = 0; i < passosGuilhotina; i++) {

        digitalWrite(STEP_GUILHOTINA, HIGH);
        delayMicroseconds(velocidadeSubida);

        digitalWrite(STEP_GUILHOTINA, LOW);
        delayMicroseconds(velocidadeSubida);
      }

      delay(2000);

      // ==================================
      // ESTEIRA ANDA
      // ==================================

      Serial.println("Movendo esteira...");

      digitalWrite(DIR_ESTEIRA, HIGH);

      for(int i = 0; i < passosEsteira; i++) {

        digitalWrite(STEP_ESTEIRA, HIGH);
        delayMicroseconds(velocidadeEsteira);

        digitalWrite(STEP_ESTEIRA, LOW);
        delayMicroseconds(velocidadeEsteira);
      }

      delay(2000);
    }

    // ======================================
    // ESTEIRA VOLTA
    // ======================================

    Serial.println("\nVoltando esteira...");

    digitalWrite(DIR_ESTEIRA, LOW);

    int passosVolta =
      passosEsteira * quantidadePlantas;

    for(int i = 0; i < passosVolta; i++) {

      digitalWrite(STEP_ESTEIRA, HIGH);
      delayMicroseconds(velocidadeEsteira);

      digitalWrite(STEP_ESTEIRA, LOW);
      delayMicroseconds(velocidadeEsteira);
    }

    Serial.println("\n>>> CICLO FINALIZADO <<<");
  }

  else {

    Serial.println("\n>>> NÃO IRRIGAR <<<");
  }
}

// ==========================================
// MOCK CLIMA
// ==========================================

DadosClima buscarClima() {

  DadosClima c;

  c.valido = true;

  return c;
}

// ==========================================
// CONSULTA IA
// ==========================================

String consultarModelo(DadosClima c) {

  if(WiFi.status() != WL_CONNECTED) {

    return "ERRO";
  }

  WiFiClientSecure client;

  client.setInsecure();

  HTTPClient http;

  http.begin(client, SERVIDOR_URL);

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  char payload[256];

  snprintf(
    payload,
    sizeof(payload),

    "{\"precipitacao\":%.2f,"
    "\"temperatura\":%.2f,"
    "\"orvalho\":%.2f,"
    "\"umidade\":%.0f,"
    "\"vento\":%.2f}",

    c.precipitacao,
    c.temperatura,
    c.orvalho,
    c.umidade,
    c.vento
  );

  int httpCode =
    http.sendRequest("POST", payload);

  String resposta =
    http.getString();

  if(httpCode != 200) {

    http.end();

    return "ERRO";
  }

  DynamicJsonDocument doc(512);

  DeserializationError error =
    deserializeJson(doc, resposta);

  if(error) {

    http.end();

    return "ERRO";
  }

  http.end();

  return String(
    (const char*)doc["irrigar"]
  );
}
