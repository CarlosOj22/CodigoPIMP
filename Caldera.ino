//CALDERA
//---------------------------------------
//CODIGO QUE COMPRUEBA EL ESTADO DE LA CALDERA Y ENVIA UN OF O ON;
//TAMBIEN MOSTRARA POR PANTALLA EL ESTADO DE LA CALDERA

//LIBRERIAS

//#include <DHT.h>
//#include <DHT_U.h>*/
#include <WiFi.h>
#include <WiFiMulti.h>
#include <MQTTPubSubClient.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//CONSTANTES PARA PANTALLA LED
const int SCREEN_WIDTH = 128; // OLED display width, in pixels
const int SCREEN_HEIGHT = 64; // OLED display height, in pixels


//Declaracion para que funcione pantalle led, se le pasa como parametros el ancho y alto
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

display.setcursor(10,0);
int pinCalderaEnvio = 5; //Envia señal a la caldera 
int pinCalderaRecibe = 2;//Este recibe estado caldera

//Valores para conexion MQTT
const char* mqttServer = "10.42.0.1";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
//DHT dht(DHTPIN, DHT11);
//Objetos de tipo wifimulti y wificlient y PuBSubClient
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

//FUNCION QUE RECIBE UN MENSAJE EN EL TEMA SUSCRITO
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

//FUNCIONCIO PARA INICIAR MQTT
void InitMqtt() {
  //Se llama a los metodos setServer y setCallback 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void setup() {


  Serial.begin(9600);
  pinMode(pinCalderaEnvio, OUTPUT);
  pinMode(pinCalderaRecibe, INPUT);
  WiFi.disconnect(true);

  WiFi.mode(WIFI_STA);

  //Añadimos punto de acceso iot
  wifiMulti.addAP("losfontanerosiot", "Yungbeef@123");

  wifiMulti.run();

  Serial.println("Conectando a WiFi...");

  //Eseramos que se conecte
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Esperando conexion WiFis");
    delay(1000);
  }

  //DAMO LOS DATOS DE LA CONEXION
  Serial.println("\nConectado a:");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //INICIALIZAMOS PROTOCOLO MQTT
  InitMqtt();
  //BUSCAMOS CONEXION TODO EL RATO
  while (!client.connected()) {
    Serial.println("Conectando a mqtt...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("Conectado");
      //NOS SUSCRIBIMOS A ESTADOCALDERA PARA SUBIR ON O OFF
      client.subscribe("losfontaneros/estadoCaldera");
      Serial.println("Suscripción a 'losfontaneros/estadoCaldera' exitosa.");
    //SI NO SE CONECTA DA ERROR
    } else {
      Serial.print("ERROR");
      Serial.print(client.state());
      delay(2000);
    }
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    while(true);
  }
}


void loop() {


  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
 
 //INTENTAMOS CONECTARNOS A EL PUNTO ACCESO
  while (!client.connected()) {
    Serial.println("Conectando a mqtt...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("Conectado");
      //SUSCRIBIMOS A TOPIC CALDERA
      client.subscribe("losfontaneros/estadoCaldera");
      Serial.println("Suscripción a 'losfontaneros/estadoCaldera' exitosa.");
      client.setCallback(callback);
    } else {
      Serial.print("ERROR");
      Serial.print(client.state());
      delay(2000);
    }
  }

  //ENVIAMOS UN HIGH A CALDERA
  digitalWrite(pinCalderaEnvio, HIGH);

  //SI VVUELVE COMO HIGH, CIRCUITO CERRADO, CALDERA ENCENDIDA
  if (digitalRead(pinCalderaRecibe) == HIGH) {
    Serial.println("Caldera encendida.");
    client.publish("losfontaneros/estadoCaldera", "ON");
    // Display static text
    display.println("Caldera encendida");
    display.display();
  
  //SI NO, CALDERA APAGADA
  } else {
    Serial.println("Caldera apagada.");
    client.publish("losfontaneros/estadoCaldera", "OFF");
     // Display static text
    display.println("Caldera apagada");
    display.display();  
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, intentando reconectar...");
    wifiMulti.run();
  } 
  //USAMOS PAUSE NO DELAY!!!!!!
  //CON DELAY TENIAMOS QUE PULSAR TODO EL RATO HASTA COINCIDER QUE LA MAQUINA NO STVUERA DORMIDA EN EL DELAY Y EL MENSAJE LLEGABA!
  pausa(30000);
}
