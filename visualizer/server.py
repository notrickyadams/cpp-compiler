#!/usr/bin/env python3
"""
cpp-compiler visualizer — local dev server.

Stdlib only (http.server, subprocess, json, pathlib) — no pip
install needed, matching the C++ side's zero-dependency test
framework. Serves the static frontend and one endpoint, POST
/compile, that shells out to the real compiled binary and
returns its --json output verbatim.
"""
import http.server
import json
import subprocess
from pathlib import Path

VISUALIZER_DIR = Path(__file__).resolve().parent
PROJECT_ROOT   = VISUALIZER_DIR.parent
COMPILER_EXE   = PROJECT_ROOT / "build" / "compiler.exe"
TEMP_SOURCE    = PROJECT_ROOT / "build" / "_visualizer_input.cpp"

PORT = 8000


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(VISUALIZER_DIR), **kwargs)

    def do_POST(self):
        if self.path != "/compile":
            self.send_error(404)
            return

        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)
        try:
            source = json.loads(body).get("source", "")
        except json.JSONDecodeError:
            self._send_raw_json(json.dumps({"error": "invalid request body"}), 400)
            return

        if not COMPILER_EXE.exists():
            self._send_raw_json(json.dumps({
                "error": f"compiler not found at {COMPILER_EXE} — run `mingw32-make all` first"
            }), 500)
            return

        TEMP_SOURCE.write_text(source, encoding="utf-8")

        # shell=False (the default for a list argv): subprocess talks
        # to CreateProcess directly, bypassing cmd.exe entirely. That
        # sidesteps the cmd.exe "build/x.exe" vs "build\x.exe" parsing
        # quirk this project hit repeatedly in its own C++ test helpers
        # (a MASM32 install's build.bat shadows the forward-slash
        # form) — it's a cmd.exe-specific behavior that doesn't apply
        # when Python launches the process directly.
        result = subprocess.run(
            [str(COMPILER_EXE), str(TEMP_SOURCE), "--json"],
            capture_output=True,
            text=True,
        )

        output = result.stdout.strip()
        if not output:
            output = json.dumps({"error": result.stderr.strip() or "compiler produced no output"})
        self._send_raw_json(output)

    def _send_raw_json(self, raw_text, status=200):
        encoded = raw_text.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)

    def log_message(self, fmt, *args):
        pass  # quiet by default — this is a local dev tool


def main():
    server = http.server.HTTPServer(("127.0.0.1", PORT), Handler)
    print(f"cpp-compiler visualizer running at http://127.0.0.1:{PORT}")
    print(f"compiler binary: {COMPILER_EXE}")
    print("Press Ctrl+C to stop.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
