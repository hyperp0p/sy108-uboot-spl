#!/usr/bin/env python3
"""Fetch pinned dependencies only while building Docker; no host cache needed."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

lock = json.loads(Path(sys.argv[1]).read_text())
for name, spec in lock.items():
    archive = Path('/tmp') / (name + '.tar')
    subprocess.run(['curl', '--fail', '--location', '--silent', '--show-error',
                    '--retry', '3', '--connect-timeout', '20', '--max-time', '600',
                    '--output', str(archive), spec['url']], check=True)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != spec['sha256']:
        raise SystemExit('SHA-256 mismatch for ' + name + ': ' + digest)
    dest = Path(spec['destination'])
    dest.mkdir(parents=True)
    subprocess.run(['tar', '-xf', str(archive), '-C', str(dest),
                    '--strip-components=1'], check=True)
    archive.unlink()
    print('%s: verified %s (%s)' % (name, spec['kind'], spec['version']), flush=True)
