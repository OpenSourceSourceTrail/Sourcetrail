FROM ubuntu:24.04

# Configure apt to not install suggested or recommended packages
RUN echo 'APT::Install-Suggests "0";' >> /etc/apt/apt.conf.d/00-docker && \
    echo 'APT::Install-Recommends "0";' >> /etc/apt/apt.conf.d/00-docker

# Set the ARG for non-interactive apt
ARG DEBIAN_FRONTEND=noninteractive

# Install all dependencies in one step to reduce layers
RUN apt-get update &&                                                    \
    apt-get install -y                                                   \
    build-essential                                                      \
    wget                                                                 \
    curl                                                                 \
    locales                                                              \
    gcc                                                                  \
    g++                                                                  \
    mold                                                                 \
    gdb                                                                  \
    valgrind                                                             \
    git                                                                  \
    git-lfs                                                              \
    ninja-build                                                          \
    python3-venv                                                         \
    qtbase5-dev                                                          \
    libqt5svg5-dev                                                       \
    graphviz                                                             \
    cppcheck                                                             \
    doxygen                                                              \
    clang                                                           

# Generate en_US.UTF-8 locale on the system
RUN locale-gen en_US.UTF-8

# Install CMake in one step
RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0-linux-x86_64.tar.gz -O /tmp/cmake.tar.gz && \
    tar -xzf /tmp/cmake.tar.gz -C /usr/local --strip-components=1 &&                                                            \
    rm /tmp/cmake.tar.gz

# Build Clang from source
ENV INSTALL_PREFIX="/opt/llvm-19.1.7"
RUN mkdir -p "/tmp/llvm" &&                                                                               \
    git clone --depth 1 --branch llvmorg-19.1.7 "https://github.com/llvm/llvm-project.git" "/tmp/llvm" && \
    cmake -S "/tmp/llvm/llvm" -B "/tmp/llvm/build"                                                        \
    -G Ninja                                                                                              \
    -DCMAKE_BUILD_TYPE=Release                                                                            \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"                                                            \
    -DLLVM_ENABLE_PROJECTS="clang"                                                                        \
    -DLLVM_ENABLE_RTTI=ON                                                                                 \
    -DCLANG_LINK_CLANG_DYLIB=ON                                                                           \
    -DLLVM_LINK_LLVM_DYLIB=ON                                                                             \
    -DLLVM_TARGETS_TO_BUILD=host &&                                                                       \
    ninja -C "/tmp/llvm/build" -j"$(nproc)" &&                                                            \
    ninja -C "/tmp/llvm/build" install &&                                                                 \
    rm -rf "/tmp/llvm"

# Add a non-root user in one step
# For further details check https://askubuntu.com/questions/1513927/ubuntu-24-04-docker-images-now-includes-user-ubuntu-with-uid-gid-1000
RUN userdel -r ubuntu
RUN useradd -ms /bin/bash sourcetrail

# Switch to the non-root user
USER sourcetrail

# Install Python packages
RUN python3 -m venv ~/env && \
    ~/env/bin/pip install --no-cache-dir conan==2.5.0 cmakelint==1.4.3 gcovr==5.0 pygments==2.11 pymdown-extensions==9.3
