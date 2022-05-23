PROGRAM P
VAR
    x : INT;
    y : INT := 5;
    z : INT := 3;
END_VAR
    x := x + 1;
    y := x + y - z;
    z := x + y;
    x := z - y;
END_PROGRAM