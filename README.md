# wx-config-msys2

`wx-config` tool for MSYS2 based installation of wxWidgets using the mingw64 repository

# About
---

The puropse is tool is to allow easy integration between IDEs and wxWidgets installed via MSYS2 `pacman` from the `mingw64`
repository.

Usually, you will install wxWidgets via `pacman` using commands similar to this:

```bash
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw64/mingw-w64-x86_64-cmake
pacman -S git
pacman -S mingw64/mingw-w64-x86_64-wxmsw3.1
pacman -S mingw64/mingw-w64-x86_64-gdb
```

The above should install:

- mingw64 toolchain (`make`, `gcc`, `g++` etc)
- mingw64 `cmake`
- git (placed under `/usr/bin/git`)
- wxWidgets 3.1 placed under `/mingw64/lib/`
- `gdb`

All the tools placed under `/mingw64/` can be used from a `CMD` terminal and do not require `MSYS2` terminal (e.g. `bash`)
The `wx-config-msys2` is designed to produce build + link lines that are suitable to work with MinGW toolchain

# Build
---

You will need the followings:

- `C++` compiler supporting `c++20` or later installed and configured in the system path
- `cmake` installed and configured in the system path

```bash
mkdir build-debug
cd build-debug
cmake .. -G"MinGW Makefiles"
make -j24
```

# Usage
---

To generate link line:

```batch
wx-config-msys2 --libs --prefix=C:\msys2\mingw64
```

To generate compile line:

```batch
wx-config-msys2 --cflags --prefix=C:\msys2\mingw64
```

Compile with debug:

```batch
wx-config-msys2 --cflags --prefix=C:\msys2\mingw64 --debug
```

