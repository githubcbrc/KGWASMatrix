FROM ubuntu:20.04

# Avoid warnings by switching to noninteractive
ARG DEBIAN_FRONTEND=noninteractive

# Configure apt and install packages
RUN apt-get update && apt-get -y install --no-install-recommends apt-utils dialog 2>&1

RUN apt-get -y install \
    # Verify git, process tools, lsb-release (useful for CLI installs) installed
    git git-flow iproute2 procps lsb-release \
    # Install GNU GCC Toolchain, version 10
    gcc-10 g++-10 gdb libstdc++-10-dev libc6-dev \
    # Install LLVM Toolchain, version 10
    clang-10 clangd-10 \
    llvm-10 lldb-10 libc++-10-dev \
    # Install build generator & dependency resolution and build accelarator tools
    make ninja-build autoconf automake libtool m4 cmake ccache



ARG DEBIAN_FRONTEND=dialog

RUN mkdir -p /project
WORKDIR /project


