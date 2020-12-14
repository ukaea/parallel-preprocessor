# to generate site on localhost, to debug the similar process in github workflow
# tested on Ubuntu 20.04, not all sections are working, 
# coverage and doc can be viewed without running this script.
# this script must be run in the oroject root folder
# github-ci run in project root,  assuming there exist 2 folders: doxygen

MODULES="PPP Geom"
REPO_DIR="./"
SRC_DIR="$REPO_DIR/src/"
MODULE_SOURCES="$REPO_DIR/src/PPP/* $REPO_DIR/src/Geom/*"

QA_DIR="build/QA"
REBUID_REPO="false"

if [ ! -d ./src ]; then
echo "this script must be run in the repo root dir"
exit 1
fi

# install tool if not yet installed
#sudo apt install similarity-tester cppcheck flawfinder clang-tidy


# must be in the build folder, only for Linux
if [ "REBUID_REPO" = "true" ]; then

    # make a folder for quality assurance log files: code analysis
    if [ -d $QA_DIR ]; then rm -rf $QA_DIR; fi
    mkdir $QA_DIR

    cd build
    cmake .. -DCODE_COVERAGE=ON -DCLANG_TIDY=ON
    make -j4 2>&1 | tee $QA_DIR/clang_tidy_build_results.log

    make doc
    if [ ! -d doxygen ]; then  
        ln -s public ../doxygen
    fi

    make coverage
fi

cd ${REPO_DIR}
cppcheck --language=c++ --std=c++17 --enable=all -DROOT_DIR=${REPO_DIR} -v --xml --xml-version=2 ${SRC_DIR} 2>&1 | tee $QA_DIR/cpp-check.xml

# -R recursively,  but can not control the depth, 
# since third-party code moved into each module, excluding some folder by this way
sim_c++ -a -o "$QA_DIR/sim_cpp.log" ${REPO_DIR}/src/PPP/*.h ${REPO_DIR}/src/Geom/*.h

flawfinder ${MODULE_SOURCES} | tee $QA_DIR/flawfinder.log

mkdir -p site/cppcheck
cppcheck-htmlreport --title="ppp" --file=$QA_DIR/cpp-check.xml --report-dir=site/cppcheck --source-dir=${SRC_DIR}


python3 ${REPO_DIR}/scripts/site_generation.py $QA_DIR/clang_tidy_build_results.log $QA_DIR/flawfinder.log $QA_DIR/sim_cpp.log
echo "visit index.html in this folder"