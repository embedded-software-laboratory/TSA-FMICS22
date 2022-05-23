PROGRAM P
VAR
    x : INT;
    y : BOOL;
    z : BOOL;
END_VAR
    x := -1;
    y := change(TRUE, FALSE);
    z := NOT(y);
    x := z - y;
END_PROGRAM