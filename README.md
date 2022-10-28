What's qLibc?  [![Actions Status](https://github.com/wolkykim/qlibc/workflows/CI/badge.svg)](https://github.com/wolkykim/qlibc/actions)
=============

qLibc is currently one of the most functionally-complete, publicly-licensed
C/C++ libraries. The goal of the qLibc project is to provide a **simple and
powerful general purpose C/C++ library** that includes all kinds of containers
and general library routines. It provides a ready-made set of common container
APIs with a consistent API look.

## qLibc Copyright

qLibc is published under 2-clause BSD license known as Simplified BSD License.
Please refer the LICENSE document included in the package for more details.

## API Reference

* [qlibc Core API Reference](https://wolkykim.github.io/qlibc/doc/html/files.html)
  * Containers for Key/Value pairs
    * Tree Table --- in binary tree(left-leaning red-black tree) data structure.
    * Hash Table --- in hash-based data structure.
    * Static Hash Table --- in fixed size memory(array/mmapped/shared).
    * List Table --- in (doubly) linked-list data structure.
  * Containers for Objects
    * List --- Doubly Linked List.
    * Vector --- implements a growable array of elements.
    * Queue --- FIFO(First In First Out) implementation.
    * Stack --- LIFO(Last In First Out) implementation.
  * General utilities.
    * String --- string trimmer, modifier, replacer, case converter, pattern detectors, ...
    * I/O --- non-blocking I/O, stream reader/writer, ...
    * File --- file locking, file/directory hander, path correctors, ...
    * IPC, Semaphore Shared-memory
    * En/decoders --- Url en/decoder, Base64 en/decoder, Hex en/decoder, ...
    * Hashes --- Murmur hases, FNV hases, MD5 hashes, ...
    * Time --- time diff, time format converstion, ...

* [qLibc Extension API Reference](https://wolkykim.github.io/qlibc/doc/html/files.html)
  * Apache-style Configuration File Parser.
  * INI-style Configuration File Parser.
  * HTTP client.
  * Rotating File Logger.
  * Database(MySQL) interface.
  * [Token-Bucket](https://en.wikipedia.org/wiki/Token_bucket)

## qLibc Tables at a Glance

| Characteristics     | Tree Table   | Hash Table   |Static Hash Table| List Table   |
|:--------------------|:------------:|:------------:|:---------------:|:------------:|
| Data structure      | Binary Tree  | Slot Index   | Block Array     | Linked-List  |
| Search complexity   | O(log n)     | O(1) / O(n)  | O(1) / O(n)     | O(n)         |
| Insert complexity   | O(log n)     | O(1) / O(n)  | O(1) / O(n)     | O(1)         |
| Delete complexity   | O(log n)     | O(1) / O(n)  | O(1) / O(n)     | O(n)         |
| Space complexity    | O(n)         | O(n)         | -               | O(n)         |
| Space allocation    | Dynamic      | Dynamic      | Pre-allocation  | Dynamic      |
| Key Stored Sorted   | Yes          | No           | No              | Yes (option) |
| User comparator     | Supported    | -            | -               | Supported    |
| Duplicated keys     | No           | No           | No              | Yes (option) |
| Key stored digested | No           | No           | Yes             | No           |
| Search Nearest Key  | Yes          | No           | No              | No           |
| Iterator support    | Yes          | Yes          | Yes             | Yes          |
| Iterator visit order| min -> max   | random       | random          | insert order |
| Thread-safe option  | Supported    | Suported     | User            | Supported    |
| Can use shared mem  | No           | No           | Yes             | No           |

## Consistent API Look

All container APIs have a consistent look and feel. It basically provides
a creator function which usually returns a pointer to a container structure.
Also, **all functions related to the container can be accessed through function
pointers inside of the container** or traditional style direct access APIs.
For an example, 

So, regardless of which container you use, you can simply put elements into
a list with `container->put(container, ...)` or you can call them using
direct API like qtreetbl_pub(container, ...).

An examples below illustrates how it looks like.

~~~{.c}
  // create a hash-table.
  qhashtbl_t *tbl = qhashtbl(0, QHASHTBL_THREADSAFE);
  
  // add an element which key name is "score".
  int x = 12345;
  tbl->put(tbl, "score", &x, sizeof(int));
  
  // get the value of the element.
  int *px = tbl->get(tbl, "score", NULL, true);
  if(px != NULL) {
    printf("%d\n", *px);
    free(px);
  }
  
  // release table
  tbl->free(tbl);
~~~

Here is an identical implementation with a Linked-List-Table container.
You may notice that there aren't any code changes at all, except for 1 line
in the table creation. This is why qLibc encapsulates corresponding function
pointers inside of the container object.

~~~{.c}
  // create a linked-list-table. THE ONLY LINE YOU NEED TO CHANGE.
  qlisttbl_t *tbl = qlisttbl(QLISTTBL_THREADSAFE);
  
  // add an element which key name is "score".
  int x = 12345;
  tbl->put(tbl, "score", &x, sizeof(int));
  
  // get the value of the element.
  int *px = tbl->get(tbl, "score", NULL, true);
  if(px != NULL) {
    printf("%d\n", *px);             
    free(px);
  }
  
  // release table
  tbl->free(tbl);
~~~

## Looking for people to work with.

We're looking for people who want to work together to develop and improve qLibc.
Currently, we have high demands on following areas.

* Automated testing
* Documentation.
* New feature implementation.

## Contributors

The following people have helped with suggestions, ideas, code or fixing bugs:
(in alphabetical order by first name)

* [Seungyoung "Steve" Kim](https://github.com/wolkykim) - Project Lead
* [Alexandre Lucchesi](https://github.com/alexandrelucchesi)
* [Anthony Tseng](https://github.com/darkdh)
* [Carpentier Pierre-Francois](https://github.com/kakwa)
* Cesar
* [Colin](https://github.com/colintd)
* [Charles](https://github.com/Charles0429)
* [Dmitry Vorobiev](https://github.com/demitsuri)
* [Fabrice Fontaine](https://github.com/ffontaine)
* [Fullaxx](https://github.com/Fullaxx)
* HyoSup Woo
* [Keith Rosenberg](https://github.com/netpoetica)
* Krishna
* [Liu Zhongchao](https://github.com/haveTryTwo)
* Luis Jimenez
* Maik Beckmann
* RQ
* [Ryan Gonzalez](https://github.com/kirbyfan64)
* Umesh

If we have forgotten or misspelled your name, please let us know.
