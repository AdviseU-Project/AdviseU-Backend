# AdviseU-Backend

Backend server used for AdviseU's scheduling algorithm

## Build with Docker

Ensure you have Docker Desktop running, then run:

```
docker build -t adviseu-backend .
```

## Or Build Manually

- First, build/install `Gecode` on your machine from `libs/gecode-release-6.2.0`.

  ```bash
  cd libs/gecode-release-6.2.0
  ./configure --disable-examples --prefix=$(pwd)/install
  make
  make install
  export LD_LIBRARY_PATH=$(pwd)/install/lib:$LD_LIBRARY_PATH
  ```

- Second, build/install `mongo-cxx-driver` on your machine from `libs/mongo-cxx-driver-r4.0.0`.

  ```bash
  cd libs/mongo-cxx-driver-r4.0.0/build
  cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17
  cmake --build .
  sudo cmake --build . --target install
  ```

- Third, clone/build/install `Boost >=1.88.0` on your machine.

  ```bash
  git clone --recursive https://github.com/boostorg/boost.git
  cd boost
  mkdir -p build
  cd build
  cmake ..
  cmake --build .
  cmake --build . --target install
  ```

- Ensure your MongoDB credentials are added to `src/main.cpp:29`.
- Then you can run the following to build and start the server.
  ```bash
  rm -r build
  mkdir build
  cd build
  cmake ..
  cd ..
  cmake --build build
  cd build
  ./AdviseU-Backend
  ```
