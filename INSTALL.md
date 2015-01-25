Installing qLibc
================

## Using CMake (Recommended)
The newer build system uses CMake, and can be built like so, assuming you have cloned the source into a folder `qlibc`. Create a build folder for CMake to target so that the source directory can stay clean:
```
git clone git@github.com:wolkykim/qlibc.git
mkdir qlibc-build
cd glibc-build
cmake ../qlibc && make && ctest --output-on-failure
```
This will create static and shared libraries in your source/lib folder as well as generate them in the build directory:
```
lib/libqlibc-static.a
lib/libqlibc.dylib
lib/libqlibcext-static.a
lib/libqlibcext.dylib
```
and it will also run the unit tests using CTest.

## Configure
Configure compile option by executing included "configure" script.

```
$ ./configure
```

By default qLibc will be installed in `/usr/local/{include,lib}`, so use `--prefix` option if you want to change the installation path:

```
$ ./configure --prefix=/usr/local/qlibc
```

For those who don't want to build the extension library `libqlibcext`:

```
$ ./configure --disable-ext
```

For those who want HTTPS support in `qhttpclient` extension:

```
$ ./configure --with-openssl
```

To see detailed configure options, use `--help` option:

```
$ ./configure --help
```

## Compile

Run `make` in your terminal to compile the source code:

```
$ make
```

## Install

This command will install `qLibc` on your system. By default, the directory prefix is "/usr/local" so it will be installed in `/usr/local/include` and `/usr/local/lib`:

```
$ make install
```

This will only install header files and library(archive; static and dynamic) files.
The documentation in the "doc" directory will not be installed, so make a copy of it yourself if you want to keep it.

## Uninstall

`qLibc` can be completely removed from the system.

```
$ make uninstall
```

## Test

Sample source and API documents are provided with the package in the "examples" and "doc" directories.

The examples also can be compiled by typing "make" command in the "examples" directory.

Analyzing the example code is probably best starting point, but be aware that the examples were written as simply as possible just to give you straight-forward ideas. So, please refer included API document for more details.
