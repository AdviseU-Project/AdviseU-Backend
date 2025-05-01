# Stage 1: Base dependencies
FROM ubuntu:24.04 AS base

## Set the environment variables to non-interactive to avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

## Install common dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    wget \
    curl \
    sudo \
    libssl-dev \
    libcurl4-openssl-dev \
    autoconf \
    automake \
    libtool \
    dos2unix \
    file \
    pkg-config \
    python3 \
    libzstd-dev \
    libsasl2-dev \
    && rm -rf /var/lib/apt/lists/*

# Stage 2: Build CMake
FROM base AS cmake-builder
WORKDIR /tmp
RUN wget https://github.com/Kitware/CMake/releases/download/v3.23.0/cmake-3.23.0-linux-x86_64.sh -q -O /tmp/cmake-install.sh && \
    chmod u+x /tmp/cmake-install.sh && \
    mkdir -p /opt/cmake && \
    /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake && \
    ln -s /opt/cmake/bin/* /usr/local/bin/ && \
    rm /tmp/cmake-install.sh

# Stage 3: Build Boost >=1.88.0
FROM cmake-builder AS boost-builder
WORKDIR /tmp
RUN git clone --recursive https://github.com/boostorg/boost.git && \
    cd boost && \
    mkdir -p build && \
    cd build && \
    cmake .. && \
    cmake --build . && \
    cmake --build . --target install

# Stage 4: Build Gecode
FROM cmake-builder AS gecode-builder
WORKDIR /app
COPY libs/gecode-release-6.2.0 /app/libs/gecode-release-6.2.0
WORKDIR /app/libs/gecode-release-6.2.0
RUN autoreconf -i && \
    ./configure --disable-examples --prefix=/app/libs/gecode-release-6.2.0/install && \
    make && \
    make install

# Stage 5: Build MongoDB C++ Driver
FROM cmake-builder AS mongodb-builder
WORKDIR /app
COPY libs/mongo-cxx-driver-r4.0.0 /app/libs/mongo-cxx-driver-r4.0.0
WORKDIR /app/libs/mongo-cxx-driver-r4.0.0

## Initialize as git repo to avoid version detection errors
RUN git init . && git add . && git config --global user.email "docker@example.com" && git config --global user.name "Docker Build" && git commit -m "Initial commit"

## Build MongoDB driver with explicit version
RUN mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 \
             -DPYTHON_EXECUTABLE=$(which python3) \
             -DBUILD_VERSION=4.0.0 && \
    cmake --build . && \
    cmake --build . --target install

# Stage 6: Final image with only necessary components
FROM base AS runtime

## Add ldconfig to ensure shared libraries are found
RUN ldconfig
WORKDIR /app

## Copy built components from previous stages
COPY --from=cmake-builder /opt/cmake /opt/cmake
COPY --from=boost-builder /usr/local/include/boost /usr/local/include/boost
COPY --from=boost-builder /usr/local/lib /usr/local/lib
COPY --from=gecode-builder /app/libs/gecode-release-6.2.0/install /app/libs/gecode-release-6.2.0/install
COPY --from=mongodb-builder /usr/local/include/mongocxx /usr/local/include/mongocxx
COPY --from=mongodb-builder /usr/local/include/bsoncxx /usr/local/include/bsoncxx
COPY --from=mongodb-builder /usr/local/lib /usr/local/lib

## Create lib64 directory for MongoDB libraries
RUN mkdir -p /usr/local/lib64

## Copy MongoDB lib64 if it exists (done in a RUN command to handle conditionally)
RUN if [ -d "/usr/local/lib64" ] && [ "$(ls -A /usr/local/lib64)" ]; then \
      cp -rf /usr/local/lib64/* /usr/local/lib/ || true; \
    fi

## Make sure cmake is in the PATH
RUN ln -s /opt/cmake/bin/* /usr/local/bin/ && \
    ldconfig

## Set LD_LIBRARY_PATH for Gecode
ENV LD_LIBRARY_PATH=/app/libs/gecode-release-6.2.0/install/lib:$LD_LIBRARY_PATH

## Copy application code
COPY . /app

## Build AdviseU-Backend
WORKDIR /app
RUN rm -rf build && \
    mkdir -p build && \
    cd build && \
    cmake .. && \
    cmake --build .

## Expose port and run command
EXPOSE 8080
CMD ["./build/AdviseU-Backend"]
