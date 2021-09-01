FROM espressif/idf:release-v4.3

RUN apt-get update -y && apt-get install -y
RUN apt-get install -y libglib2.0-dev && \
    apt-get install -y ninja-build && \
    apt-get install -y libpixman-1-dev && \ 
    apt-get install -y libgcrypt20-dev

# Install modified qemu
RUN git clone --recursive https://github.com/maxsydney/qemu.git
WORKDIR "/qemu"
RUN ./configure --target-list=xtensa-softmmu \
    --enable-gcrypt --enable-sanitizers \
    --enable-debug --enable-sanitizers \
    --disable-strip --disable-user \
    --disable-capstone --disable-vnc \
    --disable-sdl --disable-gtk
RUN ninja -C build

# Install node tools
ENV NVM_VERSION v0.38.0
ENV NODE_VERSION v16.8.0
ENV NVM_DIR /usr/local/nvm
RUN mkdir $NVM_DIR
RUN curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.11/install.sh | bash

ENV NODE_PATH $NVM_DIR/v$NODE_VERSION/lib/node_modules
ENV PATH $NVM_DIR/versions/node/v$NODE_VERSION/bin:$PATH

RUN echo "source $NVM_DIR/nvm.sh && \
    nvm install $NODE_VERSION && \
    nvm alias default $NODE_VERSION && \
    nvm use default" | bash
    
# Install Python dependencies
RUN pip3 install heatshrink2 hiyapyco unity-test-parser