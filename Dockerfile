FROM espressif/idf:release-v4.3

RUN apt-get update -y && apt-get install -y

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
    
# Python dependencies
RUN pip3 install heatshrink2 hiyapyco  