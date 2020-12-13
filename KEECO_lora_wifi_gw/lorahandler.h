#ifndef lorahandler_H
#define lorahandler_H

class LoraHandler {
  public:
    long timeout_hands;
    long timeout_ping;
    char IO_status_loc;
    char IO_status_rem;
    bool lora_connected;

    LoraHandler(void);

    bool loraPing(void);
    bool loraGetStatus(void);
    bool loraSendStatus(char stat);
    void loraInLoop(void);

  private:
    long last_sent;
    long last_received;
    int msgCount;
    int msgWaitforAck;
    byte localAddress;
    byte destination;
    bool lora_conn_prev;

    bool sendMessage(String outgoing, byte type = 0);
    int onReceive(int packetSize);
    void sendAck(byte msgId);
    String errorParser(int ec);
};

#endif
