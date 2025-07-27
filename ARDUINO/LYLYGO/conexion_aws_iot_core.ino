#define TINY_GSM_MODEM_SIM7000
#include <TinyGsmClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pines del sensor DS18B20
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Calibración de temperatura (si hay diferencia con sensor de referencia)
float offset = -0.5;

// Sensor capacitivo de humedad
#define PIN_HUMEDAD_SUELO 32
int valorSeco = 2630;
int valorHumedo = 1200;

// Pines UART del SIM7000G
#define MODEM_RST        5
#define MODEM_PWRKEY     4
#define MODEM_POWER_ON   23
#define MODEM_TX         27
#define MODEM_RX         26

// Comunicación serial
#define SerialMon Serial
#define SerialAT  Serial1

// Red móvil (GPRS)
const char apn[] = "internet.itelcel.com";
const char gprsUser[] = "webgprs";
const char gprsPass[] = "webgprs2002";

TinyGsm modem(SerialAT);

void setup() {
  SerialMon.begin(115200);
  delay(10);

  // Iniciar sensor de temperatura
  sensors.begin();

  // UART para SIM7000G
  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);

   // Encender módem
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);

  SerialMon.println("---Iniciando módem----");
  modem.restart();

   // Conectar a la red
  SerialMon.print("----Conectando a red NB-IoT/GPRS----");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println("Error al conectar red móvil");
    while (true);
  }
  SerialMon.println("Conectado a red móvil");

}

void loop() {
  // === Leer temperatura del DS18B20 ===
  sensors.requestTemperatures();
  float tempReal = sensors.getTempCByIndex(0);
  float tempCalibrada = tempReal + offset;

  // === Leer humedad del sensor capacitivo ===
  int valorSensor = analogRead(PIN_HUMEDAD_SUELO);
  int humedad = map(valorSensor, valorSeco, valorHumedo, 0, 100);
  humedad = constrain(humedad, 0, 100);

  // Verificar conexión GPRS
  if (!modem.isGprsConnected()) {
    SerialMon.println("🔄 Reconectando GPRS...");
    modem.gprsConnect(apn, gprsUser, gprsPass);
    delay(2000);
    return;
  }

  // === Crear mensaje SMS ===
  String mensaje = "Temp suelo: " + String(tempCalibrada, 1) + "°C | Humedad suelo: " + String(humedad) + "%";
  SerialMon.println("📤 Enviando SMS:");
  SerialMon.println(mensaje);

  // Enviar SMS
  bool enviado = modem.sendSMS("+527122244922", mensaje);  // Reemplaza con tu número

  if (enviado) {
    SerialMon.println("✅ SMS enviado con éxito");
  } else {
    SerialMon.println("❌ Error al enviar SMS");
  }

  delay(600000);  // Esperar 10 minutos antes de la siguiente lectura
}