void stopAPSrvCallback(WiFiEvent_t event, WiFiEventInfo_t info) {
#ifdef DEBUG
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif
  if (espConfig.statuses.softApRunning) {
    stopAPServices();
  }
  espConfig.statuses.wifiIsConnected = true;
}


void startAPSrvCallback(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (espConfig.statuses.wifiIsConnected) {
    if (!(espConfig.statuses.softApRunning)) {
      startAPServices();
      Serial.println("AP Services Started (192.168.4.1)");
    }
    espConfig.statuses.wifiIsConnected = false;
#ifdef DEBUG
    Serial.println("WiFi lost connection");
#endif
  }
}
