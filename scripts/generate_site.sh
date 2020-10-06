# tested on Ubuntu 20.04
# this script must be run in the build folder

MODULES="PPP Geom"
MODULE_SOURCES="../PPP/* ../Geom/*"
REPO_DIR="../"

# install tool if not yet installed
#sudo apt install similarity-tester cppcheck flawfinder clang-tidy

# make a folder for quality assurance log files: code analysis
if [ -d QA ]; then rm -rf QA; fi
mkdir QA

# must be in the build folder, only for Linux
cmake .. -DCODE_COVERAGE=ON -DCLANG_TIDY=ON
make -j4 2>&1 | tee QA/clang_tidy_build_results.log

cppcheck --language=c++ --std=c++17 --enable=all -DROOT_DIR=${REPO_DIR} -v --xml --xml-version=2 ${MODULES} 2>&1 | tee QA/cpp-check.xml

sim_c++ -aR -o "QA/sim_cpp.log" ${MODULE_SOURCES}

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
