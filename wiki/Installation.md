# Installing PPP

## Install runtime dependencies

Runtime dependencies (TBB, OpenCascade, etc. - see the compilation guides for each platform) must be installed before PPP binary packages.  Note: If FreeCAD is installed, then this includes the dependencies required by PPP.

## Download (x86_64 architecture) binary packages

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

## Test the installation

Please run the Python tests described in [wiki/Testing.md](wiki/Testing.md) to test your installation.
