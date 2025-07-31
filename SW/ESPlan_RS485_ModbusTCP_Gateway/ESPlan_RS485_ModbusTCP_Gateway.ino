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
#include <Preferences.h>
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
IPAddress local_IP(10, 10, 70, 114);
IPAddress gateway(10, 10, 101, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress dns(8, 8, 8, 8);

// web server for configuration
WebServer server(80);

#define DEFAULT_TCP_PORT 502
uint16_t tcpPort = DEFAULT_TCP_PORT;
// Modbus TCP server
ModbusIP mb;

#define MODBUS_REG_COUNT 1200
uint16_t holdingRegs[MODBUS_REG_COUNT];

// Modbus RTU master
ModbusMaster modbus;

Preferences prefs;

struct MapItem {
    uint8_t  slave;
    uint16_t reg;
    uint16_t len;
    uint16_t tcp;
};

#define MAX_ITEMS 10
MapItem maps[MAX_ITEMS];
uint8_t mapCount = 0;
uint32_t baudrate = 9600;

void startMbServer(){
    mb = ModbusIP();
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
    String json = "{\"ip\":\"" + ETH.localIP().toString() + "\",\"gw\":\"" + gateway.toString() + "\",\"mask\":\"" + subnet.toString() + "\",\"baud\":" + String(baudrate) + ",\"port\":" + String(tcpPort) + ",\"items\":";
    json += "[";
    for (uint8_t i = 0; i < mapCount; i++)
    {
        json += "{\"s\":" + String(maps[i].slave) + ",\"r\":" + String(maps[i].reg) + ",\"n\":" + String(maps[i].len) + ",\"t\":" + String(maps[i].tcp) + "}";
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
    }
    if (server.hasArg("gw"))
    {
        gateway.fromString(server.arg("gw"));
    }
    if (server.hasArg("mask"))
    {
        subnet.fromString(server.arg("mask"));
    }
    ETH.config(local_IP, gateway, subnet, dns, dns);

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
        String nArg = "n" + String(i);
        String tArg = "t" + String(i);
        if (server.hasArg(sArg) && server.hasArg(rArg) && server.hasArg(nArg) && server.hasArg(tArg))
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
                maps[mapCount].len   = server.arg(nArg).toInt();
                maps[mapCount].tcp   = tcp;
                mapCount++;
            }
        }
    }

    prefs.putUInt("baud", baudrate);
    prefs.putUShort("port", tcpPort);
    prefs.putUChar("ip0", local_IP[0]);
    prefs.putUChar("ip1", local_IP[1]);
    prefs.putUChar("ip2", local_IP[2]);
    prefs.putUChar("ip3", local_IP[3]);
    prefs.putUChar("gw0", gateway[0]);
    prefs.putUChar("gw1", gateway[1]);
    prefs.putUChar("gw2", gateway[2]);
    prefs.putUChar("gw3", gateway[3]);
    prefs.putUChar("ms0", subnet[0]);
    prefs.putUChar("ms1", subnet[1]);
    prefs.putUChar("ms2", subnet[2]);
    prefs.putUChar("ms3", subnet[3]);
    prefs.putUChar("count", mapCount);
    for(uint8_t i=0;i<mapCount;i++){
        char key[6];
        sprintf(key,"s%u",i); prefs.putUChar(key,maps[i].slave);
        sprintf(key,"r%u",i); prefs.putUShort(key,maps[i].reg);
        sprintf(key,"n%u",i); prefs.putUShort(key,maps[i].len);
        sprintf(key,"t%u",i); prefs.putUShort(key,maps[i].tcp);
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleValue()
{
    if (server.hasArg("t"))
    {
        uint16_t t = server.arg("t").toInt();
        uint16_t n = 1;
        if (server.hasArg("n")) n = server.arg("n").toInt();
        String out = "";
        for (uint16_t i = 0; i < n && t + i < MODBUS_REG_COUNT; i++)
        {
            out += String(t + i) + ": " + String(mb.Hreg(t + i)) + "\n";
        }
        server.send(200, "text/plain", out);
        return;
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
        uint8_t result = modbus.readHoldingRegisters(maps[i].reg, maps[i].len);
        if (result == modbus.ku8MBSuccess)
        {
            for(uint16_t j=0;j<maps[i].len;j++){
                uint16_t v = modbus.getResponseBuffer(j);
                if (maps[i].tcp + j < MODBUS_REG_COUNT) {
                    holdingRegs[maps[i].tcp + j] = v;
                    mb.Hreg(maps[i].tcp + j, v);
                }
            }
        }
    }
}


void setup()
{
    Serial.begin(115200);

    prefs.begin("cfg", false);
    baudrate = prefs.getUInt("baud", baudrate);
    tcpPort = prefs.getUShort("port", DEFAULT_TCP_PORT);
    local_IP[0] = prefs.getUChar("ip0", local_IP[0]);
    local_IP[1] = prefs.getUChar("ip1", local_IP[1]);
    local_IP[2] = prefs.getUChar("ip2", local_IP[2]);
    local_IP[3] = prefs.getUChar("ip3", local_IP[3]);
    gateway[0] = prefs.getUChar("gw0", gateway[0]);
    gateway[1] = prefs.getUChar("gw1", gateway[1]);
    gateway[2] = prefs.getUChar("gw2", gateway[2]);
    gateway[3] = prefs.getUChar("gw3", gateway[3]);
    subnet[0]  = prefs.getUChar("ms0", subnet[0]);
    subnet[1]  = prefs.getUChar("ms1", subnet[1]);
    subnet[2]  = prefs.getUChar("ms2", subnet[2]);
    subnet[3]  = prefs.getUChar("ms3", subnet[3]);
    mapCount = prefs.getUChar("count", 0);
    for(uint8_t i=0;i<mapCount && i<MAX_ITEMS;i++){
        char key[6];
        sprintf(key,"s%u",i);
        maps[i].slave = prefs.getUChar(key,1);
        sprintf(key,"r%u",i);
        maps[i].reg = prefs.getUShort(key,0);
        sprintf(key,"n%u",i);
        maps[i].len = prefs.getUShort(key,1);
        sprintf(key,"t%u",i);
        maps[i].tcp = prefs.getUShort(key,0);
    }

    pinMode(ETH_NRST_PIN, OUTPUT);
    digitalWrite(ETH_NRST_PIN, LOW);
    delay(500);
    digitalWrite(ETH_NRST_PIN, HIGH);

    WiFi.onEvent(WiFiEvent);
    // start Ethernet with LAN8720 PHY
    ETH.begin(ETH_PHY_LAN8720, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLOCK_GPIO17_OUT);
    ETH.config(local_IP, gateway, subnet, dns, dns);

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