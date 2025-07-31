#ifndef STYLES_H
#define STYLES_H
const char styles_css[] PROGMEM = R"rawliteral(
body{font-family:Helvetica;background:#2e2e2e;color:#fff;text-align:center;margin:0;padding:20px;}
#container{display:flex;justify-content:center;align-items:flex-start;gap:20px;}
.settings{display:flex;flex-direction:column;text-align:left;border-right:1px solid #555;padding-right:20px;margin-right:20px;}
.field{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;}
table{margin:auto;border-collapse:collapse;}
td,th{padding:6px;border:1px solid #555;}
input,button{background:#505050;color:#fff;border:1px solid #888;border-radius:4px;padding:4px 6px;}
input{width:110px;}
button{padding:4px 10px;cursor:pointer;}
.icon{background:none;border:none;padding:0;}
.tableWrap{display:flex;flex-direction:column;align-items:center;}
#add{margin-top:8px;}
)rawliteral";
#endif
