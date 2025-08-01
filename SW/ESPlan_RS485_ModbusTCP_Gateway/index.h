#ifndef INDEX_H
#define INDEX_H
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>RS485 Modbus Gateway</title>
    <link rel='stylesheet' href='/styles.css'>
    <link href='https://fonts.googleapis.com/icon?family=Material+Icons' rel='stylesheet'>
</head>
<body>
    <h2>RS485 Modbus Gateway</h2>
    <form id='cfgForm'>
        <div id='container'>
            <div class='settings'>
                <div class='field'><label>IP</label><input id='ip' name='ip'></div>
                <div class='field'><label>Gateway</label><input id='gw' name='gw'></div>
                <div class='field'><label>Mask</label><input id='mask' name='mask'></div>
                <div class='field'><label>Baud</label><input id='baud' name='baud'></div>
                <div class='field'><label>Port</label><input id='port' name='port'></div>
                <div class='field'><label>Clients</label><div class='clientsWrap'><span id='clients'>0</span><button type='button' id='showClients' class='icon' title='Show clients'><span class='material-icons'>visibility</span></button></div></div>
                <div class='field'><label>Cycle time</label><div class='cycleWrap'><span id='cycle'>0</span> ms</div></div>
            </div>
            <div class='tableWrap'>
                <table id='map'>
                    <tr><th>Slave</th><th>Reg</th><th>Len</th><th>TCP</th><th>End</th><th>Action</th></tr>
                </table>
                <button type='button' id='add'><span class='material-icons'>add</span></button>
            </div>
        </div>
        <input type='submit' id='save' value='Save'>
        <button type='button' id='restart'>Restart</button>
    </form>
    <script src='/script.js'></script>
</body>
</html>
)rawliteral";
#endif
