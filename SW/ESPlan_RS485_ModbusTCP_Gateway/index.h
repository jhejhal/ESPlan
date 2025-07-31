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
        Baud <input id='baud' name='baud'><br>
        Port <input id='port' name='port'><br>
        <table id='map'>
            <tr><th>Slave</th><th>Reg</th><th>TCP</th><th></th><th></th></tr>
        </table>
        <button type='button' id='add'>+</button><br>
        <input type='submit' id='save' value='Save'>
    </form>
    <script src='/script.js'></script>
</body>
</html>
)rawliteral";
#endif
