#ifndef SCRIPT_H
#define SCRIPT_H
const char script_js[] PROGMEM = R"rawliteral(
var clientIPs=[];
function addRow(item){
  var t=document.getElementById('map');
  var r=t.insertRow(-1);
  r.innerHTML="<td><input class='s' type='number' min='1' value='"+(item?item.s:1)+"'></td>"+
             "<td><input class='r' type='number' min='0' value='"+(item?item.r:0)+"'></td>"+
             "<td><input class='n' type='number' min='1' value='"+(item?item.n:1)+"'></td>"+
             "<td><input class='t' type='number' min='0' value='"+(item?item.t:0)+"'></td>"+
             "<td class='end'></td>"+
             "<td><button type='button' onclick='delRow(this)' class='icon'><span class='material-icons'>delete</span></button></td>"+
             "<td><button type='button' onclick='showVal(this)'>Show</button></td>";
  updateEnd(r);
  r.querySelector('.n').addEventListener('input',()=>updateEnd(r));
  r.querySelector('.t').addEventListener('input',()=>updateEnd(r));
}
function delRow(btn){
  var r=btn.parentNode.parentNode;
  r.parentNode.removeChild(r);
}
function showVal(btn){
  var r=btn.parentNode.parentNode;
  var tcp=parseInt(r.querySelector('.t').value);
  var len=parseInt(r.querySelector('.n').value);
  fetch('/value?t='+tcp+'&n='+len).then(res=>res.text()).then(v=>alert(v));
}
function updateEnd(row){
  var tcp=parseInt(row.querySelector('.t').value)||0;
  var len=parseInt(row.querySelector('.n').value)||1;
  row.querySelector('.end').textContent=tcp+len-1;
}
function loadCfg(){
  fetch('/config').then(r=>r.json()).then(cfg=>{
    document.getElementById('ip').value=cfg.ip;
    document.getElementById('gw').value=cfg.gw;
    document.getElementById('mask').value=cfg.mask;
    document.getElementById('baud').value=cfg.baud;
    document.getElementById('port').value=cfg.port;
    cfg.items.forEach(addRow);
  });
}
function saveCfg(e){
  e.preventDefault();
  var t=document.getElementById('map');
  var rows=t.querySelectorAll('tr');
  var data=new URLSearchParams();
  data.append('ip',document.getElementById('ip').value);
  data.append('gw',document.getElementById('gw').value);
  data.append('mask',document.getElementById('mask').value);
  data.append('baud',document.getElementById('baud').value);
  data.append('port',document.getElementById('port').value);
  data.append('count',rows.length-1);
  for(var i=1;i<rows.length;i++){
    var r=rows[i];
    data.append('s'+(i-1),r.querySelector('.s').value);
    data.append('r'+(i-1),r.querySelector('.r').value);
    data.append('n'+(i-1),r.querySelector('.n').value);
    data.append('t'+(i-1),r.querySelector('.t').value);
  }
  var ranges=[];
  for(var i=1;i<rows.length;i++){
    var r=rows[i];
    var s=parseInt(r.querySelector('.t').value)||0;
    var l=parseInt(r.querySelector('.n').value)||1;
    ranges.push({s:s,e:s+l-1});
  }
  for(var i=0;i<ranges.length;i++){
    for(var j=i+1;j<ranges.length;j++){
      if(ranges[i].s<=ranges[j].e && ranges[j].s<=ranges[i].e){
        alert('Overlapping TCP ranges');
        return;
      }
    }
  }
  var pass=prompt('Enter password');
  if(pass===null)return;
  data.append('pass',pass);
  fetch('/config',{method:'POST',body:data}).then(r=>{
    if(r.status==401){
      alert('Wrong password');
    }else{
      location.reload();
    }
  });
}
function updateClients(){
  fetch('/clients').then(r=>r.json()).then(data=>{
    document.getElementById('clients').textContent=data.count;
    clientIPs=data.ips;
  });
}
function showClients(){
  if(clientIPs.length==0){
    alert('No clients');
  }else{
    alert(clientIPs.join('\n'));
  }
}
function restartDevice(){
  if(confirm('Restart device?')){
    fetch('/restart',{method:'POST'});
  }
}
document.addEventListener('DOMContentLoaded',()=>{
  document.getElementById('add').addEventListener('click',()=>addRow());
  document.getElementById('cfgForm').addEventListener('submit',saveCfg);
  document.getElementById('showClients').addEventListener('click',showClients);
  document.getElementById('restart').addEventListener('click',restartDevice);
  loadCfg();
  updateClients();
  setInterval(updateClients,1000);
});
)rawliteral";
#endif
