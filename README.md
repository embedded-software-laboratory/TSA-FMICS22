# TSA-FMICS22
Source code and benchmarks for the FMICS22 submission.

# Evaluation
## Test Suite Generation on PLCopen Safety
```console
$ docker build -f Dockerfile -t fmics22 .
$ docker run fmics22 TSG
```
## Test Suite Augmentation on PPU
```console
$ docker build -f Dockerfile -t fmics22 .
$ docker run fmics22 TSA
```
