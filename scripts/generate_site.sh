# tested on Ubuntu 20.04
# this script must be run in the build folder
# github-ci run in project root,  assuming there exist 2 folders: doxygen

MODULES="PPP Geom"
MODULE_SOURCES="../src/PPP/* ../src/Geom/*"
REPO_DIR="../"

if [ ! -d ../src ]; then
echo "this script must be run in the build folder which must be a subfolder of repo root dir"
exit 1
fi

# install tool if not yet installed
#sudo apt install similarity-tester cppcheck flawfinder clang-tidy

# make a folder for quality assurance log files: code analysis
if [ -d QA ]; then rm -rf QA; fi
mkdir QA

# must be in the build folder, only for Linux
cmake .. -DCODE_COVERAGE=ON -DCLANG_TIDY=ON
make -j4 2>&1 | tee QA/clang_tidy_build_results.log

cppcheck --language=c++ --std=c++17 --enable=all -DROOT_DIR=${REPO_DIR} -v --xml --xml-version=2 ${MODULES} 2>&1 | tee QA/cpp-check.xml

# -R recursively,  but can not control the depth, 
# since third-party code moved into each module, remove -R option
sim_c++ -a -o "QA/sim_cpp.log" ${MODULE_SOURCES}

flawfinder ${MODULE_SOURCES} | tee QA/flawfinder.log

mkdir -p site/cppcheck
cppcheck-htmlreport --title="ppp" --file=QA/cpp-check.xml --report-dir=site/cppcheck --source-dir=../

make doc
if [ ! -d public ]; then  
    ln -s ./public ./doxygen
fi

make coverage

python3 ../scripts/site_generation.py QA/clang_tidy_build_results.log QA/flawfinder.log QA/sim_cpp.log
echo "visit index.html in this folder"
