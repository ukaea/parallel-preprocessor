#!/bin/bash

#inside the build folder,  assuming out of source building
if [ -d wiki ]; then
echo "this script only work for out of source building like this`
echo `mkdir build && cd build && cmake ..``"
exit -1
fi

echo "start test in the folder $(pwd)"
# using symbolic link created manually, instead of copy for POSIX
# this may have been done in cmake
if [ ! -d data ]; then
ln -s ../data  data
#cp -r ../data  data  # this will not fail even data folder exists
fi

# # this may have been done in cmake, but fine to redo it
if [ -d ppptest ]; then rm -rf ppptest; fi
mkdir ppptest && cd ppptest
cp ../../src/python/*.py ./

echo "start unit test in the folder:$(pwd)"
../bin/pppBaseTests
../bin/pppParallelTests
#../bin/pppAppTests  # PPP::CommandLineProcessor  is not merged 
../bin/pppGeomTests


echo "start python tests in the folder:$(pwd)"
# this for loop is support by bash only, not sh
for ((i=1;i<=10;i++)) ; do echo "sample_file$i.txt";  done > sampleManifest.txt
python3 pppPipelineController.py sampleManifest.txt

python3 geomPipeline.py search ../data/test_geometry/test_geometry.stp --verbosity WARNING
if [ ! $? -eq 0 ]
then
  echo "Error during geometry search action"
  exit 1
fi
python3 geomPipeline.py detect ../data/test_geometry/test_geometry.stp --verbosity WARNING
# run `python3 geomPipeline.py` to generate a config.json file for test_*.py below
python3 geomPipeline.py ../data/test_geometry/test_geometry.stp --verbosity WARNING

if [ $? -eq 0 ]
then
  echo "Successfully run geometry pipeline"
  python3 test_imprint.py
  #python3 test_collision.py
  #python3 test_fixing.py
  if [ $? -eq 0 ]; then
    python3 test_collision.py
  fi
  echo "test completed"
else
  echo "geometry pipeline test failed"
  exit 2
fi

