# Building toolchain for OSX

### Install ct-ng:

```
brew install crosstool-ng
```

### Install extra dependencies:

```
brew install help2man bison
```

Using **Disk Utility** create APFS(case-sensitive) disk with name xtool-build-env


### Build crosstool-ng toolchain
change variables in `.config` file form this directory if disk name is different:

```
CT_WORK_DIR="/Volumes/xtool-build-env/.build"
CT_PREFIX_DIR="/Volumes/xtool-build-env/${CT_TARGET}"
```

change user limit to build:

```
ulimit -n 1024
```

copy file `.config` from this direcory to `/Volumes/xtool-build-env` :

```
cp .config /Volumes/xtool-build-env
```

build toolchain:
```
cd /Volumes/xtool-build-env
ct-ng build
```

After toolchain built successfully change PATH variable:
```
export PATH=/Volumes/xtool-build-env/aarch64-rpi3-linux-gnu/bin/:$PATH
```

### Building driver

Build driver:
```
cd library
mkdir build
cd build
cmake ..
make
```

Build example:
```
cd library/example
mkdir build
cd build
cmake ..
make
```
