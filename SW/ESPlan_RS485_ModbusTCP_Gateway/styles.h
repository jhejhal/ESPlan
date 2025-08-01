#ifndef STYLES_H
#define STYLES_H
const char styles_css[] PROGMEM = R"rawliteral(
body{font-family:Helvetica;background:#2e2e2e;color:#fff;text-align:center;margin:0;padding:20px;}
#container{display:flex;justify-content:center;align-items:flex-start;gap:20px;margin-bottom:15px;}
.settings{display:flex;flex-direction:column;text-align:left;border-right:1px solid #555;padding-right:20px;margin-right:20px;min-width:230px;}
.field{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;}
table{margin:auto;border-collapse:collapse;min-width:700px;}
td,th{padding:6px;border:1px solid #555;}
input,button{background:#505050;color:#fff;border:1px solid #888;border-radius:4px;padding:4px 6px;transition:linear 0.1s;}
input{width:110px;}
button,input[type=submit]{padding:4px 10px;cursor:pointer;}
button:not(.icon):hover,input:hover[type=submit]{color:black;background-color:lightgray;transition:linear 0.1s;}
.icon:hover{color:gray;transition:linear 0.1s;}
.icon{background:none;border:none;padding:0;}
.tableWrap{display:flex;flex-direction:row;align-items:flex-end;}
.clientsWrap{display:flex;align-items:center;gap:4px;}
.cycleWrap{display:flex;align-items:center;gap:4px;}
.actionWrap{display:flex;gap:4px;}
#add{margin-left:8px;margin-bottom:1px;height:30px;width:30px;}
#add span{position:relative;left:-6px;font-size:20px;}
#restart,#save{width:110px;}
#restart{margin-left:10px;}
)rawliteral";
#endif
