PROGRAM P
VAR
    a : INT;
    b : INT;
    c : INT;
    d : INT;
END_VAR
    a := 42;
    b := a;
    c := a + b;
    a := c + 23;
    c := a + d;
END_PROGRAM