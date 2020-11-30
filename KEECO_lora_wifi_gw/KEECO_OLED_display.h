#ifndef KEECO_OLED_display_H
#define KEECO_OLED_display_H

class displayHandler {
  public:
    int buffer_ptr;
    String display_buffer[6];
    displayHandler(void);
    void addLine(char* text);
    void addLine(String text);
    void displayBuffer(void);
};

#endif
