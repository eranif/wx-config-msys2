# wx-config-msys2

wx-config tool for MSYS2 based installation of wxWidgets using the mingw64 repository

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

