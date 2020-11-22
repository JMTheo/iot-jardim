#include <ArduinoJson.h>
#include <SPI.h>          //Conexão ethernet <-> uno
#include <Ethernet.h>     //Lib para o modulo ethernet
#include <PubSubClient.h> //Lib para a conexão com o broker
#include "DHT.h"          //Lib para usar o DHT11

// Ips para configuração do Ethernet
byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED};
IPAddress ip(45, 165, 179, 80);
IPAddress server(45, 165, 179, 100);
const char *TOPICO = "iot/casa";

//DHT
#define DHTTYPE DHT11

//Definindo as portas.
const int LED = 8;     //Vai ser o RELE
const int VALVULA = 9; //Válvula solenoide no RELE
const int DHTPIN = 7;        //Sensor de temperatura/umidade ar

//Analógicos
const int HIG = A3;           //Sensor de umidade de solo
const int LDR = A4;           //Sensor de luz

//Variavéis auxliares.
String mensagem;
char leitura;
unsigned long lastMsg = 0;

StaticJsonBuffer<256> JSONbuffer;
JsonObject& JSONencoder = JSONbuffer.createObject();

//Definindo o cliente client e o ethernet
DHT dht(DHTPIN, DHTTYPE);
EthernetClient ethClient;
PubSubClient client(ethClient);

void setup()
{
  //Iniciando o monitor serial
  Serial.begin(9600);

  //Definindo as gpio
  pinMode(LED, OUTPUT);
  pinMode(LDR, INPUT);
  

  //Iniciando o DHT11
  dht.begin();

  //Configurações para iniciar o MQTT
  client.setServer(server, 3000);
  client.setCallback(callback);

  //Iniciando o modulo ethernet
  Ethernet.begin(mac, ip);
  delay(1500);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    enviarDados();
    
  }
}

//Executa quando recebe uma mensagem do broker
void callback(char *topic, byte *payload, unsigned int length)
{
  for (int i = 0; i < length; i++)
  {
    char c = (char)payload[i];
    mensagem += c;
  }
  Serial.print("Tópico ");
  Serial.print(topic);
  Serial.print(" | ");
  Serial.println(mensagem);
  if (mensagem == "ligarLampadaSala")
  {
    digitalWrite(LED, HIGH);
  }
  else if (mensagem == "desligarLampadaSala")
  {
    digitalWrite(LED, LOW);
  }
  limpaMSG();
  Serial.println();
}

//Função para se reconectar ao client MQTT
void reconnect()
{
  // Fica em loop enquanto não conecta
  while (!client.connected())
  {
    Serial.print("Iniciando conexão com o broker...");
    // Tenta realizar conexão
    if (client.connect("eArduino"))
    {
      Serial.println("Conectado");
      // Fazendo o "Subscribe" no broker
      client.subscribe(TOPICO);
    }
    else
    {
      Serial.print("erro, rc=");
      Serial.print(client.state());
      Serial.println("tentando conexão em 5 segundos...");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

int lerLdr()
{
  //Lendo o valor do sensor de luz
  int luz = analogRead(LDR);
  //Definindo o minimo de luz e o máximo
  //variavel a ser mapeada| minimo | max | min % | max %
  luz = map(luz, 0, 900, 100, 0); // Escuro tende a 0
  return luz;
}

void enviarDados()
{
  char msg[256];
  int luz = lerLdr();
  JSONencoder["umi"] = dht.readHumidity();
  JSONencoder["temp"] = dht.readTemperature();;
  JSONencoder["luz"] = lerLdr();
  JSONencoder["umi_s"] = 40.5; //Alterar pela funcao do leitor de umidade de solo
  
  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
 
  if (client.publish("iot/casa", JSONmessageBuffer) == true) {
      Serial.println("Sucesso ao enviar dados");
  } else {
      Serial.println("Erro ao enviar dados");
  }
}

void limpaMSG()
{
  mensagem = "";
  leitura = '0';
}
