mkdir build-conda
pushd build-conda

# for conda, pybind11 should detect the python installation correctly
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -DPYTHON_EXECUTABLE=$PREFIX/bin/python \
     ..

make -j4
make install

# python package built and installed to site-package by make install
# ` build-conda/python` folder should have been copied/symlink to
#python python/setup.py install --single-version-externally-managed --record record.txt

popd
