WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);
#define MQTT_CONN_RETRY_WAIT 300000


long mqttLastConnAttempt = 0;

char status_topic[64] = "/state";
char status_text[64] = "online";

/*
   We use TLS certifications to encrypt the communication.
   WiFiClientSecure defines that the communication happens with TLS
   The fingerprint can be extracted on your computer from the cert file. We only use the fingerprint and not the whole cert not to take too much memory for this.
   Please note that you need to call the .verfiy function to actually check the fingerprint.
   It is important to understand that with this TLS method we achieve two things:
      1.) Integrity   - we make sure we connect to the right server
      2.) Encryption  - the data sent over the connection is encrypted (as defined in the cert file)
   This is a great tutorial to create your own TLS cert http://www.steves-internet-guide.com/mosquitto-tls/
*/

void initMqtt() {
  int retry = 0;
  boolean result = false;
  client.setServer(espConfig.mqttServer, 8883);
  client.setCallback(mqttSubCallback);
  client.setSocketTimeout(15);
  while ((!result) && (retry < 3)) {
#ifdef DEBUG
    Serial.print(retry + 1);
    Serial.println(". try to connect to MQTT broker..");
#endif
    result = mqttReconnect();
    retry++;
  }
  if (result) {
#ifdef DEBUG
    Serial.println("Successfully connected to the MQTT broker");
#endif
  }
  else {
#ifdef DEBUG
    Serial.println("Not connected to the MQTT broker");
#endif
    mqttLastConnAttempt = millis();
  }
}

void mqttSubCallback(char* topic, byte* payload, unsigned int length) {
  unsigned int iter = 0;
  unsigned int topic_length = strlen(topic);
  int subtopic_ptr = -1;
  char sub_topic[64];
  while (iter < topic_length) {
    if (topic[(topic_length - iter)] == '/') {
      subtopic_ptr = topic_length - iter;
      iter = topic_length + 1;
    }
    else {
      iter++;
    }
  }
  for (int i = 0; i < (topic_length - subtopic_ptr); i++) {
    sub_topic[i] = topic[subtopic_ptr + i];
    sub_topic[i + 1] = '\0';
  }
  mqttReceivedCallback(sub_topic, payload, length);
}

boolean mqttReconnect() {
  if (client.connect(espConfig.wifiAP.ssid, espConfig.mqttUsername, espConfig.mqttPassword)) {
    for (int i = 0; i < espConfig.mqttSubTopicCount ; i++ ) {
      mqttSubscribe(espConfig.mqttSubTopic[i]);
    }
    mqttPublishIP();
    announceNodeState();
    espConfig.statuses.mqttIsConnected = true;
#ifdef DEBUG
    Serial.println("Connected to MQTT Server");
#endif
  }
  return client.connected();
}

void mqttInLoop() {
  if (espConfig.statuses.wifiIsConnected) {
    if (!client.connected()) {
      long now = millis();
      if (now - mqttLastConnAttempt > MQTT_CONN_RETRY_WAIT) {
        mqttLastConnAttempt = now;
        if (mqttReconnect()) {
          Serial.println("MQTT Reconnect Successful.");
        }
        else {
#ifdef DEBUG
          Serial.println("Still no connection to MQTT Server");
          Serial.println(client.state());
          espConfig.statuses.mqttIsConnected = false;
#endif
        }
      }
    }
    else {
      client.loop();
    }
  }
}

void announceNodeState() {
  mqttPublish(status_topic, status_text);
#ifdef DEBUG
  Serial.println("Device status published on MQTT: ");
  Serial.println(status_text);
#endif
}

void mqttSubscribe(char *subtopic) {
  char temp_topic[128] = " ";
  strcpy(temp_topic, espConfig.deviceUUID);
  strcat(temp_topic, subtopic);
  client.subscribe(temp_topic);
#ifdef DEBUG
  Serial.print("Subscribed to: ");
  Serial.println(temp_topic);
#endif
}

void mqttPublish(char *pub_subtopic, char *mqtt_buffer) {
  char temp_topic[128] = " ";
  byte bytes[strlen(mqtt_buffer)];
  strcpy(temp_topic, espConfig.deviceUUID);
  strcat(temp_topic, pub_subtopic);

  for (unsigned int i = 0; i < strlen(mqtt_buffer); i++) {
    bytes[i] = (byte)mqtt_buffer[i];
  }
  client.publish(temp_topic, bytes, strlen(mqtt_buffer));
}

void mqttPublishIP() {
  char temp_topic[128] = " ";
  char mqtt_buffer[64] = " ";
  char IP_topic[32];
  strcpy(IP_topic, "/IPaddress");

  toStringIp(WiFi.localIP()).toCharArray(mqtt_buffer, 16);
  strcpy(temp_topic, espConfig.deviceUUID);
  strcat(temp_topic, IP_topic);
  client.publish(temp_topic, mqtt_buffer, 16);
}
