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

echo "start unit test in the folder:$(pwd)"
../bin/pppBaseTests
../bin/pppParallelTests
../bin/pppAppTests  
../bin/pppGeomTests


echo "start python tests in the folder:$(pwd)"
# this for loop is support by bash only, not sh
#for ((i=1;i<=10;i++)) ; do echo "sample_file$i.txt";  done > sampleManifest.txt
# this works for sh
for i in `seq 1 10` ; do echo "sample_file$i.txt";  done > sampleManifest.txt
# PPP::CommandLineProcessor  is not merged 
#pppPipelineController.py sampleManifest.txt

geomPipeline.py search ../data/test_geometry/test_geometry.stp --verbosity WARNING
if [ ! $? -eq 0 ]
then
  echo "Error during geometry search action"
  exit 1
fi
geomPipeline.py detect ../data/test_geometry/test_geometry.stp --verbosity WARNING
# run `geomPipeline.py` to generate a config.json file for test_*.py below
geomPipeline.py ../data/test_geometry/test_geometry.stp -o test_geometry_result.brep
# test manifest.json input file type
geomPipeline.py check ../data/test_geometry/test_geometry_manifest.json --verbosity WARNING
if [ ! $? -eq 0 ] ; then   echo "Error during geometry test" ;  exit 1 ; fi

# test single thread mode, only for non-coupled operations
geomPipeline.py check ../data/test_geometry/test_geometry.stp --thread-count 1 --verbosity WARNING
if [ ! $? -eq 0 ] ; then   echo "Error during geometry test in single thread mode" ;  exit 1 ; fi

# test_*.py has not been installed by package, so must be copied into this place
cp ../../src/python/*.py ./
if [ $? -eq 0 ]
then
  echo "Successfully run geometry pipeline"
  test_imprint.py
  #test_collision.py
  #test_fixing.py
  if [ $? -eq 0 ]; then
    test_collision.py
  fi
  echo "test completed"
else
  echo "geometry pipeline test failed"
  exit 2
fi

