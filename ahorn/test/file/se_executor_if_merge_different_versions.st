PROGRAM P
VAR_INPUT
    a : INT;
END_VAR
VAR
    b : INT;
    c : INT;
    d : INT;
    e : INT;
    f : INT;
END_VAR
    a := a;
    f := a;
    c := f + b;
    IF c >= 32 THEN
        d := b + 1;
        b := a;
    ELSE
        d := b + e;
        e := c + 1;
    END_IF;
END_PROGRAM