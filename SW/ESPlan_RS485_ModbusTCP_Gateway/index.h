#ifndef INDEX_H
#define INDEX_H
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>RS485 Modbus Gateway</title>
    <link rel='stylesheet' href='/styles.css'>
</head>
<body>
    <h2>RS485 Modbus Gateway</h2>
    <form id='cfgForm'>
        IP <input id='ip' name='ip'><br>
        Gateway <input id='gw' name='gw'><br>
        Mask <input id='mask' name='mask'><br>
        Baud <input id='baud' name='baud'><br>
        Port <input id='port' name='port'><br>
        <table id='map'>
            <tr><th>Slave</th><th>Reg</th><th>Len</th><th>TCP</th><th>End</th><th></th><th></th></tr>
        </table>
        <button type='button' id='add'>+</button><br>
        <input type='submit' id='save' value='Save'>
    </form>
    <script src='/script.js'></script>
</body>
</html>
)rawliteral";
#endif
