What's qLibc?
=============

The goal of qLibc project is to provide a simple but powerful general purpose C/C++ library which includes all kinds of containers and general library routines. It provides ready-made set of common container APIs with constant look that can be used with any built-in types and with any user-defined types.
qLibc Copyright

qLibc Copyright
===============
All of the deliverable code in qLibc has been dedicated to the public domain by the authors. Anyone is free to copy, modify, publish, use, compile, sell, or distribute the original qLibc code, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.

All of the deliverable code in qLibc has been written from scratch (except of MD5 code published by RSA Data Security, Inc). No code has been taken from other projects or from the open internet. Every line of code can be traced back to its original author. So the qLibc code base is clean and is not contaminated with licensed code from other projects.

Features
========

    General Containers.
        List  Doubly Linked List.
        List Table  KEY/VALUE paired table implemented on linked-list.
        Hash Table  Hash based KEY/VALUE paired table.
        Static Hash Table  Static(array/mmapped/shared) memory based KEY/VALUE paired table.
        Vector  implements a growable array of elements.
        Queue  FIFO(First In First Out) implementation.
        Stack  LIFO(Last In First Out) implementation.
    General utilities.
        String
        I/O
        File
        IPC(Semaphore, Shared-memory)
        En/decoders
        Hashes
        System
        Time
        and more...
    Extensions.
        INI-style Configuration File Parser.
        Apache-style Configuration File Parser.
        Rotating File Logger.
        HTTP client.
        Database(MySQL) interface.
