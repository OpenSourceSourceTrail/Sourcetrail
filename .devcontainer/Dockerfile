FROM ubuntu:22.04

# Configure apt to not install suggested or recommended packages
RUN echo 'APT::Install-Suggests "0";' >> /etc/apt/apt.conf.d/00-docker && \
    echo 'APT::Install-Recommends "0";' >> /etc/apt/apt.conf.d/00-docker

# Set the ARG for non-interactive apt
ARG DEBIAN_FRONTEND=noninteractive

# Remove existing Clang or LLVM installations
RUN apt-get update && apt-get purge -y 'llvm-*' 'clang-*' \
    apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Install all dependencies in one step to reduce layers
RUN apt-get update &&                                                    \
    apt-get install -y                                                   \
    build-essential                                                      \
    wget                                                                 \
    curl                                                                 \
    locales                                                              \
    gcc-12                                                               \
    g++-12                                                               \
    mold                                                                 \
    gdb                                                                  \
    valgrind                                                             \
    git                                                                  \
    git-lfs                                                              \
    ninja-build                                                          \
    python3-pip                                                          \
    qtbase5-dev                                                          \
    libqt5svg5-dev                                                       \
    graphviz                                                             \
    doxygen &&                                                           \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 10 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 10 && \
    apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Generate en_US.UTF-8 locale on the system
RUN locale-gen en_US.UTF-8

# Install CMake in one step
RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0-linux-x86_64.tar.gz -O /tmp/cmake.tar.gz && \
    tar -xzf /tmp/cmake.tar.gz -C /usr/local --strip-components=1 &&                                                            \
    rm /tmp/cmake.tar.gz

# Build Clang from source
RUN INSTALL_PREFIX="/opt/llvm-15.0.7" && \
    wget -q https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-15.0.7.tar.gz -O /tmp/llvmorg-15.0.7.tar.gz && \
    tar -xzf /tmp/llvmorg-15.0.7.tar.gz -C /tmp/ &&                                                                       \
    cmake -G Ninja                                                                                                        \
          -DCMAKE_BUILD_TYPE=Release                                                                                      \
          -DLLVM_ENABLE_PROJECTS=clang                                                                                    \
          -DLLVM_ENABLE_RTTI=ON                                                                                           \
          -DLLVM_TARGETS_TO_BUILD=host                                                                                    \
          -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}                                                                        \
          /tmp/llvm-project-llvmorg-15.0.7/llvm -B /tmp/build &&                                                          \
    cmake --build /tmp/build &&                                                                                           \
    cmake --install /tmp/build &&                                                                                         \
    rm -rf /tmp/llvmorg-15.0.7.tar.gz /tmp/llvm-project-llvmorg-15.0.7 /tmp/build

# Update environment variables
ENV PATH="/opt/llvm-15.0.7/bin:$PATH"
ENV LD_LIBRARY_PATH="/opt/llvm-15.0.7/lib:$LD_LIBRARY_PATH"
ENV CMAKE_PREFIX_PATH="/opt/llvm-15.0.7:$CMAKE_PREFIX_PATH"

# Install Python packages in one step
RUN pip3 install --no-cache-dir conan==2.5.0 cmakelint gcovr

# Add a non-root user in one step
RUN useradd -ms /bin/bash sourcetrail

# Switch to the non-root user
USER sourcetrail
