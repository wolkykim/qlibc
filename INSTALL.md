Installing qLibc
================

qLibc supports POSIX-compliant operating systems, including Linux, macOS,
and most Unix systems. Windows is supported for most features.

## Configure

### Linux and most Unix systems.

Run the `configure` command.

```
$ ./configure
```

By default, qLibc will be installed in `/usr/local/{include,lib}`, so use
the `--prefix` option if you want to change the installation path:

```
$ ./configure --prefix=/usr/local/qlibc
```

For those who don't want to build the extension library `libqlibcext`:

```
$ ./configure --disable-ext
```

If you want HTTPS support in the `qhttpclient` extension:

```
$ ./configure --with-openssl
```

To see detailed configure options, use the `--help` option:

```
$ ./configure --help
```

### macOS and Windows systems (and most Unix systems)

Run the `cmake` command.

```
$ cmake .
```

On many systems, CMake must be installed separately. qLibc requires CMake
version 2.8 or later. Currently, we're focusing on getting the library to
compile on those systems using CMake, so it does not provide all of the
configure options available in the `configure` command, nor does it generate
Makefiles for examples and unit tests.

## Compile

Run `make` in your terminal to compile the source code:

```
$ make
```

## Install

This command will install `qLibc` on your system. By default, the directory
prefix is `/usr/local`, so header files will be installed into
`/usr/local/include` and library files will be installed into
`/usr/local/lib`:

```
$ make install
```

This installs only header files and library files (archive, static, and
dynamic). The documentation files in the `doc` directory are not installed,
so copy them separately if you want to keep them on the system.

## Uninstall

`qLibc` can be removed completely from the system.

```
$ make uninstall
```

## Examples and Unit Tests

Reviewing the example code is probably the best way to get familiar with
qLibc, but please note that the examples were written as simply as possible
to demonstrate typical usage.

The example code and API documents are provided with the package in the
`examples` and `doc` directories.

Please note that the build Makefile is provided only when the package is
configured using the `configure` script. CMake does not generate those
Makefiles.

### Run Examples

The examples can also be compiled by running `make` in the `examples`
directory.

```
$ cd examples
$ make
```

If you are using CMake to configure the qLibc package, it will not generate
Makefiles for the example code.

### Run Unit Tests

Unit tests can be compiled and run as follows:

```
$ cd tests
$ make test
```
