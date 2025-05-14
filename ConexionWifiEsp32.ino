
//LIBRERIAS
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <MQTTPubSubClient.h>
#include <PubSubClient.h>

//VARIABLES
int DHTPIN = 17;
int pinrel = 25;
int TEMPERATURA_LIMITE = 24;
int ledR = 26;
int ledV = 27;
float t;
int h;

//CONSTANTES PARA CONECTARNOS A PUNTO DE ACCESO
const char* mqttServer = "10.42.0.1";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

//OBJETOS DE TIPO WIFI Y DHT
DHT dht(DHTPIN, DHT11);
WiFiMulti wifiMulti;
WiFiClient espClient;
PubSubClient client(espClient);

//FUNCION PAUSE, PARA QUE NO SE CUELGUE CON DELAY EL PROGRAMA
void pausa(long milisegundospausa)
{
  unsigned long tiempoInicial = millis();
  unsigned long tiempoPermanecerMQTT = millis();
  while((millis() - tiempoInicial) < milisegundospausa)
  {
    if (tiempoPermanecerMQTT < millis())
    {
      yield();
      client.loop();
      tiempoPermanecerMQTT = millis() + 1000;
      //Serial.println("Enviado, permanecer conectado a broker MQTT");
    }
  }
}


//FUNCION CALLBACK (FUNCION NECESARIA PARA CREAR EN EL MENSAJE EN EL TOPIC)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el tema: ");
  Serial.print(topic);
  Serial.print(". Mensaje: ");
  String messageTemp = "";

  for (size_t i = 0; i < length; i++) {
    messageTemp.concat((char)payload[i]);
  }
  Serial.println(messageTemp);
}

//INICIALIZACION DE MQQTT
void InitMqtt() {
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}


//SETUP
void setup() {

  Serial.begin(9600);
  dht.begin();
  pinMode(pinrel, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledV, OUTPUT);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  //CLAVES PUNTO ACCESO 
  wifiMulti.addAP("losfontanerosiot", "Yungbeef@123");

  //INICIAMOS WIFIMULTI
  wifiMulti.run();
  Serial.println("Conectando a WiFi...");

  //INTENTAMOS CONECTARNOS
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Esperando conexion WiFis");
    delay(1000);
  }

  //MOSTRAMOS CONEXION
  Serial.println("\nConectado a:");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //INICIAMOS MQTT
  InitMqtt();
  //NOS INTENTAMOS CONECTAR A MQTT Y SUS TOPICS
  while (!client.connected()) {
    Serial.println("Conectando a mqtt...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("Conectado");
      client.subscribe("losfontaneros/estadoRadiador");
      client.subscribe("losfontaneros/temperaturaConfort");
      Serial.println("Suscripción a 'losfontaneros/estadoRadiador' exitosa.");

    } else {
      Serial.print("ERROR");
      Serial.print(client.state());
      delay(2000); 
    }
  }
}

void loop() {

  //VARIABLES LOCALES HUMEDAD Y TEMPERATURA
  h = dht.readHumidity();
  t = dht.readTemperature();

  char str[30];

  //Mostramos Humedad y temperatura junto con IP
  Serial.print("Humedad: ");
  Serial.print(h);
  Serial.print("%  Temperatura: ");
  Serial.print(t);
  Serial.print("ºC  ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  sprintf(str, "%2.2f", t);

  //REVISAMOS SI ESTAMOS CONECTADOS, SI LO ESTAMOS NOS SUBSCRIBIMOS
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      //SUSCRIBIMOS A TOPIC
      client.subscribe("losfontaneros/estadoRadiador");
      client.subscribe("losfontaneros/temperaturaConfort");
      Serial.println("Suscripción a 'losfontaneros/estadoRadiador' exitosa.");
      client.setCallback(callback);
    } else {
      Serial.print("ERROR DE CONEXION");
      Serial.print(client.state());
      delay(2000);
    }
  }

  //COMPROBAMOS TEMPERATURA PARA ENCENDER O APAGAR RUIDO
  if (t > TEMPERATURA_LIMITE) {
    digitalWrite(pinrel, HIGH);
    Serial.println("Temperatura alta, apagamos los radiadores.");
    client.publish("losfontaneros/temperatura", str);
    digitalWrite(ledR, HIGH);
    digitalWrite(ledV, LOW);
  } else {
    digitalWrite(pinrel, LOW);
    Serial.println("Temperatura baja, encendemos los radiadores.");
    client.publish("losfontaneros/temperatura", str);
    digitalWrite(ledV, HIGH);
    digitalWrite(ledR, LOW);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, intentando reconectar...");
    wifiMulti.run();
  }
  client.loop();
  pausa(30000);
}
