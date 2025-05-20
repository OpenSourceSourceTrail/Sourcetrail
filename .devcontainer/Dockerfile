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
    graphviz                                                             \
    cppcheck                                                             \
    doxygen                                                              \
    clang                                                                \
    libssl-dev  

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

# Install Qt 6.8.2 from source
RUN apt-get update && \
    apt-get install -y \
    libfontconfig1-dev \
    libfreetype-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxcb-cursor-dev \
    libxcb-glx0-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxcb-shape0-dev \
    libxcb-shm0-dev \
    libxcb-sync-dev \
    libxcb-util-dev \
    libxcb-xfixes0-dev \
    libxcb-xinerama0-dev \
    libxcb-xkb-dev \
    libxcb1-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libxrender-dev

RUN wget -q https://download.qt.io/archive/qt/6.8/6.8.2/single/qt-everywhere-src-6.8.2.tar.xz -O /tmp/qt-everywhere-src-6.8.2.tar.xz && \
    tar -xf /tmp/qt-everywhere-src-6.8.2.tar.xz -C /tmp && \
    mkdir -p /tmp/qt-everywhere-src-6.8.2/build

WORKDIR /tmp/qt-everywhere-src-6.8.2/build

RUN ../configure -prefix /usr/local/qt6.8.2 -opensource -confirm-license -nomake examples -nomake tests -submodules qtbase,qt5compat,qtsvg -no-opengl -openssl-linked -release && \
    cmake --build . --parallel && \
    cmake --install . && \
    rm -rf /tmp/qt-everywhere-src-6.8.2 /tmp/qt-everywhere-src-6.8.2.tar.xz

# Install Qt Installer framework
RUN apt-get update && \
    apt-get install -y \
    libdbus-1-3

RUN wget -O /tmp/ifw-4.7.0.run https://download.qt.io/official_releases/qt-installer-framework/4.7.0/QtInstallerFramework-linux-x64-4.7.0.run && \
    chmod +x /tmp/ifw-4.7.0.run && \
    /tmp/ifw-4.7.0.run --accept-licenses --confirm-command install && \
    cp -r /root/Qt/QtIFW-4.7.0/bin/* /usr/local/qt6.8.2/bin && \
    rm /tmp/ifw-4.7.0.run && \
    rm -rf /root/Qt

# Install linuxdeploy
RUN apt-get update && \
    apt-get install -y \
    fuse \ 
    libfuse2

RUN wget -q -P /tmp/ {https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20250213-2/linuxdeploy-x86_64.AppImage,https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage} && \
    chmod +x /tmp/linuxdeploy*.AppImage && \
    mv /tmp/linuxdeploy-x86_64.AppImage /usr/local/bin/linuxdeploy && \
    mv /tmp/linuxdeploy-plugin-qt-x86_64.AppImage /usr/local/bin/linuxdeploy-plugin-qt

# Add a non-root user in one step
# For further details check https://askubuntu.com/questions/1513927/ubuntu-24-04-docker-images-now-includes-user-ubuntu-with-uid-gid-1000
RUN userdel -r ubuntu
RUN useradd -ms /bin/bash sourcetrail

# Switch to the non-root user
USER sourcetrail

# Install Python packages
RUN python3 -m venv ~/env && \
    ~/env/bin/pip install --no-cache-dir conan==2.5.0 cmakelint==1.4.3 gcovr==5.0 pygments==2.11 pymdown-extensions==9.3
