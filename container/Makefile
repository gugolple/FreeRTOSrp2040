.PHONY: all run test

all: .Dockerfile

.Dockerfile: Dockerfile
	podman build -t ${VER_POD_IMAGE} -f Dockerfile
	touch -m .Dockerfile
clean:
	rm -f .Dockerfile
