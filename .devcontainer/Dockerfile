FROM ubuntu:22.04

RUN echo 'APT::Install-Suggests "0";'   >> /etc/apt/apt.conf.d/00-docker
RUN echo 'APT::Install-Recommends "0";' >> /etc/apt/apt.conf.d/00-docker

ARG DEBIAN_FRONTEND=noninteractive

# Install
RUN apt-get update        \
    && apt-get install -y \
    build-essential       \
    wget                  \
    curl                  \
    clang                 \
    mold                  \
    gdb                   \
    valgrind              \
    git                   \
    git-lfs               \
    cmake                 \
    ninja-build           \
    python3-pip           \
    libboost-all-dev      \
    qtbase5-dev           \
    libqt5svg5-dev        \
    graphviz              \
    doxygen

# Clean ups
RUN apt-get autoremove -y \
    && apt-get clean -y   \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install \
  conan==1.59.0  \
  cmakelint      \
  gcovr

RUN useradd -ms /bin/bash sourcetrail
USER sourcetrail