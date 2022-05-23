# SA

For the analysis with <b>CRAB</b>:
- Calls must be flattened, i.e., every variable must be written and read in the caller.
- No access to member functions of a callee from a caller, e.g., `if Timer.Q then` is not allowed.