#!/bin/sh
# Build U-Boot/SPL or run offline envelope validation in Docker.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
label=${1:-dev}
part=${2:-all}
case "$label" in *[!a-zA-Z0-9_-]*|'') echo 'Invalid output name.' >&2; exit 64;;esac
case "$part" in all|spl|uboot|offline);;*)exit 64;;esac
if test "$part" = offline; then
 : "${SPL_IMAGE:?Set SPL_IMAGE to the original SPL input}"
 : "${SML_IMAGE:?Set SML_IMAGE to the original SML input}"
 : "${MAIN_CAPSULE:?Set MAIN_CAPSULE to the main boot capsule}"
 : "${RECOVERY_CAPSULE:?Set RECOVERY_CAPSULE to the Recovery boot capsule}"
 for input in "$SPL_IMAGE" "$SML_IMAGE" "$MAIN_CAPSULE" "$RECOVERY_CAPSULE"; do
  test -f "$input" || { echo "Missing read-only input: $input" >&2; exit 66; }
 done
 git -C "$root" diff --exit-code oem -- src/chipram >/dev/null
fi
image=${BOOT_IMAGE:-udx710-boot:build}
docker build --platform linux/amd64 -f "$root/docker/Dockerfile" -t "$image" "$root"
out=$root/build/$label
mkdir -p "$out"
if test "$part" = offline; then
 exec docker run --rm --platform linux/amd64 --network none --user "$(id -u):$(id -g)" \
  --env "OEM_COMMIT=$(git -C "$root" rev-parse oem)" \
  --env "SOURCE_COMMIT=$(git -C "$root" rev-parse HEAD)" \
  --env "BUILD_IMAGE_ID=$(docker image inspect "$image" --format '{{.Id}}')" \
  --env "SOURCE_DIRTY=$(test -z "$(git -C "$root" status --porcelain)" && echo 0 || echo 1)" \
  --env "SPL_SOURCE=$SPL_IMAGE" --env "SML_SOURCE=$SML_IMAGE" \
  --env "MAIN_SOURCE=$MAIN_CAPSULE" --env "RECOVERY_SOURCE=$RECOVERY_CAPSULE" \
  --mount "type=bind,source=$SPL_IMAGE,target=/inputs/spl.bin,readonly" \
  --mount "type=bind,source=$SML_IMAGE,target=/inputs/sml.bin,readonly" \
  --mount "type=bind,source=$MAIN_CAPSULE,target=/inputs/main.bin,readonly" \
  --mount "type=bind,source=$RECOVERY_CAPSULE,target=/inputs/recovery.bin,readonly" \
  --mount "type=bind,source=$out,target=/out" --entrypoint /bin/sh "$image" /opt/boot/offline.sh
fi
exec docker run --rm --platform linux/amd64 --network none --user "$(id -u):$(id -g)" \
 --mount "type=bind,source=$out,target=/out" "$image" "$part"
