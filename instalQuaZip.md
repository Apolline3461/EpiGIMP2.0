# Install QuaZip

## Linux

```bash
git clone https://github.com/stachenov/quazip
cd quazip
mkdir build && cd build
cmake .. \
  -DQUAZIP_QT_MAJOR_VERSION=6 \
  -DBUILD_WITH_QT6=ON \
  -DQUAZIP_USE_QT5_CODEC=OFF \
  -DCMAKE_INSTALL_PREFIX=/usr/local
make -j
sudo make install
```