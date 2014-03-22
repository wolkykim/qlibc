Installing qLibc
================

Configure
=========

Configure compile option by executing included "configure" script.

```
$ ./configure
```

By default qLibc will be install on /usr/local/{include,lib}, so use --prefix option if you want to change the installation path.

```
$ ./configure --prefix=/usr/local/qlibc
```

For those who use qLibc in multi-threaded programming model and want qLibc handle thread-safe operation internally by itself.

```
$ ./configure --enable-threadsafe
```

For those who don't want to build the extension library, libqlibcext.

```
$ ./configure --disable-ext
```

For those who want HTTPS support in qhttpclient extension.

```
$ ./configure --with-openssl
```

To see detailed configure options, use --help option.

```
$ ./configure --help
```

Compile
=======

Type make to compile the source codes.

```
$ make
```

Install
=======

This command will install qLibc on your system. By default, the directory prefix is "/usr/local" so it will be install on /usr/local/include and /usr/local/lib.

```
$ make install
```

This will only install header files and library(archive; static and dynamic) files.
The documentations in "doc" directory will not be installed, so make a copy of it by yourself if you want to keep it.

Uninstall
=========

qLibc can be completely removed from the system.

```
$ make uninstall
```

Test
====

Sample sources and API document are provided with the package in the "examples" and "doc" directories.

The examples also can be compiled by typing "make" command in the "examples" directory.

Analyzing the example codes is probably best starting point.
But be aware that the examples were written as simple as it can be just to give you straight-forward ideas, so please refer included API document for more details.
