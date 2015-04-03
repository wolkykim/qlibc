Installing qLibc
================

qLibc supports POSIX-compliant operating systems, including Linux, MacOS X
and most of Unix systems. Windows OS is supported with most of features.

## Configure

### Linux and most of Unix systems.

Run "configure" command.

```
$ ./configure
```

By default qLibc will be installed in `/usr/local/{include,lib}`, so use
`--prefix` option if you want to change the installation path:

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

### MacOS X and Windows systems (includes most of Unix systems)

Run "cmake" command.

```
$ cmake .
```

In many systems, CMake needs to be installed separately. qlibc requires CMake
version 2.8 or above. Currently, we're focusing on getting the library compiled
on those machines using cmake thus it doesn't provide the full configure
options that are provided in the configure command as well as build makefiles
for examples and unit tests.

## Compile

Run `make` in your terminal to compile the source code:

```
$ make
```

## Install

This command will install `qLibc` on your system. By default, the directory
prefix is "/usr/local" so header files will be installed into `/usr/local/include`
and library files will be installed into `/usr/local/lib`:

```
$ make install
```

This will only install header files and library(archive; static and dynamic)
files. The document files in the "doc" directory will not be installed,
so please make a copy of it by yourself if you want to keep it in the system.

## Uninstall

`qLibc` can be completely removed from the system.

```
$ make uninstall
```

## Examples and Unit Tests.

Analyzing the example code is probably a best starting point to being familiar
with qlibc, but please note that the examples were written as simply
as possible just to show the sample usages.

As a reminder, example codes and API documents are provided with the package
in the "examples" and "doc" directories.

Please note that the build Makefile is only provided when the package is
configured usinig "configure" script. CMake doesn't build the makefiles.

### Run Examples

The examples also can be compiled by running "make" command in the "examples"
directory.

```
$ cd examples
$ make
```

If you're using cmake to configure the qlibc package, it will not generate
Makefile for codes in examples.

### Run Unit Tests

Unit tests can be compiled and run as following.

```
$ cd tests (or in src directory)
$ make test
```
