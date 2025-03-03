export VER_POD_IMAGE = freertosbuildrp2040
.PHONY: all run test container build

# Path configs
BUILD_DIR = build
SRC_DIR=src
CONTAINER_DIR=container
LDIR=$(shell pwd)

# Podman arguments shared
export PODMAN_ARGS = --rm -v ${LDIR}/:/mnt
PODMAN_RUN = podman run ${PODMAN_ARGS}
PODMAN_CONTAINER_RUN = ${PODMAN_RUN} -t ${VER_POD_IMAGE}

# RP SDK Configs
export PICO_PLATFORM
export PICO_SDK_PATH=${LDIR}/pico-sdk

container:
	cd ${CONTAINER_DIR} ; make all

build:
	git submodule update --init
	cmake -S . -B ${BUILD_DIR}

compile: build
	cmake --build ${BUILD_DIR} -j$(shell nproc)

container-build: container
	${PODMAN_CONTAINER_RUN} make build

container-compile: container
	${PODMAN_CONTAINER_RUN} make compile

container-clean: container
	${PODMAN_CONTAINER_RUN} make clean

container-it: container
	${PODMAN_RUN} -it ${VER_POD_IMAGE}

clean-container:
	cd ${CONTAINER_DIR} ; make clean

clean:
	rm -rf ${BUILD_DIR}

clean-all: clean clean-container
