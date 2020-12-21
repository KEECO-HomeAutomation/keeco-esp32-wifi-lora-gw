LoraHandler::LoraHandler() {
  last_sent = 0;           // timestamp of the latest received message
  last_received = 0;       // timestamp of the latest sent message

  timeout_hands = 5000;    // timeout for receiving an acknowledge (seconds)
  timeout_ping = 60000;    // timeout for the frequency of ping messages

  IO_status_loc = 0;       // status of the local instance
  IO_status_rem = 0;       // status of the remote instance

  state_changed_loc = false;
  state_changed_rem = false;

  lora_connected = false;  // flag to store the status of the LoRa connection
  lora_conn_prev = false;
  msgCount = 0;            // internal counter to store the message id (0..200)
  localAddress = 0xBB;     // address of this device
  destination = 0xFD;      // destination to send to

  msgWaitforAck = -1;      // id of the message that is awaiting an acknowledge, -1 if none
}

void LoraHandler::setDisplayHandler(displayHandler& displayH) {
  dhRef = displayH;
}

void LoraHandler::setConfigFileHandler(ConfigurationHandler& configH) {
  chRef = configH;
}

bool LoraHandler::sendMessage(String outgoing , byte type) {
  if (msgWaitforAck == -1) {
    LoRa.beginPacket();                   // start packet
    LoRa.write(destination);              // add destination address
    LoRa.write(localAddress);             // add sender address
    LoRa.write(type);                     // msg type (0 = normal, 1 = ping, 2 = ACK)
    LoRa.write(msgCount);                 // add message ID
    LoRa.write(outgoing.length());        // add payload length
    LoRa.print(outgoing);                 // add payload
    LoRa.endPacket();                     // finish packet and send it

#ifdef DEBUG
    Serial.print("LoRa Sending: ");
    Serial.println(outgoing);
#endif

    last_sent = millis();
    msgWaitforAck = msgCount;
    msgCount++;
    if (msgCount > 200) {
      msgCount = 0;
    }
    return true;
  }
  else {
    return false;                         // if waiting for an ACK we do not send new message
  }
}

void LoraHandler::sendAck(byte msgId) {
  String outgoing = "ACK";              // string for the acknowledge, will be discarded
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(2);                        // msg type (0 = normal, 1 = ping, 2 = ACK)
  LoRa.write(msgId);                    // add message ID, msgId is not auto incremented. Equals to the msgId we are acknowledging
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
}

bool LoraHandler::loraPing() {
  return sendMessage("ping", 1);        // msg type (0 = normal, 1 = ping, 2 = ACK)
}

int LoraHandler::onReceive(int packetSize) {
  if (packetSize == 0) return -1;       // Return error code -1: no packet received

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte type = LoRa.read();              // incoming msg type
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // incoming payload
  String command = "";                  // if msg type normal first 2 characters define message command

  lora_connected = true;

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }
#ifdef DEBUG
  Serial.println(incoming);
#endif
  if (incomingLength != incoming.length())
  {
#ifdef DEBUG
    Serial.println("LoRa error: incorrent msg length");
#endif
    return -2;                                   // Return error code -2: received msg length mismatch
  }

  if (recipient != localAddress && recipient != 0xFF) {
#ifdef DEBUG
    Serial.println("LoRa error: incorrent msg address");
#endif
    return -3;                                  // Return error code -3: incorrect address
  }
  last_received = millis();
  if (type == 0) {                              // msg type (0 = normal)
    sendAck(incomingMsgId);
    command = incoming.substring(0, 2);
    if (command.equals("GS")) {                 // GS : Get Status
      loraSendStatus(IO_status_loc);
    }
    if (command.equals("ST")) {                 // ST : Status
      IO_status_rem = (byte)incoming.charAt(2);
      state_changed_rem = true;
    }
  }
  if (type == 1) {                              // msg type (1 = ping)
    sendAck(incomingMsgId);
  }
  if (type == 2) {                              // msg type (2 = ACK)
    if (incomingMsgId == msgWaitforAck) {
      msgWaitforAck = -1;
    }
    else {
#ifdef DEBUG
      Serial.println("LoRa error: wrong msg acknowledged");
#endif
      return -4;                                  // Return error code -4: an other message has been acknowledged
    }
  }
  return 0;                                       // Return code 0: Everything OK
}

bool LoraHandler::loraSendStatus(char stat) {
  String message = "ST" + String(stat);
  return sendMessage(message);
}

bool LoraHandler::loraGetStatus() {
  String message = "GS";
  return sendMessage(message);
}

void LoraHandler::loraInLoop() {
  int error_c = 0;
  onReceive(LoRa.parsePacket());
#ifdef DEBUG
  if (error_c) {
    Serial.println(errorParser(error_c));
  }
#endif
  if (msgWaitforAck != -1) {
    if ((millis() - last_sent) > timeout_hands) {
#ifdef DEBUG
      Serial.print(msgWaitforAck);
      Serial.println(" msgId has not received an ACK");
#endif
      lora_connected = false;
      msgWaitforAck = -1;
    }
  }
  if ((millis() - last_received) > timeout_ping) {
    last_received = millis();
    loraPing();
  }
  if (lora_conn_prev != lora_connected) {
    chRef.statuses.loraIsConnected = lora_connected;  // this is triggering the display to be updated
    lora_conn_prev = lora_connected;
  }
  if (state_changed_rem) {
    state_changed_rem = false;
    mqttSendStatustoHub(IO_status_rem);
    dhRef.updateInternalStat();
  }
}

String LoraHandler::errorParser(int ec) {
  String formatted;
  String error_t;
  switch (ec) {
    case 0:
      error_t = "No error";
      break;
    case -1:
      error_t = "No packet received";
      break;
    case -2:
      error_t = "Received msg length mismatch";
      break;
    case -3:
      error_t = "Incorrect address";
      break;
    case -4:
      error_t = "An other message has been acknowledged";
      break;
    default:
      error_t = "Unknown error code";
      break;
  }
  formatted = "Error Code: " + String(ec) + " | " + error_t;
  return formatted;
}

LoraHandlerEP::LoraHandlerEP() {

  timeout_ping = 65000;    // timeout for the frequency of ping messages

  localAddress = 0xFD;     // address of this device
  destination = 0xBB;      // destination to send to
}
