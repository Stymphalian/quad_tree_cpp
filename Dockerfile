FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata
RUN apt-get install -y vim build-essential git cmake net-tools gdb clang
RUN apt-get install -y wget unzip libopencv-dev ffmpeg

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libgtest-dev \
    libssl-dev \
    pkg-config \
    python3 \
    python3-pip \
    wget

# Set environment variables
# ENV CXX=g++-8
# ENV CC=gcc-8

# Create a working directory
WORKDIR /work
