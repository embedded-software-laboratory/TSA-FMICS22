PROGRAM P
VAR
    f : Fb;
END_VAR
    f();
END_PROGRAM

FUNCTION_BLOCK Fb
VAR_INPUT
    a : BOOL;
    b : INT;
    c : BOOL;
END_VAR
VAR_OUTPUT
    x : INT;
    y : BOOL;
    z : BOOL;
END_VAR
    IF a OR c THEN
        x := b + 1;
        y := a;
    ELSE
        x := 10;
        z := b;
    END_IF;
END_FUNCTION_BLOCK