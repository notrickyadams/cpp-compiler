// ============================================================
//  cpp-compiler visualizer — frontend
//
//  Plain JS, no framework, no build step: this is a "small GUI"
//  for a teaching/portfolio compiler, not a production app. One
//  fetch() per Compile click, one render function per panel.
// ============================================================

const sourceInput = document.getElementById("sourceInput");
const compileBtn  = document.getElementById("compileBtn");
const statusLine  = document.getElementById("statusLine");

// ── Tabs ─────────────────────────────────────────────────────
document.querySelectorAll(".tab").forEach(tab => {
  tab.addEventListener("click", () => {
    document.querySelectorAll(".tab").forEach(t => t.classList.remove("active"));
    document.querySelectorAll(".panel").forEach(p => p.classList.remove("active"));
    tab.classList.add("active");
    document.getElementById("panel-" + tab.dataset.tab).classList.add("active");
  });
});

function activateTab(name) {
  document.querySelector(`.tab[data-tab="${name}"]`)?.click();
}

// ── Compile ──────────────────────────────────────────────────
compileBtn.addEventListener("click", compile);

async function compile() {
  compileBtn.disabled = true;
  statusLine.textContent = "Compiling...";
  statusLine.className = "status";

  try {
    const res = await fetch("/compile", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ source: sourceInput.value }),
    });
    const data = await res.json();

    if (data.error) {
      statusLine.textContent = "error: " + data.error;
      statusLine.className = "status error";
      return;
    }

    renderTokens(data.tokens);
    renderAst(data.ast);
    renderSemantic(data.semanticLog);
    renderPre("irBefore", data.irBefore);
    renderPre("irAfter", data.irAfter);
    renderOptimization(data.optimizationReports);
    renderPre("assembly", data.assembly);
    const errorCount = renderDiagnostics(data.diagnosticsReport);

    if (errorCount > 0) {
      statusLine.textContent = `${errorCount} error(s)`;
      statusLine.className = "status error";
      activateTab("diagnostics");
    } else {
      statusLine.textContent = "Compiled OK";
      statusLine.className = "status ok";
    }
  } catch (err) {
    statusLine.textContent = "request failed: " + err.message;
    statusLine.className = "status error";
  } finally {
    compileBtn.disabled = false;
  }
}

// ── Panel renderers ──────────────────────────────────────────

function renderPre(panelId, text) {
  const panel = document.getElementById("panel-" + panelId);
  panel.innerHTML = "";
  const pre = document.createElement("pre");
  pre.textContent = text && text.length ? text : "(empty — this stage did not run)";
  panel.appendChild(pre);
}

function renderTokens(tokens) {
  const panel = document.getElementById("panel-tokens");
  panel.innerHTML = "";
  if (!tokens || tokens.length === 0) {
    panel.innerHTML = '<p class="placeholder">No tokens.</p>';
    return;
  }
  const list = document.createElement("div");
  list.className = "token-list";
  for (const tok of tokens) {
    const chip = document.createElement("span");
    chip.className = "token-chip";
    const lexeme = tok.lexeme ? `(${tok.lexeme})` : "";
    chip.innerHTML = `<span class="t-type">${escapeHtml(tok.type)}</span>${escapeHtml(lexeme)}` +
                      `<span class="t-pos">${tok.line}:${tok.column}</span>`;
    list.appendChild(chip);
  }
  panel.appendChild(list);
}

function renderSemantic(log) {
  const panel = document.getElementById("panel-semantic");
  panel.innerHTML = "";
  if (!log || log.length === 0) {
    panel.innerHTML = '<p class="placeholder">No semantic checks ran (lex/parse stopped first).</p>';
    return;
  }
  const pre = document.createElement("pre");
  pre.textContent = log.join("\n");
  panel.appendChild(pre);
}

function renderOptimization(reports) {
  const panel = document.getElementById("panel-optimization");
  panel.innerHTML = "";
  if (!reports || reports.length === 0) {
    panel.innerHTML = '<p class="placeholder">No optimization reports (IR generation did not run).</p>';
    return;
  }
  const pre = document.createElement("pre");
  pre.textContent = reports.join("\n");
  panel.appendChild(pre);
}

// Returns the error count so the caller can decide whether to
// jump to this tab and color the status line.
function renderDiagnostics(report) {
  const panel = document.getElementById("panel-diagnostics");
  panel.innerHTML = "";
  const diagnostics = (report && report.diagnostics) || [];

  if (diagnostics.length === 0) {
    panel.innerHTML = '<p class="placeholder">No diagnostics — compilation succeeded.</p>';
    return 0;
  }

  let errorCount = 0;
  for (const d of diagnostics) {
    if (d.severity === "error" || d.severity === "fatal") errorCount++;

    const card = document.createElement("div");
    card.className = "diag";

    let html = `<div class="diag-header">${escapeHtml(d.severity)}[${escapeHtml(d.kind)}]: ${escapeHtml(d.message)}</div>`;
    html += `<div>--&gt; line ${d.span.startLine}, col ${d.span.startCol}</div>`;
    html += `<h4>Root cause</h4><p>${escapeHtml(d.rootCause)}</p>`;
    html += `<h4>Why this happened</h4><p>${escapeHtml(d.explanation).replace(/\n/g, "<br>")}</p>`;

    if (d.fixes && d.fixes.length) {
      html += `<h4>How to fix</h4><ol>`;
      for (const f of d.fixes) html += `<li>${escapeHtml(f)}</li>`;
      html += `</ol>`;
    }

    if (d.trace && d.trace.length) {
      html += `<h4>Internal trace</h4>`;
      for (const step of d.trace) {
        const cls = step.ok ? "trace-step" : "trace-fail";
        html += `<div class="${cls}">[${step.ok ? "ok" : "FAIL"}] ${escapeHtml(step.component)}</div>`;
      }
    }

    card.innerHTML = html;
    panel.appendChild(card);
  }
  return errorCount;
}

// ── AST tree (collapsible, via native <details>/<summary>) ───

function renderAst(ast) {
  const panel = document.getElementById("panel-ast");
  panel.innerHTML = "";
  if (!ast) {
    panel.innerHTML = '<p class="placeholder">No AST (parsing did not produce a tree).</p>';
    return;
  }
  const tree = document.createElement("div");
  tree.className = "ast-tree";
  tree.appendChild(astNodeToDom(ast));
  panel.appendChild(tree);
}

// Renders one AST JSON node as a <details> element, recursing
// into any field whose value is itself a node object ({"node":...})
// or an array of such objects. Plain scalar fields (name, op,
// value, resolvedType, ...) are shown inline in the summary line.
function astNodeToDom(node) {
  const details = document.createElement("details");
  details.open = true;

  const summary = document.createElement("summary");
  const scalarParts = [];
  const childEntries = [];

  for (const [key, value] of Object.entries(node)) {
    if (key === "node") continue;
    if (isAstNode(value)) {
      childEntries.push([key, value]);
    } else if (Array.isArray(value) && value.length > 0 && value.every(isAstNode)) {
      // length > 0 matters: [].every(...) is vacuously true, which
      // would otherwise route an EMPTY array (e.g. a no-arg
      // function's params: []) away from the scalar branch below
      // and silently drop it from the summary line entirely.
      childEntries.push([key, value]);
    } else if (Array.isArray(value)) {
      // e.g. params: [{name,type}, ...] — plain data, not AST nodes
      scalarParts.push(`${key}=${JSON.stringify(value)}`);
    } else {
      scalarParts.push(`<span class="ast-field">${escapeHtml(key)}</span>=${escapeHtml(String(value))}`);
    }
  }

  summary.innerHTML = `<span class="ast-node-name">${escapeHtml(node.node)}</span> ` +
                       scalarParts.join("  ");
  details.appendChild(summary);

  for (const [key, value] of childEntries) {
    if (value === null) continue;
    if (Array.isArray(value)) {
      for (const child of value) details.appendChild(astNodeToDom(child));
    } else {
      details.appendChild(astNodeToDom(value));
    }
  }

  return details;
}

function isAstNode(v) {
  return v !== null && typeof v === "object" && !Array.isArray(v) && "node" in v;
}

function escapeHtml(s) {
  return String(s)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

// Compile the default sample on first load.
compile();
