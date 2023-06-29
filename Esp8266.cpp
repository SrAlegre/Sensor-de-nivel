/*
  sensor de nivel usando o esp8266
  Nome : Yuri Martins Alegre

  OBS: As versões das lib's estão desatualizado devido a falta de compatibilidade delas trabalhando em conjunto.
  Abaixo estão comentadas as versões de cada lib que foi encontrada problema, as que não possuiem versão ao lado é correspondente a mais recente

  Instruções de instações:
  
   1) Para instalar a lib do esp8266 é posivel atraves do caminho File->Preferences . Cola o link abaixo em Additional Boards manager URLs.

     http://arduino.esp8266.com/stable/package_esp8266com_index.json
   
   2) Agora para instalar a Lib do Arduino Json e PubSubClient, vamos acessar Tools->LiberyManger, pesquise por ArduinoJson instale, em seguida busque por PubSubClient(Versão publicada por Nick o'Leary)

*/


//Versão da lib do esp8622 : 2.6



#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>  // ver. 6.14.1
#include <Wire.h>
#include <LiquidCrystal_I2C.h>



#define VERSION 1


// WiFi
const char *ssid = "LabTeC - Profs";  // Nome do SSID (Nome da rede Wi-Fi, Lembre de usar caracter por caracter, incluindo os espaços)
const char *password = "";    // Senha da rede citado

// MQTT Broker
const char *mqtt_broker = "0.tcp.sa.ngrok.io"; // o endereçõ do broker, nesse caso como estamos hospedando no Ngrok , temos esse caminho

String _topic = "sensor/" + WiFi.macAddress() + "/out";  // o comando WiFi.macAddress() nos da o MAC do esp, que esta sendo usado como identificação do mesmo, String_topic sera nosso topico criado no broker


const char *topic = _topic.c_str(); 
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 18305; //A porta para o esp acessar para conectar ao broker


WiFiClient espClient;
PubSubClient client(espClient);




// DEFINIÇÕES
#define endereco 0x27  // Endereços comuns: 0x27, 0x3F
#define colunas 20
#define linhas 4
float analogPin = A0;
float val = 0, metros = 0;


LiquidCrystal_I2C lcd(endereco, colunas, linhas);



void setup() {

  lcd.init();       // INICIA A COMUNICAÇÃO COM O DISPLAY
  lcd.backlight();  // LIGA A ILUMINAÇÃO DO DISPLAY

  
  Serial.begin(115200);

  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

//Acessar o broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public emqx mqtt broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}

void loop() {
  client.loop();

  char buffer[256];
  float Dados = LeituraSensor(); //recebemos o dado lido pelo sensor
  char dado_string[8];
  dtostrf(Dados, 5, 2, dado_string);

  StaticJsonDocument<96> doc; //Abrimos um Arquivo estilo json para colocar os dados

  String Mac = String(WiFi.macAddress());

  
  doc["version"] = VERSION;
  doc["level"] = Dados;
  
  serializeJson(doc, buffer); //fizemos uma json com os dados carregados
  Serial.println();



  client.publish(topic, buffer); // publicamos o json no broker
  client.subscribe(topic); 
  delay(5000);
}


float LeituraSensor() {


  val = analogRead(analogPin);
  val = (float)val * 5 / 1023;

  lcd.clear();  // LIMPA O DISPLAY
  lcd.print("- Tensao : ");
  lcd.print(val);
  lcd.print(" V");
  lcd.setCursor(0, 1);  // POSICIONA O CURSOR NA PRIMEIRA COLUNA DA LINHA 2
  Serial.println("Tensao:");
  Serial.println(val);
  lcd.setCursor(0, 3);
  //lcd.print("atualizando 3s");
  lcd.print("- Metros:");
  metros = 0.0119*val - 0.2449; //Esse é o algoritmo que fizemos baseado nos dados que obtemos de forma "analogica", anotando os valores das tensões medidadas conforme aumentamos a profundidada do sensor, então essa função não é puramente linear
  
  lcd.print(metros);


  delay(3000);
  return metros;
}
