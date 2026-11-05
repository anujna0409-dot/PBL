/* Frontend for the C deadlock-simulation API. */
const $ = id => document.getElementById(id);
let S = null;
const ACTIONS = [
  'Create system', 'Edit Available / Max / Allocation', 'Show state',
  'Safety check', 'Request resources', 'Release resources',
  'Detect deadlock', 'Recover: terminate victim', 'Refresh / Reset'
];
const log = message => { $('log').textContent += message + '\n'; $('log').scrollTop = $('log').scrollHeight; };
const zeros = (r,c) => Array.from({length:r},()=>Array(c).fill(0));

async function api(path, body='') {
  const response = await fetch(path, {method: body === '' ? 'GET' : 'POST',
    headers: body === '' ? {} : {'Content-Type':'text/plain'}, body: body || undefined});
  if (!response.ok) throw new Error('Server returned HTTP '+response.status);
  return response.json();
}
function matrixHTML(title,key,rows) {
  let h='<table><caption>'+title+'</caption><tr><th></th>';
  for(let j=0;j<S.m;j++) h+='<th>R'+j+'</th>';
  h+='</tr>';
  for(let i=0;i<rows;i++) {
    h+='<tr><th>P'+i+'</th>';
    for(let j=0;j<S.m;j++) h+='<td><input data-k="'+key+'" data-i="'+i+'" data-j="'+j+'" type="number" min="0" value="'+(key==='avail'?S.available[j]:S[key][i][j])+'"></td>';
    h+='</tr>';
  }
  return h+'</table>';
}
function needHTML() {
  let h='<table><caption>Need (Max − Allocation)</caption><tr><th></th>';
  for(let j=0;j<S.m;j++) h+='<th>R'+j+'</th>';
  h+='</tr>';
  for(let i=0;i<S.n;i++) { h+='<tr><th>P'+i+'</th>'; for(let j=0;j<S.m;j++) h+='<td>'+S.need[i][j]+'</td>'; h+='</tr>'; }
  return h+'</table>';
}
function buildTables() {
  $('tables').innerHTML=matrixHTML('Available','avail',1)+matrixHTML('Maximum Requirement','max',S.n)+
    matrixHTML('Allocation','allocation',S.n)+needHTML();
  $('tables').querySelectorAll('input').forEach(el=>el.addEventListener('change',updateSetup));
}
function readTable() {
  const available=Array(S.m).fill(0), max=zeros(S.n,S.m), allocation=zeros(S.n,S.m);
  $('tables').querySelectorAll('input').forEach(el=>{
    const v=Number(el.value), i=Number(el.dataset.i), j=Number(el.dataset.j), k=el.dataset.k;
    if(!Number.isInteger(v)||v<0) throw new Error('Enter non-negative whole numbers only.');
    if(k==='avail') available[j]=v;
    else if(k==='max') max[i][j]=v;
    else if(k==='allocation') allocation[i][j]=v;
  });
  return {available,max,allocation};
}
async function setupFromTables() {
  const d=readTable();
  for(let i=0;i<S.n;i++) for(let j=0;j<S.m;j++) if(d.allocation[i][j]>d.max[i][j]) throw new Error('Allocation cannot exceed Maximum for P'+i+', R'+j+'.');
  const nums=[S.n,S.m,...d.available,...d.allocation.flat(),...d.max.flat()];
  return api('/api/setup',nums.join(' '));
}
async function updateSetup() {
  try { const data=await setupFromTables(); render(data); log(data.message); }
  catch(e) { log('Input error: '+e.message); }
}
function render(data) {
  if(data.n>0) {
    S={n:data.n,m:data.m,available:data.available,allocation:data.allocation,max:data.max,need:data.need};
    buildTables();
  }
  const state=(data.state||'NOT_SET').toLowerCase();
  $('badge').textContent=(data.state||'NOT_SET'); $('badge').className='badge '+state;
  $('seq').textContent='Safe sequence: '+(data.safeSequence?.length?data.safeSequence.map(p=>'P'+p).join(' → '):data.state==='SAFE'?'(none needed)':'none');
  const pct=Math.round(data.utilization?.overall||0);
  $('utilFill').style.width=pct+'%'; $('utilText').textContent=pct+'%';
  if(typeof drawRAG==='function' && S) {
    const dead=data.deadlocked||[];
    const graphState={n:S.n,m:S.m,avail:data.available,max:data.max,alloc:data.allocation,req:data.requests,
      active:(data.active||[]).map(Boolean)};
    drawRAG($('rag'),graphState,dead);
  }
}
async function run(path,body='') {
  try { const data=await api(path,body); render(data); log(data.message||'Completed'); }
  catch(e) { log('Error: '+e.message+' — start the C server and open this page through http://localhost:8080'); }
}
function parseAmounts() {
  if(!S) throw new Error('Create the system first.');
  const a=$('amt').value.trim().split(/\s+/).map(Number);
  if(a.length!==S.m||a.some(x=>!Number.isInteger(x)||x<0)) throw new Error('Enter exactly '+S.m+' non-negative whole numbers.');
  return a;
}
async function act(k) {
  try {
    if(k===0) {
      const n=Number($('n').value),m=Number($('m').value);
      if(!Number.isInteger(n)||!Number.isInteger(m)||n<1||n>10||m<1||m>10) throw new Error('Processes and resources must be between 1 and 10.');
      S={n,m,available:Array(m).fill(0),max:zeros(n,m),allocation:zeros(n,m),need:zeros(n,m)};
      buildTables(); const d=await setupFromTables(); render(d); log(d.message); return;
    }
    if(!S) throw new Error('Create a system first.');
    if(k===1) { log('Edit table cells; each valid change is saved to the C backend.'); return; }
    if(k===2) return run('/api/status');
    if(k===3) return run('/api/safety');
    if(k===6) return run('/api/detect');
    if(k===7) return run('/api/recover');
    if(k===8) { if(confirm('Reset the system?')) { S=null; $('tables').innerHTML=''; $('badge').textContent='NOT CREATED'; $('badge').className='badge'; $('seq').textContent='Safe sequence: none'; log('System reset.'); } else return run('/api/status'); return; }
    const p=Number($('pid').value),a=parseAmounts();
    if(!Number.isInteger(p)||p<0||p>=S.n) throw new Error('Enter a valid process ID.');
    if(k===4) return run('/api/request',[p,...a].join(' '));
    if(k===5) return run('/api/release',[p,...a].join(' '));
  } catch(e) { log('Input error: '+e.message); }
}
ACTIONS.forEach((name,k)=>{const b=document.createElement('button');b.textContent=(k+1)+'. '+name;b.onclick=()=>act(k);$('menu').appendChild(b);});
run('/api/status');
