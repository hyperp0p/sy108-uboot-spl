"""Package U-Boot with the donor SML payload and signature descriptor."""
import hashlib
import json
from pathlib import Path
import struct
import sys

READ_LIMIT = 0xC0000
PARTITION = 0x140000


def package(uboot, sml, output):
    code = uboot.read_bytes()
    donor = sml.read_bytes()
    if len(donor) < 512 or donor[:8] != b'DHTB\1\0\0\0':
        raise ValueError('SML is not an original SY108 DHTB image')
    old_size = struct.unpack_from('<I', donor, 48)[0]
    if 512 + old_size + 96 > len(donor):
        raise ValueError('Incomplete SML signature descriptor')
    descriptor = bytearray(donor[512 + old_size:512 + old_size + 96])
    fields = struct.unpack('<8sII10Q', descriptor)
    size, offset, cert_size, cert_offset = fields[3:7]
    if fields[:3] != (bytes(8), 0, 0) or (size, offset, cert_size) != (old_size, 512, 564):
        raise ValueError('Expected the unmodified SY108 SML content-certificate format')
    if any(fields[7:]) or cert_offset != 512 + old_size + 96:
        raise ValueError('Unexpected optional data or certificate location')
    certificate = donor[cert_offset:cert_offset + cert_size]
    signed = donor[offset:offset + size]
    if len(certificate) != cert_size or struct.unpack_from('<II', certificate) != (0, 2048):
        raise ValueError('Missing original SML content certificate')
    if hashlib.sha256(signed).digest() != certificate[268:300]:
        raise ValueError('Original SML payload hash does not match its certificate')
    padding = bytes((-len(code)) % 16)
    prefix = code + padding
    body = prefix + signed
    new_offset = 512 + len(prefix)
    new_cert = 512 + len(body) + 96
    required = new_cert + cert_size
    if required > READ_LIMIT:
        raise ValueError('Required bytes %d exceed SPL read limit %d' % (required, READ_LIMIT))
    struct.pack_into('<Q', descriptor, 24, new_offset)
    struct.pack_into('<Q', descriptor, 40, new_cert)
    header = bytearray(donor[:512])
    header[8:40] = hashlib.sha256(body).digest()
    struct.pack_into('<Q', header, 40, 0x9F000000)
    struct.pack_into('<I', header, 48, len(body))
    result = bytes(header) + body + descriptor + certificate
    result += b'\xff' * (PARTITION - len(result))
    output.write_bytes(result)
    return {'partition_bytes': len(result), 'required_bytes': required,
            'spl_read_bytes': READ_LIMIT, 'spare_read_bytes': READ_LIMIT-required,
            'uboot_dtb_bytes': len(code), 'sml_payload_bytes': len(signed),
            'signed_payload_offset': new_offset, 'signed_payload_bytes': size,
            'certificate_offset': new_cert, 'certificate_bytes': cert_size,
            'sha256': hashlib.sha256(result).hexdigest()}


if __name__ == '__main__':
    report = package(*(Path(p) for p in sys.argv[1:]))
    print(json.dumps(report, indent=2))
