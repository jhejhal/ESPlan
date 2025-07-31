#include <Arduino.h>
#include <ArduinoModbus.h>

// RS485 serial pins on ESPlan
#define RS485_RX 36
#define RS485_TX 4

#define MODBUS_ID 1

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

  if (!ModbusRTUServer.begin(MODBUS_ID, Serial1)) {
    Serial.println("Failed to start Modbus RTU Server");
    while (1);
  }

  ModbusRTUServer.configureHoldingRegisters(0x00, 3); // three registers
  randomSeed(millis());
}

void loop() {
  static uint32_t lastUpdate = 0;
  ModbusRTUServer.poll();

  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    for (uint8_t i = 0; i < 3; i++) {
      uint16_t value = random(0, 101); // 0..100
      ModbusRTUServer.holdingRegisterWrite(i, value);
    }
  }
}
