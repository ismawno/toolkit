# Use an Ubuntu base image
FROM ubuntu:22.04

RUN apt-get update
RUN apt-get install -y git
RUN apt-get install -y build-essential
RUN apt-get install -y cmake
RUN apt-get install -y lldb
RUN apt-get install -y gdb
RUN apt-get install -y python3-lldb
RUN apt-get install -y python3
RUN apt-get install -y python3-venv
RUN rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy your application source code into the container
COPY ./toolkit/ ./toolkit/
COPY ./CMakeLists.txt ./
COPY ./tests/ ./tests/
COPY ./setup ./setup
COPY ./convoy.py ./

# Build your application
# RUN mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DTOOLKIT_BUILD_TESTS=ON && make