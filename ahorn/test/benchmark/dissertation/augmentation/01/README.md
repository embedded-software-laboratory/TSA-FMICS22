## Example for Call Coverage in three and two cycles, respectively
P is the base version of the program which expected branch coverage of 100% is reached after three
cycles due to the dependency of the inner branch in the callee on the local variable.

In Q, the increment in the callee is reconfigured such that branch coverage of 100% can already be
reached after two cycles instead of three.

`S.st` denotes the "shadowed" program, i.e., `P.st` and `Q.st` in one file. 