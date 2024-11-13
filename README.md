# libbag

A simple, modern, header-only, custom implementation of bundling (zipping) and unbundling (unzipping) library in C++20.
There is no allocation and deallocation in the library as it uses streams and insert iterators to output result.

## Directory structure

These are the directories and its corresponding details.

| Directory | Detail                          |
| --------- | ------------------------------- |
| `code`    | Executables' code.              |
| `library` | the libbag header-only library. |
| `test`    | Test files with Catch2.         |

## Build

```sh
mkdir build.local
cd build.local
cmake ..
cmake --build .
```
