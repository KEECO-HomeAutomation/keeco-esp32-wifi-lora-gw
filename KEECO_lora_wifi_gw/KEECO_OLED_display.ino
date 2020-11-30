displayHandler::displayHandler() {
  buffer_ptr = 0;
}
void displayHandler::addLine(char* text) {
  display_buffer[buffer_ptr] = String(text);
  buffer_ptr++;
  if (buffer_ptr == 6) {
    buffer_ptr = 0;
  }
  displayBuffer();
}
void displayHandler::addLine(String text) {
  display_buffer[buffer_ptr] = text;
  buffer_ptr++;
  if (buffer_ptr == 6) {
    buffer_ptr = 0;
  }
  displayBuffer();
}

void displayHandler::displayBuffer() {
  int ptr = 0;
  Heltec.display->clear();
  
  for (int i = buffer_ptr; i < (6 + buffer_ptr); i++) {
    if (i > 5) {
      ptr = i - 6;
    }
    else {
      ptr = i;
    }
    Heltec.display->drawString(0, ((i-buffer_ptr) * 10), display_buffer[ptr]);
  }
  Heltec.display->display();
}
