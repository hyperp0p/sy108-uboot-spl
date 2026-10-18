LABEL ?= dev
.PHONY: all uboot spl offline
export SPL_IMAGE SML_IMAGE MAIN_CAPSULE RECOVERY_CAPSULE
all:
	sh docker/build.sh "$(LABEL)" all
uboot:
	sh docker/build.sh "$(LABEL)" uboot
spl:
	sh docker/build.sh "$(LABEL)" spl

offline:
	sh docker/build.sh "$(LABEL)" offline
