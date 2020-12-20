#define SCREENSAV 5000

displayHandler::displayHandler() {
  buffer_ptr = 0;
  stat_changed = true;
  visible = true;
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
    Heltec.display->drawString(0, ((i - buffer_ptr) * 10), display_buffer[ptr]);
  }
  Heltec.display->display();
}

//128x64 px display
void displayHandler::displayStatuses(statuses local_stat) {
  Heltec.display->clear();
  Heltec.display->drawString(5, 0, "LoRa");
  Heltec.display->drawString(47, 0, "WiFi");
  Heltec.display->drawString(89, 0, "MQTT");
  boolToString(local_stat.lora_stat);
  Heltec.display->drawString(5, 10, stat_temp);
  boolToString(local_stat.wifi_stat);
  Heltec.display->drawString(47, 10, stat_temp);
  boolToString(local_stat.mqtt_stat);
  Heltec.display->drawString(89, 10, stat_temp);
  Heltec.display->drawLine(0, 24, 127, 24);
  Heltec.display->drawLine(40, 0, 40, 24);
  Heltec.display->drawLine(82, 0, 82, 24);
  Heltec.display->drawLine(0, 24, 127, 24);
  Heltec.display->drawString(0, 26, "IP:" + toStringIp(WiFi.localIP()));
  Heltec.display->drawLine(0, 38, 127, 38);
  Heltec.display->drawString(0, 40, "Statuses");
  Heltec.display->drawString(0, 50, String(local_stat.IO_stat, BIN));
  Heltec.display->display();
  displayedAt = millis();
}

void displayHandler::updateInternalStat() {
  if ((display_stat.mqtt_stat != espConfig.statuses.mqttIsConnected) || (display_stat.wifi_stat != espConfig.statuses.wifiIsConnected) || (display_stat.lora_stat != espConfig.statuses.loraIsConnected)) {
    stat_changed = true;
    display_stat.mqtt_stat = espConfig.statuses.mqttIsConnected;
    display_stat.wifi_stat = espConfig.statuses.wifiIsConnected;
    display_stat.lora_stat = espConfig.statuses.loraIsConnected;
    Serial.println("states changed");
    if (espConfig.statuses.loraIsConnected) {
      digitalWrite(25,HIGH);     //White LED on the board
    }
    else {
      digitalWrite(25,LOW);
    }
  }
}

void displayHandler::displayInLoop() {
  updateInternalStat();
  if (stat_changed) {
    displayStatuses(display_stat);
    stat_changed = false;
    visible = true;
    Serial.println("stat changed if");
  }
  if (visible) {
    if ((millis() - displayedAt) > SCREENSAV) {
      Heltec.display->clear();
      Heltec.display->display();
      visible = false;
      Serial.println("visible if");
    }
  }
}

void displayHandler::boolToString(bool val) {
  if (val) {
    stat_temp = "OK";
  }
  else {
    stat_temp = "NOK";
  }
}
