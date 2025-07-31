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
#define MAX_CLIENTS 4
WiFiClient mbClients[MAX_CLIENTS];

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
    String page =
        "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>RS485 Modbus Gateway</title>"
        "<style>body{font-family:Helvetica;background:#2e2e2e;color:#fff;text-align:center;}"
        "table{margin:auto;border-collapse:collapse;}td,th{padding:4px;}"
        "input,button{background:#505050;color:#fff;border:1px solid #ccc;border-radius:4px;}"
        "button{padding:4px 8px;}</style>"
        "<script>function addRow(){var t=document.getElementById('map');"
        "var r=t.insertRow(-1);var i=t.rows.length-2;"
        "r.innerHTML='<td><input name=\'s'+i+'\' type=number min=1 value=1></td>'+"
        "'<td><input name=\'r'+i+'\' type=number min=0 value=0></td>'+"
        "'<td><input name=\'t'+i+'\' type=number min=0 value=0></td>'+"
        "'<td><button type=button onclick=\'delRow(this)\'>-</button></td>';"
        "document.getElementById('count').value=i+1;}"
        "function delRow(b){var r=b.parentNode.parentNode;r.parentNode.removeChild(r);"
        "var t=document.getElementById('map');document.getElementById('count').value=t.rows.length-2;}"
        "</script></head><body>";
    page += "<h2>RS485 Modbus Gateway</h2>";
    page += "<form method='POST' action='/config'>";
    page += "IP <input name='ip' value='" + ETH.localIP().toString() + "'><br>";
    page += "Baud <input name='baud' value='" + String(baudrate) + "'><br>";
    page += "<table id='map'><tr><th>Slave</th><th>Reg</th><th>TCP</th><th></th></tr>";
    for (int i = 0; i < mapCount; i++)
    {
        page += "<tr><td><input name='s" + String(i) + "' type='number' value='" + String(maps[i].slave) + "'></td>";
        page += "<td><input name='r" + String(i) + "' type='number' value='" + String(maps[i].reg) + "'></td>";
        page += "<td><input name='t" + String(i) + "' type='number' value='" + String(maps[i].tcp) + "'></td>";
        page += "<td><button type='button' onclick='delRow(this)'>-</button></td></tr>";
    }
    page += "</table>";
    page += "<input type='hidden' id='count' name='count' value='" + String(mapCount) + "'>";
    page += "<button type='button' onclick='addRow()'>+</button><br>";
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
            }
        }
    }
}

void handleMBTCP(WiFiClient &client)
{
    if (client.available() < 7)
        return;

    uint8_t header[7];
    client.readBytes(header, 7);
    uint16_t transId = (header[0] << 8) | header[1];
    uint16_t length = (header[4] << 8) | header[5];
    uint8_t unitId = header[6];

    while (client.available() < length)
        ;

    uint8_t pdu[256];
    if (length > sizeof(pdu))
    {
        client.flush();
        return;
    }
    client.readBytes(pdu, length);

    uint8_t functionCode = pdu[0];
    uint8_t response[260];
    uint16_t respLen = 0;

    if (functionCode == 3 && length >= 5)
    {
        uint16_t startAddr = (pdu[1] << 8) | pdu[2];
        uint16_t quantity = (pdu[3] << 8) | pdu[4];
        if (startAddr + quantity <= MODBUS_REG_COUNT && quantity <= 125)
        {
            response[0] = transId >> 8;
            response[1] = transId & 0xFF;
            response[2] = 0;
            response[3] = 0;
            response[4] = 0;
            response[5] = quantity * 2 + 3;
            response[6] = unitId;
            response[7] = functionCode;
            response[8] = quantity * 2;
            respLen = 9;
            for (int i = 0; i < quantity; i++)
            {
                uint16_t val = holdingRegs[startAddr + i];
                response[respLen++] = val >> 8;
                response[respLen++] = val & 0xFF;
            }
        }
        else
        {
            response[0] = transId >> 8;
            response[1] = transId & 0xFF;
            response[2] = 0;
            response[3] = 0;
            response[4] = 0;
            response[5] = 3;
            response[6] = unitId;
            response[7] = functionCode | 0x80;
            response[8] = 0x02;
            respLen = 9;
        }
    }
    else
    {
        response[0] = transId >> 8;
        response[1] = transId & 0xFF;
        response[2] = 0;
        response[3] = 0;
        response[4] = 0;
        response[5] = 3;
        response[6] = unitId;
        response[7] = functionCode | 0x80;
        response[8] = 0x01;
        respLen = 9;
    }

    client.write(response, respLen);
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
    mbServer.begin();

    Serial1.begin(baudrate, SERIAL_8N1, RS485_RX, RS485_TX);

    server.on("/", handleRoot);
    server.on("/config", HTTP_POST, handleConfig);
    server.begin();
}

void loop()
{
    if (eth_connected)
    {
        if (mbServer.hasClient())
        {
            WiFiClient newClient = mbServer.available();
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (!mbClients[i] || !mbClients[i].connected())
                {
                    if (mbClients[i]) mbClients[i].stop();
                    mbClients[i] = newClient;
                    break;
                }
            }
            if (newClient && newClient.connected())
            {
                newClient.stop();
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (mbClients[i] && mbClients[i].connected() && mbClients[i].available())
            {
                handleMBTCP(mbClients[i]);
            }
        }

        server.handleClient();
        ArduinoOTA.handle();
    }
    pollRS485();
}

