#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <time.h>
//------------------ 
#include <PubSubClient.h>
//------------------ 
#include <NTPClient.h>
#include <WiFiUdp.h>
//------------------ 
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
 
#define SDA_PIN D7
#define RST_PIN D6
#define SCK_PIN  D4
#define MOSI_PIN  D3 
#define MISO_PIN  D1
MFRC522 mfrc522(SDA_PIN, RST_PIN);   // Create MFRC522 instance.

int digitalVal;
bool executed = false;
const char* ssid = "Drifada";
const char* password = "sdfsdfdssdfs"; // fdssdfsdfds
const int led = D5;
byte willQoS = 0;
//------------------ 
const char *mqtt_broker = "broker.mqttdashboard.com"; // "broker.emqx.io"; // teste ;
const char *topic = "arduino/LED";
const char *topic1 = "arduino/HORA";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
boolean wifiConnected = false;
// ------------------ 
const long utcOffsetInSeconds = -10800; // -3*60*60 (Brasil)
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
//------------------ 
WiFiClient espClient;
PubSubClient client(espClient);
//------------------ 
ESP8266WebServer server(80);
//------------------ 

  
void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266! (blinking LED)");
  int led_state = 0;
  for (int ii=0;ii<10;ii++){
    digitalWrite(led,led_state);
    led_state = !led_state;
    delay(100);
  }
}

String GetDataHora()
{ 
  String formattedDate;
  formattedDate = timeClient.getFormattedTime();
  return formattedDate;
  }

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

char* toCharArray(String str) {
  return &str[0];
}

String GetHora(){
  String tmp;
  tmp=GetDataHora();
  //client.publish(topic1,toCharArray(tmp));
  return tmp;
}

void led_on(){
 
  server.send(200, "text/plain", "LED on");
  client.publish(topic,toCharArray("{Date:"+GetHora()+",LED:on}"));
  digitalWrite(led,HIGH);
}

void led_off(){
  String tmp;
  tmp="{time:"+GetDataHora()+ ",Led:off}";
  server.send(200, "text/plain", "LED off");  
  client.publish(topic,toCharArray("{Date:"+GetHora()+",LED:off}"));
  digitalWrite(led,LOW);
}

void setup(void){
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  // instancia leitor do rfid
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522
  // ---------------------------------
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  if (wifiConnected)
  {
    timeClient.begin();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  }
  server.on("/", handleRoot); // what happens in the root
  server.on("/on",led_on); // turn led on from webpage /on
  server.on("/off", led_off); // turn led off from webpage /off

  server.begin();
  Serial.println("HTTP server started");
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
  // publish and subscribe
  client.publish(topic, " ");
  client.publish(topic, "hello emqx");
  client.subscribe(topic);
  client.subscribe(topic1);
}

void verifica_rfid()
{
   if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    Serial.println("Erro na conexao do Leitor de Cartao PICC_IsNewCardPresent");
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    Serial.println("Erro na conexao do Leitor de Cartao PICC_ReadCardSerial");
    return;
  }
  Serial.print("UID da tag :");
  String conteudo= "";
  byte letra;
    for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  conteudo.toUpperCase();
  Serial.println();
  Serial.print("Mensagem : "+conteudo);
  }
  
void loop(void){
  server.handleClient();
   timeClient.update();
  client.loop();
 // verifica_rfid();
 if ( mfrc522.PICC_IsNewCardPresent()) {    
    if ( mfrc522.PICC_ReadCardSerial()) {
      Serial.println("Great!.. each time a card is read and re-read this text will print *ONCE ONLY*");
      executed = true;
    }    
    else  
    {     Serial.println(" erro na leitura*"); }
  } 
  else {  
    if (executed) {
      Serial.println("Oh dear... this seems to keep printing... ");  
      executed = false;
    }
  }
}




// connect to wifi  returns true if successful or false if not
boolean connectWifi()
{
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}