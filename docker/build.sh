#!/bin/sh
# Optional POSIX convenience wrapper: the host only runs Docker.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
label=${1:-dev}
part=${2:-all}
case "$label" in *[!a-zA-Z0-9_-]*|'') echo 'Invalid output name.' >&2; exit 64;;esac
case "$part" in all|spl|uboot);;*)exit 64;;esac
image=${BOOT_IMAGE:-udx710-boot:build}
docker build --platform linux/amd64 -f "$root/docker/Dockerfile" -t "$image" "$root"
out=$root/build/$label
mkdir -p "$out"
exec docker run --rm --platform linux/amd64 --network none --user "$(id -u):$(id -g)" \
 --mount "type=bind,source=$out,target=/out" "$image" "$part"
