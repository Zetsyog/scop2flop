# scop2flop

Compute the FLOP amount in a given C source file.
The program will extract an openscop using Clan
Then for each statement compute how much float operations are performed.
Then it will compute the ehrhart polynomials and evaluate them for given parameters.
And then print to stdout the number of FLOP.

## Build

```bash
git clone
cd scop2flop
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage

```bash
./scop2flop <source-file.c> [PARAM_NAME=VALUE ...]
```
