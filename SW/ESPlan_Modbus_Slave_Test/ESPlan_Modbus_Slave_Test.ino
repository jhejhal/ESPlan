#include <Arduino.h>
#include <ModbusRTU.h>

// RS485 serial pins on ESPlan
#define RS485_RX 36
#define RS485_TX 4

#define MODBUS_ID 1

ModbusRTU mb;
#define REG_BASE 0

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

  mb.begin(&Serial1);
  mb.slave(MODBUS_ID);
  for (uint8_t i = 0; i < 3; i++) {
    mb.addHreg(REG_BASE + i, 0);
  }
  randomSeed(millis());
}

void loop() {
  static uint32_t lastUpdate = 0;
  mb.task();

  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    for (uint8_t i = 0; i < 3; i++) {
      uint16_t value = random(0, 101); // 0..100
      mb.Hreg(REG_BASE + i, value);
    }
  }
}
