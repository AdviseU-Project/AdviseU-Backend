# AdviseU-Lib-DegreePlanner
Static library for C++ degree planning algorithm

# Gecode Static Libraries
The provided libraries were build on an WSL 2 (x86_64) instance running Ubuntu 22.04.5 LTS.
The s prefix in front of some is used to differentiate these built libraries from the ones installed
by gecode (which don't work for some reason... investigate this). To swap non s prefixed ones over,
just extract their contents and then re-archive them with the s prefix.

# CGO Integration
Use makefile to properly pakage program for cgo

CGO Headers in main.go will look like:
```
// #cgo LDFLAGS: -L${SRCDIR} -lsolver -lsgecodesearch -lsgecodeint -lsgecodekernel -lsgecodesupport -lstdc++
// #include "cpp/solver.h"
```