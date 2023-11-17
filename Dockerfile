FROM ubuntu:23.10 as ubuntu-base

ENV DEBIAN_FRONTEND=noninteractive
ENV LD_LIBRARY_PATH=/usr/local/lib/

ARG BOOST_VERSION=1.82.0

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
        wget \
        git \
        curl \
    && rm -fr /var/lib/apt/lists/*

FROM ubuntu-base

# System locale
# Important for UTF-8
#ENV LC_ALL en_US.UTF-8
#ENV LANG en_US.UTF-8
#ENV LANGUAGE en_US.UTF-8

# Install Boost
RUN cd /tmp && \
    BOOST_VERSION_MOD=$(echo $BOOST_VERSION | tr . _) && \
    wget --no-check-certificate https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_MOD}.tar.gz && \
    tar -xvf boost_${BOOST_VERSION_MOD}.tar.gz && \
    cd boost_${BOOST_VERSION_MOD} && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 install && \
    rm -rf /tmp/*

RUN echo "alias ll='ls -la'" >> /root/.bashrc

RUN mkdir -p /src/build
WORKDIR /src/build

CMD [ "/bin/bash" ]