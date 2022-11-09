FROM ubuntu:kinetic as ubuntu-base

ENV DEBIAN_FRONTEND=noninteractive
ENV LD_LIBRARY_PATH=/usr/local/lib/

RUN apt-get -qq update -y \
    && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends \
        g++ \
        clang-format \
        doxygen \
        vim \
        cmake \
        ninja-build \
        openssl \
        libssl-dev \
        libboost-system-dev \
        libboost-thread-dev \
        libboost-test-dev \
    && rm -fr /var/lib/apt/lists/*

FROM ubuntu-base

RUN echo "alias ll='ls -la'" >> /root/.bashrc

RUN mkdir -p /src/build
WORKDIR /src/build

CMD [ "/bin/bash" ]