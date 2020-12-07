#ifndef KEECO_OLED_display_H
#define KEECO_OLED_display_H

class displayHandler {
  public:
    struct statuses 
    {
      bool lora_stat;
      bool wifi_stat;
      bool mqtt_stat;
      String local_IP;
      byte IO_stat; 
    };
    statuses display_stat;
    int buffer_ptr;
    String display_buffer[6];
    String stat_temp;
    bool stat_changed;
    unsigned long displayedAt;
    bool visible;
    displayHandler(void);
    void addLine(char* text);
    void addLine(String text);
    void displayBuffer(void);
    void displayStatuses(statuses local_stat);
    void boolToString(bool val);
    void updateInternalStat(void);
    void displayInLoop(void);
};

#endif
