PROGRAM P
VAR
    a : INT;
    b : INT;
    c : INT;
    d : INT;
END_VAR
    a := 42;
    b := a;
    c := a + b + b + a;
    a := c + 23 + a;
    IF a + b + c > 0 THEN
        a := 12 + 13 + 14 + 15;
    ELSE
        a := 2;
    END_IF;
    IF a >= 1 THEN
        b := b * b * b;
    END_IF;
END_PROGRAM