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
#include <ETH.h>
#include <WebServer.h>
#include <ModbusMaster.h>
#include <ArduinoModbus.h>

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

// Modbus TCP server
WiFiServer mbServer(502);
ModbusTCPServer modbusTCPServer;

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
    String page = "<html><body><h2>RS485 Modbus Gateway</h2>";
    page += "<form method='POST' action='/config'>";
    page += "Baud:<input name='baud' value='" + String(baudrate) + "'><br>";
    page += "IP:<input name='ip' value='" + ETH.localIP().toString() + "'><br>";
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        page += "Slave<input name='s" + String(i) + "' value='" + String(maps[i].slave) + "'>";
        page += " Reg<input name='r" + String(i) + "' value='" + String(maps[i].reg) + "'>";
        page += " TCP<input name='t" + String(i) + "' value='" + String(maps[i].tcp) + "'><br>";
    }
    page += "<input type='submit' value='Save'></form></body></html>";
    server.send(200, "text/html", page);
}

void handleConfig()
{
    if (server.hasArg("baud"))
    {
        baudrate = server.arg("baud").toInt();
        Serial1.begin(baudrate, SERIAL_8N1, RS485_RX, RS485_TX);
    }
    if (server.hasArg("ip"))
    {
        local_IP.fromString(server.arg("ip"));
        ETH.config(local_IP, gateway, subnet, dns, dns);
    }
    mapCount = 0;
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        String sArg = "s" + String(i);
        String rArg = "r" + String(i);
        String tArg = "t" + String(i);
        if (server.hasArg(sArg) && server.hasArg(rArg) && server.hasArg(tArg))
        {
            maps[i].slave = server.arg(sArg).toInt();
            maps[i].reg   = server.arg(rArg).toInt();
            maps[i].tcp   = server.arg(tArg).toInt();
            if (maps[i].tcp < 100)
            {
                mapCount = i + 1;
            }
        }
    }
    server.sendHeader("Location", "/");
    server.send(303);
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
            modbusTCPServer.holdingRegisterWrite(maps[i].tcp, maps[i].value);
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
    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_PHY_LAN8720, ETH_CLOCK_GPIO17_OUT);

    mbServer.begin();
    modbusTCPServer.begin();
    modbusTCPServer.configureHoldingRegisters(0, 100);

    Serial1.begin(baudrate, SERIAL_8N1, RS485_RX, RS485_TX);

    server.on("/", handleRoot);
    server.on("/config", HTTP_POST, handleConfig);
    server.begin();
}

void loop()
{
    if (eth_connected)
    {
        WiFiClient client = mbServer.available();
        if (client)
        {
            modbusTCPServer.accept(client);
        }
        modbusTCPServer.poll();
        server.handleClient();
    }
    pollRS485();
}

