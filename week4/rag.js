/* ===== MEMBER 3: rag.js  (RAG drawing + detection/recovery in JS) =====
   State shape: {n,m,avail[],max[][],alloc[][],req[][],active[]}            */

function detectDeadlock(S) {
  const work = [...S.avail], fin = [], dead = [];
  for (let i = 0; i < S.n; i++) fin[i] = !S.active[i] || S.alloc[i].every(x => x === 0);
  let progress = true;
  while (progress) {
    progress = false;
    for (let i = 0; i < S.n; i++) {
      if (fin[i]) continue;
      if (S.req[i].every((r, j) => r <= work[j])) {
        S.alloc[i].forEach((a, j) => work[j] += a);
        fin[i] = true; progress = true;
      }
    }
  }
  for (let i = 0; i < S.n; i++) if (!fin[i]) dead.push(i);
  return dead;                       // list of deadlocked process ids
}

function releaseAll(S, p) {
  for (let j = 0; j < S.m; j++) { S.avail[j] += S.alloc[p][j]; S.alloc[p][j] = 0; S.req[p][j] = 0; }
}

function pickVictim(S, dead) {
  let best = dead[0], bestHeld = -1;
  for (const p of dead) { const h = S.alloc[p].reduce((a, b) => a + b, 0); if (h > bestHeld) { bestHeld = h; best = p; } }
  return best;
}

function recoverTerminate(S) {
  const killed = [];
  let dead;
  while ((dead = detectDeadlock(S)).length) {
    const v = pickVictim(S, dead);
    releaseAll(S, v); S.active[v] = false; killed.push(v);
  }
  return killed;
}

function recoverPreempt(S, p) {      // takes everything from p; p stays active
  if (!S.active[p]) return false;
  releaseAll(S, p); return true;
}

function drawRAG(canvas, S, dead = []) {
  const ctx = canvas.getContext('2d'), W = canvas.width, H = canvas.height;
  ctx.clearRect(0, 0, W, H);
  const pos = {};
  const place = (count, x) => Array.from({ length: count }, (_, i) => ({ x, y: (H / (count + 1)) * (i + 1) }));
  place(S.n, W * 0.25).forEach((p, i) => pos['P' + i] = p);
  place(S.m, W * 0.75).forEach((p, j) => pos['R' + j] = p);

  const arrow = (a, b, color, dashed, label) => {
    const ang = Math.atan2(b.y - a.y, b.x - a.x), off = 24;
    const x1 = a.x + Math.cos(ang) * off, y1 = a.y + Math.sin(ang) * off;
    const x2 = b.x - Math.cos(ang) * off, y2 = b.y - Math.sin(ang) * off;
    ctx.strokeStyle = ctx.fillStyle = color; ctx.lineWidth = 2;
    ctx.setLineDash(dashed ? [6, 4] : []);
    ctx.beginPath(); ctx.moveTo(x1, y1); ctx.lineTo(x2, y2); ctx.stroke();
    ctx.setLineDash([]);
    ctx.beginPath(); ctx.moveTo(x2, y2);
    ctx.lineTo(x2 - 10 * Math.cos(ang - 0.4), y2 - 10 * Math.sin(ang - 0.4));
    ctx.lineTo(x2 - 10 * Math.cos(ang + 0.4), y2 - 10 * Math.sin(ang + 0.4));
    ctx.closePath(); ctx.fill();
    if (label > 1) { ctx.font = '12px sans-serif'; ctx.fillText('x' + label, (x1 + x2) / 2 + 4, (y1 + y2) / 2 - 4); }
  };

  for (let i = 0; i < S.n; i++) for (let j = 0; j < S.m; j++) {
    if (S.alloc[i][j]) arrow(pos['R' + j], pos['P' + i], '#1f7a4d', false, S.alloc[i][j]);
    if (S.req[i][j])   arrow(pos['P' + i], pos['R' + j], '#c0392b', true, S.req[i][j]);
  }
  const text = (t, p) => { ctx.fillStyle = '#111'; ctx.font = 'bold 13px sans-serif'; ctx.textAlign = 'center'; ctx.textBaseline = 'middle'; ctx.fillText(t, p.x, p.y); };
  for (let i = 0; i < S.n; i++) {
    const p = pos['P' + i];
    ctx.beginPath(); ctx.arc(p.x, p.y, 20, 0, 7);
    ctx.fillStyle = !S.active[i] ? '#ddd' : '#fff'; ctx.fill();
    ctx.lineWidth = 3; ctx.strokeStyle = dead.includes(i) ? '#c0392b' : '#333'; ctx.stroke();
    text('P' + i, p);
  }
  for (let j = 0; j < S.m; j++) {
    const p = pos['R' + j];
    ctx.fillStyle = '#fff'; ctx.fillRect(p.x - 20, p.y - 20, 40, 40);
    ctx.lineWidth = 3; ctx.strokeStyle = '#333'; ctx.strokeRect(p.x - 20, p.y - 20, 40, 40);
    text('R' + j + ' (' + S.avail[j] + ')', p);
  }
}
