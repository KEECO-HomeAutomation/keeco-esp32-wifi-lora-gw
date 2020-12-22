#ifndef lorahandler_H
#define lorahandler_H

class LoraHandler {
  public:
    long timeout_hands;
    long timeout_ping;
    char IO_status_loc;
    char IO_status_rem;
    bool lora_connected;
    bool state_changed_loc;
    bool state_changed_rem;

    LoraHandler(void);

    bool loraPing(void);
    bool loraGetRemStatus(void);
    bool loraSendStatus(char stat);
    bool loraSendShowDisplay(void);
    void loraInLoop(void);
    void setDisplayHandler(displayHandler* displayH);
    void setConfigFileHandler(ConfigurationHandler* configH);
    void setLocalState(byte stat);

  protected:
    long last_sent;
    long last_received;
    int msgCount;
    int msgWaitforAck;
    byte localAddress;
    byte destination;
    bool lora_conn_prev;
    displayHandler *dhRef;
    ConfigurationHandler *chRef;

    bool sendMessage(String outgoing, byte type = 0);
    int onReceive(int packetSize);
    void sendAck(byte msgId);
    String errorParser(int ec);
};

class LoraHandlerEP : public LoraHandler {
  public:
    LoraHandlerEP(void);
};

#endif
