#ifndef MQTT_H
#define MQTT_H

class MqttHandler {
  public:
    long mqttLastConnAttempt;
    char status_topic[64];
    char status_text[64];

    MqttHandler(void);
    void setConfigFileHandler(ConfigurationHandler& configH);
    void initMqtt(void);
    void mqttSubCallback(char* topic, byte* payload, unsigned int length);
    boolean mqttReconnect(void);
    void mqttInLoop(void);
    void announceNodeState(void);
    void mqttPublish(char *pub_subtopic, char *mqtt_buffer);
    void mqttPublishIP(void);

  private:
    char temp_topic[128];
    ConfigurationHandler chRef;
};
#endif
