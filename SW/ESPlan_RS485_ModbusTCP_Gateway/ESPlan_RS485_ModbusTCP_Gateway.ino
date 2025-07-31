/* RS485 to Modbus TCP gateway example for LaskaKit ESPlan
 *
 * The gateway continuously reads selected Modbus RTU registers over the RS485
 * interface and provides their values through a Modbus TCP server. All
 * parameters can be configured via a simple web interface running on the
 * built‑in Ethernet port.
 *
 * Configure:
 *  - RS485 communication parameters
 *  - list of slave devices and registers to poll
 *  - mapping of every polled register to a Modbus TCP register address
 *
 * Multiple Modbus TCP clients can connect to the gateway simultaneously. The
 * collected values are stored inside the gateway and served from memory – RS485
 * traffic is generated only by the internal polling task.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <ModbusMaster.h>
#include <Modbus.h>
#include <ModbusIP_ESP8266.h>
#include "index.h"
#include "script.h"
#include "styles.h"

// RS485 serial pins on ESPlan
#define RS485_RX 36
#define RS485_TX 4

// LAN8720A parameters
#define ETH_POWER_PIN -1
#define ETH_ADDR 0
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18
#define ETH_NRST_PIN 5

// network defaults
IPAddress local_IP(192, 168, 0, 98);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

// web server for configuration
WebServer server(80);

#define DEFAULT_TCP_PORT 502
uint16_t tcpPort = DEFAULT_TCP_PORT;
// Modbus TCP server
ModbusIP mb;

#define MODBUS_REG_COUNT 100
uint16_t holdingRegs[MODBUS_REG_COUNT];

// Modbus RTU master
ModbusMaster modbus;

struct MapItem {
    uint8_t  slave;
    uint16_t reg;
    uint16_t tcp;
    uint16_t value;
};

#define MAX_ITEMS 10
MapItem maps[MAX_ITEMS];
uint8_t mapCount = 0;
uint32_t baudrate = 9600;

void startMbServer(){
    mb.server(tcpPort);
    for(uint16_t i=0;i<MODBUS_REG_COUNT;i++){
        mb.addHreg(i);
        mb.Hreg(i, holdingRegs[i]);
    }
}
static bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_ETH_START:
        ETH.setHostname("esplan");
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        eth_connected = true;
        Serial.print("ETH IP: ");
        Serial.println(ETH.localIP());
        startMbServer();
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
        eth_connected = false;
        break;
    default:
        break;
    }
}

void handleRoot()
{
    server.send_P(200, "text/html", index_html);
}

void handleConfigGet()
{
    String json = "{\"ip\":\"" + ETH.localIP().toString() + "\",\"baud\":" + String(baudrate) + ",\"port\":" + String(tcpPort) + ",\"items\":";
    json += "[";
    for (uint8_t i = 0; i < mapCount; i++)
    {
        json += "{\"s\":" + String(maps[i].slave) + ",\"r\":" + String(maps[i].reg) + ",\"t\":" + String(maps[i].tcp) + "}";
        if (i < mapCount - 1)
            json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

void handleConfigPost()
{
    if (server.hasArg("baud"))
    {
        baudrate = server.arg("baud").toInt();
        Serial1.begin(baudrate, SERIAL_8N1, RS485_RX, RS485_TX);
    }
    if (server.hasArg("port"))
    {
        uint16_t newPort = server.arg("port").toInt();
        if (newPort != 0 && newPort != tcpPort)
        {
            tcpPort = newPort;
            startMbServer();
        }
    }
    if (server.hasArg("ip"))
    {
        local_IP.fromString(server.arg("ip"));
        ETH.config(local_IP, gateway, subnet, dns, dns);
    }

    uint8_t count = 0;
    if (server.hasArg("count"))
    {
        count = server.arg("count").toInt();
    }
    mapCount = 0;
    for (uint8_t i = 0; i < count && i < MAX_ITEMS; i++)
    {
        String sArg = "s" + String(i);
        String rArg = "r" + String(i);
        String tArg = "t" + String(i);
        if (server.hasArg(sArg) && server.hasArg(rArg) && server.hasArg(tArg))
        {
            uint16_t tcp = server.arg(tArg).toInt();
            bool dup = false;
            for (uint8_t j = 0; j < mapCount; j++)
            {
                if (maps[j].tcp == tcp)
                {
                    dup = true;
                    break;
                }
            }
            if (!dup && tcp < MODBUS_REG_COUNT)
            {
                maps[mapCount].slave = server.arg(sArg).toInt();
                maps[mapCount].reg   = server.arg(rArg).toInt();
                maps[mapCount].tcp   = tcp;
                mapCount++;
            }
        }
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleValue()
{
    if (server.hasArg("t"))
    {
        uint16_t t = server.arg("t").toInt();
        if (t < MODBUS_REG_COUNT)
        {
            server.send(200, "text/plain", String(mb.Hreg(t)));
            return;
        }
    }
    server.send(404, "text/plain", "");
}

void pollRS485()
{
    static uint32_t last = 0;
    if (millis() - last < 100)
        return;
    last = millis();

    for (int i = 0; i < mapCount; i++)
    {
        modbus.begin(maps[i].slave, Serial1);
        uint8_t result = modbus.readHoldingRegisters(maps[i].reg, 1);
        if (result == modbus.ku8MBSuccess)
        {
            maps[i].value = modbus.getResponseBuffer(0);
            if (maps[i].tcp < MODBUS_REG_COUNT) {
                holdingRegs[maps[i].tcp] = maps[i].value;
                mb.Hreg(maps[i].tcp, maps[i].value);
            }
        }
    }
}


void setup()
{
    Serial.begin(115200);

    pinMode(ETH_NRST_PIN, OUTPUT);
    digitalWrite(ETH_NRST_PIN, LOW);
    delay(500);
    digitalWrite(ETH_NRST_PIN, HIGH);

    WiFi.onEvent(WiFiEvent);
    ETH.config(local_IP, gateway, subnet, dns, dns);
    // start Ethernet with LAN8720 PHY
    ETH.begin(ETH_PHY_LAN8720, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLOCK_GPIO17_OUT);

    ArduinoOTA.setHostname("esplan");
    ArduinoOTA.begin();

    Serial1.begin(baudrate, SERIAL_8N1, RS485_RX, RS485_TX);

    server.on("/", HTTP_GET, handleRoot);
    server.on("/script.js", HTTP_GET, [](){ server.send_P(200, "application/javascript", script_js); });
    server.on("/styles.css", HTTP_GET, [](){ server.send_P(200, "text/css", styles_css); });
    server.on("/config", HTTP_GET, handleConfigGet);
    server.on("/config", HTTP_POST, handleConfigPost);
    server.on("/value", HTTP_GET, handleValue);
    server.begin();
}

void loop()
{
    if (eth_connected)
    {
        mb.task();
        server.handleClient();
        ArduinoOTA.handle();
    }
    pollRS485();
}

