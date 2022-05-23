PROGRAM P
VAR
    w : INT;
    x : INT;
    y : INT;
    z : INT;
END_VAR
    x := 5;
    x := x - 3;
    IF x < 3 THEN
        y := x * 2;
        w := y;
    ELSE
        y := x - 3;
    END_IF;
    w := x - y;
    z := x + y;
END_PROGRAM