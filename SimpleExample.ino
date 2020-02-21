 
/* ESP8266 AWS IoT example by Evandro Luis Copercini
   Public Domain - 2017
   But you can pay me a beer if we meet someday :D
   This example needs https://github.com/esp8266/arduino-esp8266fs-plugin
  It connects to AWS IoT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
*/

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
// Update these with values suitable for your network.

char ssid[32];
char password[32];
const char* cfgFile = "cfg.ini";
bool OperationMode = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char* AWS_endpoint = "a3rk99uqf1x61g-ats.iot.us-east-1.amazonaws.com"; //MQTT broker ip
void publish_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

bool loadCfgFile() {
    bool loadCfg = true;
    char c;
    byte len = 0;
    byte buf[128];
    char *str = &ssid[0];
    byte index = 0;
    byte ssid_start_position = 0;
    byte ssid_end_position = 0;
    bool ssid_is_null = true;

  if(!SPIFFS.exists(cfgFile))
  {
    loadCfg = false;
  }
  else
  {
    File file = SPIFFS.open(cfgFile, "r");
    file.read(buf, len);
    if(len > 0)
    {
      while (index < len)
      {
        if(buf[index] == ' ')
        {
          if(ssid_is_null == true)
          {
            /* copy for ssid */
            ssid_start_position = (ssid_start_position == 0 )? index: ssid_start_position;
            ssid_end_position = (ssid_start_position < index )? index : ssid_end_position;
          }
          else
          {
            /* copy for password */
            password_start_position = (password_start_position == 0 )? index: password_start_position;
            password_end_position = (password_start_position < index )? index : password_end_position;
          }
        }

        if(ssid_end_position != 0)
        {
          /* complete copy wifi ssid */
          ssid_is_null = false;
        }
      }

      /* Copy wifi ssid, pass to ram buffer */
      if(ssid_end_position > ssid_start_position && password_end_position > password_start_position)
      { 
          /* copy wifi ssid */
          for(byte idx= 0;idx < (ssid_start_position - ssid_start_position); ++idx)
          {
            ssid[idx] = buf[ssid_start_position + i];
          }
          /* copy password */
          for(byte idx= 0;idx < (password_start_position - password_start_position); ++idx)
          {
            password[idx] = buf[password_start_position + i];
          }
      }
      else
      {
        /* file is error */
        loadCfg = false;
      }
    }
    else
    {
      /* file is empty */
      loadCfg = false;
    }
    file.close();  
  }
  return loadCfg;
}

/** Store WLAN credentials to EEPROM */
void saveCfgFile() {
  File f = SPIFFS.open(cfgFile, "w");
  fprintf(f, "SSID: %s PASS: %s ", ssid, password);
  fclose(f);
}

WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, publish_callback, espClient); //set  MQTT port number to 8883 as per //standard
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {
  // We start by connecting to a WiFi network
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  while(!timeClient.update()){
    timeClient.forceUpdate();
  }
  espClient.setX509Time(timeClient.getEpochTime());
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
      if (client.connect("esp8266_thing")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("esp8266_topic", "hello world");
      // ... and resubscribe
      client.subscribe("esp8266_topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      char buf[256];
      espClient.getLastSSLError(buf,256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void connectAws()
{
  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

  // Load certificate file
  File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
  if (!cert) {
    Serial.println("Failed to open cert file");
  }
  else
    Serial.println("Success to open cert file");

  delay(1000);

  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");

  // Load private key file
  File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  }
  else
    Serial.println("Success to open private cert file");

  delay(1000);

  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");

    // Load CA file
    File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
    if (!ca) {
      Serial.println("Failed to open ca ");
    }
    else
    Serial.println("Success to open ca");

    delay(1000);

    if(espClient.loadCACert(ca))
    Serial.println("ca loaded");
    else
    Serial.println("ca failed");


}
void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  if(true == loadCfgFile())
  {
      setup_wifi();
      delay(1000);
      connectAws();
      Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
      OperationMode = true;
  }
}



void loop() {

if(OperationMode == true)
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("esp8266_topic", msg);
    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
  }
}
else
{
  char* str=ssid;
  byte index = 0;
  if(Serial.available()){
        input = Serial.read();
        Serial.print("You typed: " );
        Serial.println(input);
    }
  if(buffer[index] = 'K' && buffer[index - 1] == 'O')
  {
    
  }
}

}
