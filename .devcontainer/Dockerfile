FROM ubuntu:22.04

RUN echo 'APT::Install-Suggests "0";'   >> /etc/apt/apt.conf.d/00-docker
RUN echo 'APT::Install-Recommends "0";' >> /etc/apt/apt.conf.d/00-docker

ARG DEBIAN_FRONTEND=noninteractive
ARG COMPILER

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
    ninja-build           \
    python3-pip           \
    qtbase5-dev           \
    libqt5svg5-dev        \
    graphviz              \
    doxygen

# Install CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0-linux-x86_64.tar.gz -O /tmp/cmake.tar.gz && \
    tar -xzf /tmp/cmake.tar.gz -C /usr/local --strip-components=1 && \
    rm /tmp/cmake.tar.gz

# Set PATH environment variable
ENV PATH="/usr/local/bin/cmake:${PATH}"

# Clean ups
RUN apt-get autoremove -y \
    && apt-get clean -y   \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install \
  conan==2.5.0  \
  cmakelint      \
  gcovr

RUN conan install . \                                                                                                                                                                 SOUR-60 *2 +4 !37 ?14 
-s build_type=Debug \
-of .conan/gcc/ \
-b missing \
-pr:a .conan/gcc/profile

RUN useradd -ms /bin/bash sourcetrail
USER sourcetrail

WORKDIR /home/sourcetrail
COPY --chown=sourcetrail:sourcetrail . /home/sourcetrail

RUN if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "clang" ]; then \
      conan install . -s build_type=Release -of .conan/${COMPILER}/ -b missing -pr:a .conan/${COMPILER}/profile; \
    else \
      echo "Unknown compiler"; \
      exit 1; \
    fi
  
RUN if [ "$COMPILER" = "gcc" ]; then \
      cmake --preset=gnu_release; \
    else \
      cmake --preset=clang_release; \
    fi

WORKDIR /home/sourcetrail/build/Release/$(if [ "$COMPILER" = "gcc" ]; then echo "GNU"; else echo "CLang"; fi)

RUN ninja

CMD ["./Sourcetrail"]