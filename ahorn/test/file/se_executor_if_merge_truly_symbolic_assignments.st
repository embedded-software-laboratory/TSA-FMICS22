PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR
    y : INT := 1;
    z : INT := 2;
END_VAR
    IF x >= 32 THEN
        x := x + y;
        y := x + z;
        z := y + 1;
        x := x + z;
        y := x;
    ELSE
        x := x + x;
        z := y + z;
        y := z + 4;
        x := y + z;
        y := x + 1;
        x := y + 1;
        y := y + 1;
        z := z + 2;
        z := z + z;
        x := z + 1;
    END_IF;
END_PROGRAM