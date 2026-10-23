/* ===== MEMBER 4: app.js  (UI logic; uses rag.js from Member 3) ===== */
const $ = id => document.getElementById(id);
let S = null, lastDead = [];

const ACTIONS = [
  'Create system', 'Set Available', 'Set Max', 'Set Allocation',
  'Show state', 'Safety check', 'Request resources', 'Release resources',
  'Detect deadlock', 'Recover: terminate', 'Recover: preempt', 'Draw RAG / Reset'
];

const zeros = (r, c) => Array.from({ length: r }, () => Array(c).fill(0));
const need = (i, j) => S.max[i][j] - S.alloc[i][j];
const log = t => { $('log').textContent += t + '\n'; $('log').scrollTop = 1e9; };

function createSystem() {
  const n = +$('n').value, m = +$('m').value;
  S = { n, m, avail: Array(m).fill(0), max: zeros(n, m), alloc: zeros(n, m), req: zeros(n, m), active: Array(n).fill(true) };
  lastDead = []; buildTables(); refresh(); log(`Created system: ${n} processes, ${m} resource types`);
}

function matrixHTML(title, key, rows) {
  let h = `<table><caption>${title}</caption><tr><th></th>${Array.from({ length: S.m }, (_, j) => `<th>R${j}</th>`).join('')}</tr>`;
  for (let i = 0; i < rows; i++) {
    h += `<tr><th>P${i}</th>` + Array.from({ length: S.m }, (_, j) =>
      key === 'avail' ? `<td><input data-k="avail" data-j="${j}" type="number" min="0" value="${S.avail[j]}"></td>`
                      : `<td><input data-k="${key}" data-i="${i}" data-j="${j}" type="number" min="0" value="${S[key][i][j]}"></td>`).join('') + '</tr>';
  }
  return h + '</table>';
}

function buildTables() {
  $('tables').innerHTML =
    `<table><caption>Available</caption><tr>${S.avail.map((_, j) => `<th>R${j}</th>`).join('')}</tr><tr>${S.avail.map((v, j) => `<td><input data-k="avail" data-j="${j}" type="number" min="0" value="${v}"></td>`).join('')}</tr></table>` +
    matrixHTML('Max', 'max', S.n) + matrixHTML('Allocation', 'alloc', S.n) + needHTML();
  document.querySelectorAll('#tables input').forEach(el => el.onchange = readInputs);
}
function needHTML() {
  let h = `<table><caption>Need (Max - Allocation)</caption><tr><th></th>${Array.from({ length: S.m }, (_, j) => `<th>R${j}</th>`).join('')}</tr>`;
  for (let i = 0; i < S.n; i++) h += `<tr><th>P${i}</th>` + Array.from({ length: S.m }, (_, j) => `<td>${need(i, j)}</td>`).join('') + '</tr>';
  return h + '</table>';
}

function readInputs() {
  document.querySelectorAll('#tables input').forEach(el => {
    const v = Math.max(0, +el.value || 0), k = el.dataset.k, j = +el.dataset.j;
    if (k === 'avail') S.avail[j] = v; else S[k][+el.dataset.i][j] = v;
  });
  refresh(false);
}

/* ---- Banker's safety algorithm ---- */
function safety() {
  const work = [...S.avail], fin = S.active.map(a => !a), seq = [];
  let progress = true;
  while (progress) {
    progress = false;
    for (let i = 0; i < S.n; i++) {
      if (fin[i]) continue;
      if (S.alloc[i].every((_, j) => need(i, j) <= work[j])) {
        S.alloc[i].forEach((a, j) => work[j] += a);
        fin[i] = true; seq.push(i); progress = true;
      }
    }
  }
  return fin.every(Boolean) ? seq : null;
}

function request(p, amt) {
  if (amt.some((a, j) => a > need(p, j))) return log(`P${p}: request exceeds its maximum claim`);
  if (amt.some((a, j) => a > S.avail[j])) {
    amt.forEach((a, j) => S.req[p][j] = a);
    log(`P${p}: not enough available, request is now pending (edge added to RAG)`);
  } else {
    amt.forEach((a, j) => { S.avail[j] -= a; S.alloc[p][j] += a; });
    if (safety()) { log(`P${p}: request granted (state stays safe)`); }
    else { amt.forEach((a, j) => { S.avail[j] += a; S.alloc[p][j] -= a; S.req[p][j] = a; }); log(`P${p}: request denied, would be unsafe (made pending)`); }
  }
}

function release(p, amt) {
  if (amt.some((a, j) => a > S.alloc[p][j])) return log(`P${p}: cannot release more than it holds`);
  amt.forEach((a, j) => { S.avail[j] += a; S.alloc[p][j] -= a; });
  log(`P${p}: released resources`);
}

const parseAmt = () => { const a = $('amt').value.trim().split(/\s+/).map(Number); return a.length === S.m && a.every(x => x >= 0) ? a : null; };

function act(k) {
  if (!S && k !== 0) return log('Create a system first.');
  const p = +$('pid').value;
  switch (k) {
    case 0: createSystem(); return;
    case 1: case 2: case 3: log('Edit the table cells directly; values update on change.'); return;
    case 4: log(`Available: [${S.avail}]\nAlloc: ${JSON.stringify(S.alloc)}\nMax: ${JSON.stringify(S.max)}`); break;
    case 5: { const s = safety(); log(s ? 'SAFE. Sequence: ' + s.map(x => 'P' + x).join(' -> ') : 'UNSAFE: no safe sequence'); break; }
    case 6: case 7: { const a = parseAmt(); if (p >= S.n || !a) return log(`Enter a valid process and ${S.m} amounts.`); (k === 6 ? request : release)(p, a); break; }
    case 8: lastDead = detectDeadlock(S); log(lastDead.length ? 'DEADLOCK among: ' + lastDead.map(x => 'P' + x) : 'No deadlock'); break;
    case 9: { const killed = recoverTerminate(S); lastDead = []; log(killed.length ? 'Terminated: ' + killed.map(x => 'P' + x) : 'Nothing to recover'); break; }
    case 10: { if (p >= S.n) return log('Invalid process'); log(recoverPreempt(S, p) ? `Preempted resources of P${p}` : `P${p} is not active`); lastDead = detectDeadlock(S); break; }
    case 11: if (confirm('Reset system? (Cancel just redraws the RAG)')) { createSystem(); return; } break;
  }
  buildTables(); refresh();
}

function refresh(rebuild = true) {
  if (!S) return;
  const dead = detectDeadlock(S), seq = safety();
  lastDead = dead;
  const state = dead.length ? 'deadlocked' : seq ? 'safe' : 'unsafe';
  $('badge').textContent = state.toUpperCase(); $('badge').className = 'badge ' + state;
  $('seq').textContent = 'Safe sequence: ' + (seq && seq.length ? seq.map(x => 'P' + x).join(' -> ') : seq ? '(none needed)' : 'none');
  const total = S.avail.map((a, j) => a + S.alloc.reduce((s, r) => s + r[j], 0));
  const used = S.alloc.flat().reduce((a, b) => a + b, 0), cap = total.reduce((a, b) => a + b, 0);
  const pct = cap ? Math.round(used / cap * 100) : 0;
  $('utilFill').style.width = pct + '%'; $('utilText').textContent = pct + '%';
  drawRAG($('rag'), S, dead);
}

ACTIONS.forEach((name, k) => {
  const b = document.createElement('button'); b.textContent = `${k + 1}. ${name}`; b.onclick = () => act(k); $('menu').appendChild(b);
});
createSystem();
