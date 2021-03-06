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
const int LED_G = 5;
const int LED_V = 6;
const int DHTPIN = 7;  //Sensor de temperatura/umidade ar
const int LAMPADA = 8; //LAMPADA NO RELE
const int VALVULA = 9; //Válvula solenoide no RELE

//Analógicos
const int SOLO = A3; //Sensor de umidade de solo
const int LDR = A4;  //Sensor de luz

//Variavéis auxliares.
String mensagem;
char leitura;
unsigned long lastMsg = 0;

StaticJsonBuffer<256> JSONbuffer;
JsonObject &JSONencoder = JSONbuffer.createObject();

//Definindo o cliente client e o ethernet
DHT dht(DHTPIN, DHTTYPE);
EthernetClient ethClient;
PubSubClient client(ethClient);

void setup()
{
  //Iniciando o monitor serial
  Serial.begin(9600);

  //Definindo as gpio
  pinMode(LAMPADA, OUTPUT);
  pinMode(VALVULA, OUTPUT);
  pinMode(LED_V, OUTPUT);
  pinMode(LED_G, OUTPUT);

  pinMode(SOLO, INPUT);
  pinMode(LDR, INPUT);

  digitalWrite(LED_V, LOW);
  digitalWrite(LED_G, LOW);
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
  if (now - lastMsg > 10000)
  {
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
  if (mensagem == "lj")
  {
    digitalWrite(LAMPADA, HIGH);
  }
  else if (mensagem == "dj")
  {
    digitalWrite(LAMPADA, LOW);
  }
  else if (mensagem == "a")
  {
    acionarAgua(60);
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
      // Esperar 5s para reconectar
      delay(5000);
    }
  }
}
//Função para ler o sensor de umidade de solo
int lerSolo()
{
  int vlr = analogRead(SOLO);
  vlr = map(vlr, 100, 1023, 100, 0);
  return vlr;
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

//A cada determinado valor de tempo, checa a umidade do solo e se estiver
//abaixo de 60% ou 40% aciona a agua durante 1 segundo.
//Vale ressaltar que a válvula solenoide está como normalmente fechada
//Logo ao mandar um sinal low ela abre o circuito para que passe a energia
void acionarAgua(int vlr)
{
  if (vlr <= 60)
  {
    digitalWrite(VALVULA, LOW);
    delay(1000);
    digitalWrite(VALVULA, HIGH);
  }
  else if (vlr <= 40)
  {
    digitalWrite(VALVULA, LOW);
    delay(2000);
    digitalWrite(VALVULA, HIGH);
  }
}

void enviarDados()
{
  char msg[256];
  int umidade = lerSolo();

  mudarLeds(umidade);
  
  JSONencoder["umi"] = dht.readHumidity();
  JSONencoder["temp"] = dht.readTemperature();
  JSONencoder["luz"] = lerLdr();
  JSONencoder["umi_s"] = umidade;

  //Print para debug
  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);

  if (client.publish("iot/casa", JSONmessageBuffer) == true)
  {
    Serial.println("Sucesso ao enviar dados");
  }
  else
  {
    Serial.println("Erro ao enviar dados");
  }
  //Desativado devido a válvula ter queimado
  //acionarAgua(umidade);
}

void mudarLeds(int vlr){
  if(vlr <= 50)
  {
    digitalWrite(LED_G, LOW);
    for(int i = 0; i < 5; i++) 
    {
      digitalWrite(LED_V, HIGH);
      delay(100);
      digitalWrite(LED_V, LOW);
      delay(100);
    }
  } else {
    digitalWrite(LED_G, HIGH);
  }
}

void limpaMSG()
{
  mensagem = "";
  leitura = '0';
}
