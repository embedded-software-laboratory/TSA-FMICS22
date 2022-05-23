# Setup
- Recommended setup is either using an ubuntu machine, mac, or WSL with windows.
- **Personal preference**: CLion IDE with WSL under windows, results in quick builds.
- Clone this repository to any directory (named <repository_path> from here on)
- Execute "git submodule init" and "git submodule update" to pull googletest
- Open CLion, click "Open Project" and open <repository_path>/CMakeLists.txt

# Antlr4
1. Download the ANTLR tool itself from [here](https://www.antlr.org/download/).
2. Add the downloaded ANTLR tool to your PATH environment or put it under `usr/local/lib`.
- Note: The code has to be C++14 due to ANTLR4.

Alternatively, use your package manager to install ANTLR4.

If in any case CMake does not find your ANTLR4, try aiding CMake by adding 

```-DANTLR_EXECUTABLE=/usr/local/lib/antlr-4.9.2-complete.jar```

# Boost
1. Download and install Boost, it is required.
- Note: Use Boost 1.7.2 if you don't care.

# z3
To run algorithms you need z3. Z3 is available for most distributions. For Fedora, install `z3`, `z3-libs` and `z3-devel`.
The packages will be named similarly in other distributions. You can also compile z3
yourself. See <https://github.com/Z3Prover/z3/blob/master/README-CMake.md>.

# graphviz
Visualization of .dot representation as PNG via console `dot -Tpng input.dot -o output.png`.
