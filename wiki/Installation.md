# Installing PPP

## Try parallel-preprocessor in docker

`docker pull qingfengxia/ppp-centos`
This is a small image (size 1.6 GB) based on centos8, with only occt (v7.4) and PPP compiled from source. 

Since binary packages are available for Ubuntu, fedora and Debian, there is no need to provide Dockerfile on each platforms, but only one Docker image based on centos will be maintained.

## Install runtime dependencies

Runtime dependencies (TBB, OpenCascade, etc. - see the compilation guides for each platform) must be installed before PPP binary packages.  In short: **If FreeCAD is installed, then this includes the dependencies required by PPP**


### Install dependencies from repository

1. For Ubuntu 18.04, use FreeCAD PPA to install OCCT
```bash
# assume FreeCAD ppa has been added to your repository list
sudo add-apt-repository ppa:freecad-maintainers/freecad-stable
sudo apt update
sudo install freecad
# replace OCCT version with another version number or just `libocct-ocaf-*` to match any version
sudo apt-get install libtbb2 libocct-foundation-7.3  libocct-data-exchange-7.3  libocct-modeling-data-7.3 libocct-modeling-algorithms-7.3  libocct-ocaf-7.3
```

2. For Ubuntu 20.04 and Debian latest, opencascade 7.3 packages are available. 
```bash
sudo apt-get install libtbb2 libocct-foundation-7.3  libocct-data-exchange-7.3  libocct-modeling-data-7.3 libocct-modeling-algorithms-7.3  libocct-ocaf-7.3
```

3. For fedora 30+, opencascade 7.4 packages are available  
```
yum install opencascade-draw, opencascade-foundation,  opencascade-modeling,  opencascade-ocaf \
    opencascade-visualization freecad -y
```

### Download (x86_64 architecture) binary packages for parallel-preprocessor

Debian packages for Ubuntu and RPM packages for Fedora 30+ are available to download from <https://github.com/ukaea/parallel-preprocessor/releases>.  (These are built automatically when PRs are merged on the main branch.)  For other operating systems, please compile from source.

Package file names follow this pattern: `parallel-preprocessor-<this_software_version>-dev_<OS name>-<OS version>.<package_suffix>`.

**Note: Packages use system-wide python3.**

For Ubuntu and Debian:

- Remove old versions: `sudo apt remove parallel-preprocessor`
- Then install: `sudo dpkg -i parallel-preprocessor*.deb`

For Fedora:

- Remove old versions: `sudo dnf remove parallel-preprocessor`
- Then install: `sudo rpm -ivh parallel-preprocessor*.rpm`

Coming soon: DMG package for MacOS 10.15

Coming later: Conda package for Windows 10

### Test the installation

Please run the Python tests described in [wiki/Testing.md](wiki/Testing.md) to test your installation.
