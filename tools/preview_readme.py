#!/usr/bin/env python3
"""Preview the repository README, board video and scroll-following Dino locally."""
import argparse
import json
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8765)
    args = parser.parse_args()
    # Keep a generated fallback for opening index.html directly from disk.
    snapshot = '// Generated from README.md by tools/preview_readme.py.\n'
    snapshot += 'window.LOCAL_README_MARKDOWN = ' + json.dumps((ROOT/'README.md').read_text(encoding='utf-8-sig'), ensure_ascii=True) + ';\n'
    (ROOT/'docs/interactive/readme-copy.js').write_text(snapshot, encoding='utf-8')
    handler = partial(SimpleHTTPRequestHandler, directory=str(ROOT))
    try:
        server = ThreadingHTTPServer(('127.0.0.1', args.port), handler)
    except OSError as error:
        parser.exit(1, f'Could not start the preview: {error}. Try --port 8766.\n')
    print(f'Open http://127.0.0.1:{server.server_port}/docs/interactive/', flush=True)
    print('Local preview only. Press Ctrl+C to stop.', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
