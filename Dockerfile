# Use an official base image
FROM ubuntu:20.04

# Set the environment variables to non-interactive to avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    sudo \
    libssl-dev \
    libboost-all-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Install MongoDB C++ Driver dependencies
RUN apt-get update && apt-get install -y \
    libmongo-cxx-driver-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy your project files into the container
COPY . /app

# Install Gecode
WORKDIR /app/libs/gecode-release-6.2.0
RUN ./configure --disable-examples --prefix=$(pwd)/install && \
    make && \
    make install

# Set the LD_LIBRARY_PATH environment variable so that the system can find Gecode's libraries
ENV LD_LIBRARY_PATH=/app/libs/gecode-release-6.2.0/install/lib:$LD_LIBRARY_PATH

# Install MongoDB C++ Driver
WORKDIR /app/libs/mongo-cxx-driver-r4.0.0/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 && \
    cmake --build . && \
    sudo cmake --build . --target install

# Ensure MongoDB credentials are added (consider using environment variables for security)
# You can pass MongoDB credentials as Docker environment variables or mount a secret file
# Example (to be set when running the container):
# ENV MONGO_USERNAME="your_username" 
# ENV MONGO_PASSWORD="your_password"

# Build AdviseU-Backend
WORKDIR /app
RUN rm -r build && \
    mkdir build && \
    cd build && \
    cmake .. && \
    cmake --build . 

# Expose port that the backend listens on
EXPOSE 8080

# Command to run the backend server
CMD ["./build/AdviseU-Backend"]
