#ifndef SCRIPT_H
#define SCRIPT_H
const char script_js[] PROGMEM = R"rawliteral(
function addRow(item){
  var t=document.getElementById('map');
  var r=t.insertRow(-1);
  r.innerHTML="<td><input class='s' type='number' min='1' value='"+(item?item.s:1)+"'></td>"+
             "<td><input class='r' type='number' min='0' value='"+(item?item.r:0)+"'></td>"+
             "<td><input class='t' type='number' min='0' value='"+(item?item.t:0)+"'></td>"+
             "<td><button type='button' onclick='delRow(this)'>-</button></td>";
}
function delRow(btn){
  var r=btn.parentNode.parentNode;
  r.parentNode.removeChild(r);
}
function loadCfg(){
  fetch('/config').then(r=>r.json()).then(cfg=>{
    document.getElementById('ip').value=cfg.ip;
    document.getElementById('baud').value=cfg.baud;
    cfg.items.forEach(addRow);
  });
}
function saveCfg(e){
  e.preventDefault();
  var t=document.getElementById('map');
  var rows=t.querySelectorAll('tr');
  var data=new URLSearchParams();
  data.append('ip',document.getElementById('ip').value);
  data.append('baud',document.getElementById('baud').value);
  data.append('count',rows.length-1);
  for(var i=1;i<rows.length;i++){
    var r=rows[i];
    data.append('s'+(i-1),r.querySelector('.s').value);
    data.append('r'+(i-1),r.querySelector('.r').value);
    data.append('t'+(i-1),r.querySelector('.t').value);
  }
  fetch('/config',{method:'POST',body:data}).then(()=>location.reload());
}
document.addEventListener('DOMContentLoaded',()=>{
  document.getElementById('add').addEventListener('click',()=>addRow());
  document.getElementById('cfgForm').addEventListener('submit',saveCfg);
  loadCfg();
});
)rawliteral";
#endif
