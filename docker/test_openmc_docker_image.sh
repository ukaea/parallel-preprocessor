
# this script will test openmc env
which python
which python3
which pip

which openmc
which mbconvert
which dagmc_merge
which pppGeomPipeline
which occ_faceter

echo "$DISAPLAY"
echo "$PATH"
echo "$OPENMC_CROSS_SECTIONS"

python -c "import openmc"
#python -c "import pymoab"
#python -c "plasma_source"

echo "test sudo by sudo apt"
sudo apt -h

# this is an interactive test, works only for ssh login user
python3 -c "from matplotlib import pyplot as plt;  plt.plot([1, 2]); plt.show()"