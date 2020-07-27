# R-tree

R-tree is a header-only C++ library that implements data structure for storing and testing for collision 2D objects.  
This project consists of:  
- set of headers that include all of the logic
- executable that has no purpose for now but is intended to be used as a visual demonstration of implemented data structure
- tests executable

## Installation

### Requirements

#### Build tools

For Debian based Linux distributions:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake git
```

#### Boost

For Ubuntu 12.04 and later or Debian 7 and later libboost-test-dev has to be installed:
```bash
sudo apt-get update
sudo apt-get install libboost-test-dev
```

### Build

Clone this repo:
```bash
git clone https://github.com/gBaGu/r-tree.git && cd r-tree
```
Generate files required for building and build:
```bash
mkdir build && cd build
cmake ..
make
```
This will output binaries into the `bin/` directory.

#### Debug build

Steps are the same as ones in the previous section. All you need to do is to add **-DCMAKE_BUILD_TYPE=Debug** argument to cmake:
``` bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```
This will output binaries into the `bin/debug/` directory.

#### Release build

Steps are the same as ones in the previous section. All you need to do is to add **-DCMAKE_BUILD_TYPE=Release** argument to cmake:
``` bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```
This will output binaries into the `bin/release/` directory.
