LABEL ?= dev
.PHONY: all uboot spl
all:
	sh docker/build.sh "$(LABEL)" all
uboot:
	sh docker/build.sh "$(LABEL)" uboot
spl:
	sh docker/build.sh "$(LABEL)" spl
