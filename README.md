# AdviseU-Backend

Backend server used for AdviseU's scheduling algorithm

# Build

- First, build/install `Gecode` on your machine from `libs/gecode-release-6.2.0`

  ```bash
  cd libs/gecode-release-6.2.0
  ./configure --disable-examples --prefix=$(pwd)/install
  make
  make install
  export LD_LIBRARY_PATH=$(pwd)/install/lib:$LD_LIBRARY_PATH
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
