
.PHONY: check_crash all default tarball

ifneq ($(TARGET),)
TARGET_OPTION=TARGET="${TARGET}"
endif

CFLAGS="-g -O0"

default: check_crash
	@cp ${CRASH}/defs.h src
	@make -C src -f ./gcore.mk CFLAGS=${CFLAGS} ${TARGET_OPTION}

gcore_version: src/gcore.mk
GCORE_VERSION=$(shell awk -F = -e '/^VERSION/ {print $$2}' src/gcore.mk)

tarball: gcore_version
	@(cp -r ./src ./crash-gcore-command-${GCORE_VERSION}; \
	cp ./COPYING ./crash-gcore-command-${GCORE_VERSION}; \
	tar zcf crash-gcore-command-${GCORE_VERSION}.tar.gz crash-gcore-command-${GCORE_VERSION}; \
	rm -rf ./crash-gcore-command-${GCORE_VERSION})

check_crash:
ifndef CRASH
	@echo "Please specify CRASH=<directory>"
	@exit 1
endif

clean:
	@(rm -f src/defs.h)
	@(make -C src -f gcore.mk clean)
	@(rm -f crash-gcore-command-*.tar.gz)
