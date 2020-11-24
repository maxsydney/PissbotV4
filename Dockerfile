FROM espressif/idf:release-v4.1

RUN apt-get update -y && apt-get install -y xxd && apt-get install -y pkg-config

RUN git clone --recursive https://github.com/maxsydney/qemu.git

RUN apt-get install -y libglib2.0-dev

RUN apt-get install -y libpixman-1-dev

run mkdir /qemu/build

WORKDIR "/qemu/build"

RUN ../configure --target-list=xtensa-softmmu \
    --enable-debug --enable-sanitizers \
    --disable-strip --disable-user \
    --disable-capstone --disable-vnc \
    --disable-sdl --disable-gtk

RUN make 