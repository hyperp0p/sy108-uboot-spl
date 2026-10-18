"""Export linked firmware and create a manifest of the current build."""
import hashlib
import json
from pathlib import Path
import shutil
import sys


def align16(data):
    return data + bytes((-len(data)) % 16)


def export(part, source, output):
    target = output / part
    target.mkdir(parents=True, exist_ok=True)
    if part == 'spl':
        source = source / 'nand_spl'
        names = ['u-boot-spl', 'u-boot-spl.bin', 'u-boot-spl.map']
    else:
        names = ['u-boot', 'u-boot.bin', 'u-boot.dtb', 'u-boot-dtb.bin', 'u-boot.map']
    for name in names:
        shutil.copy2(source / name, target / name)
    if part == 'spl':
        data = (target / 'u-boot-spl.bin').read_bytes()
        name = 'u-boot-spl-aligned16.bin'
    else:
        data = (target / 'u-boot-dtb.bin').read_bytes()
        name = 'u-boot-dtb-aligned16.bin'
    (target / name).write_bytes(align16(data))
    print('Exported %s firmware to %s' % (part, target))


def manifest(output):
    rows = []
    for part in ['spl', 'uboot']:
        for path in sorted((output / part).glob('*')):
            if path.is_file():
                rows.append({
                    'file': str(path.relative_to(output)),
                    'size': path.stat().st_size,
                    'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                })
    (output / 'manifest.json').write_text(json.dumps({'artifacts': rows}, indent=2) + '\n')
    print('Wrote manifest.json for %d artifacts' % len(rows))


command = sys.argv[1]
if command in ['spl', 'uboot']:
    export(command, Path(sys.argv[2]), Path(sys.argv[3]))
elif command == 'manifest':
    manifest(Path(sys.argv[2]))
else:
    raise SystemExit('Usage: package.py spl|uboot SOURCE OUTPUT, or manifest OUTPUT')
