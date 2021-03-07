#define MQTT_CONN_RETRY_WAIT 300000

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

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

MqttHandler::MqttHandler(void) {
  mqttLastConnAttempt = 0;
  strcpy(status_topic, "/state");
  strcpy(status_text, "online");
}

void MqttHandler::setConfigFileHandler(ConfigurationHandler* configH) {
  chRef = configH;
}

void MqttHandler::initMqtt() {
  int retry = 0;
  boolean result = false;
  client.setServer(chRef->mqttServer, 8883);
  client.setCallback(&MqttHandler::mqttSubCallback);
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

void MqttHandler::mqttSubCallback(char* topic, byte* payload, unsigned int length) {
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

boolean MqttHandler::mqttReconnect() {
  if (client.connect(chRef->wifiAP.ssid, chRef->mqttUsername, chRef->mqttPassword)) {
    for (int i = 0; i < chRef->mqttSubTopicCount ; i++ ) {
      mqttSubscribe(chRef->mqttSubTopic[i]);
    }
    mqttPublishIP();
    announceNodeState();
    chRef->statuses.mqttIsConnected = true;
    if (espConfig.statuses.mqttIsConnected) {
      Serial.println("MQTT is Connected / read from ESPconfig");
    }
#ifdef DEBUG
    Serial.println("Connected to MQTT Server");
#endif
  }
  return client.connected();
}

void MqttHandler::mqttInLoop() {
  if (chRef->statuses.wifiIsConnected) {
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
          chRef->statuses.mqttIsConnected = false;
#endif
        }
      }
    }
    else {
      client.loop();
    }
  }
}

void MqttHandler::announceNodeState() {
  mqttPublish(status_topic, status_text);
#ifdef DEBUG
  Serial.println("Device status published on MQTT: ");
  Serial.println(status_text);
#endif
}

void MqttHandler::mqttSubscribe(char *subtopic) {
  strcpy(temp_topic, chRef->deviceUUID);
  strcat(temp_topic, subtopic);
  client.subscribe(temp_topic);
#ifdef DEBUG
  Serial.print("Subscribed to: ");
  Serial.println(temp_topic);
#endif
}


void MqttHandler::mqttPublish(char *pub_subtopic, char *mqtt_buffer) {
  byte bytes[strlen(mqtt_buffer)];
  strcpy(temp_topic, chRef->deviceUUID);
  strcat(temp_topic, pub_subtopic);

  for (unsigned int i = 0; i < strlen(mqtt_buffer); i++) {
    bytes[i] = (byte)mqtt_buffer[i];
  }
  client.publish(temp_topic, bytes, strlen(mqtt_buffer));
}
void MqttHandler::mqttPublishStatus(char *pub_subtopic, byte mqtt_buffer) {
  char value_string[64];
  sprintf(value_string, "{\"LoRa_Value\":\"%d\"}", (int)mqtt_buffer);
  Serial.println(value_string);
  Serial.println(strlen(value_string));
  strcpy(temp_topic, chRef->deviceUUID);
  strcat(temp_topic, pub_subtopic);
  byte bytes[strlen(value_string)];
  for (unsigned int i = 0; i < strlen(value_string); i++) {
    bytes[i] = (byte)value_string[i];
  }
  client.publish(temp_topic, bytes, sizeof(bytes));
}

void MqttHandler::mqttPublishIP() {
  char temp_topic[128] = " ";
  char mqtt_buffer[64] = " ";
  char IP_topic[32];
  strcpy(IP_topic, "/IPaddress");

  toStringIp(WiFi.localIP()).toCharArray(mqtt_buffer, 16);
  strcpy(temp_topic, chRef->deviceUUID);
  strcat(temp_topic, IP_topic);
  client.publish(temp_topic, mqtt_buffer, 16);
}


MqttHandlerEP::MqttHandlerEP(void) {
  mqtt_enabled = false;
#ifdef DEBUG
  Serial.println("MQTT EndPoint Mode");
#endif
}

void MqttHandlerEP::initMqtt() {
  int retry = 0;
  boolean result = false;
  client.setServer(chRef->mqttServer, 8883);
  client.setCallback(&MqttHandlerEP::mqttSubCallback);
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
    Serial.println("MQTT is auto-disabled on this End-Point");
#endif
    mqttLastConnAttempt = millis();
    mqtt_enabled = result;
  }
}


void MqttHandlerEP::announceNodeState() {
  if (mqtt_enabled) {
    mqttPublish(status_topic, status_text);
#ifdef DEBUG
    Serial.println("Device status published on MQTT: ");
    Serial.println(status_text);
#endif
  }
  else {
#ifdef DEBUG
    Serial.println("MQTT is auto-disabled on this End-Point");
#endif
  }
}

void MqttHandlerEP::mqttInLoop() {
  if (mqtt_enabled) {
    if (chRef->statuses.wifiIsConnected) {
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
            chRef->statuses.mqttIsConnected = false;
#endif
          }
        }
      }
      else {
        client.loop();
      }
    }
  }
}


void MqttHandlerEP::mqttPublish(char *pub_subtopic, char *mqtt_buffer) {
  if (mqtt_enabled) {
    byte bytes[strlen(mqtt_buffer)];
    strcpy(temp_topic, chRef->deviceUUID);
    strcat(temp_topic, pub_subtopic);

    for (unsigned int i = 0; i < strlen(mqtt_buffer); i++) {
      bytes[i] = (byte)mqtt_buffer[i];
    }
    client.publish(temp_topic, bytes, strlen(mqtt_buffer));
  }
  else {
#ifdef DEBUG
    Serial.println("MQTT is auto-disabled on this End-Point");
#endif
  }
}
void MqttHandlerEP::mqttPublishStatus(char *pub_subtopic, byte mqtt_buffer) {
  char value_string[64];
  if (mqtt_enabled) {
    sprintf(value_string, "{\"LoRa_Value\":\"%d\"}", (int)mqtt_buffer);
    Serial.println(value_string);
    Serial.println(strlen(value_string));
    strcpy(temp_topic, chRef->deviceUUID);
    strcat(temp_topic, pub_subtopic);
    byte bytes[strlen(value_string)];
    for (unsigned int i = 0; i < strlen(value_string); i++) {
      bytes[i] = (byte)value_string[i];
    }
    client.publish(temp_topic, bytes, sizeof(bytes));
  }
}
