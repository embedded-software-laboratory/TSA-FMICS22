PROGRAM P
VAR_INPUT
    x : BOOL;
END_VAR
VAR
    f : Fb;
END_VAR
VAR_OUTPUT
    y : BOOL;
END_VAR
    f(a:=x,c=>y);
END_PROGRAM

FUNCTION_BLOCK Fb
VAR_INPUT
    a : BOOL;
END_VAR
VAR
    b : INT;
END_VAR
VAR_OUTPUT
    c : BOOL;
END_VAR;
    IF a THEN
        b := b + 1;
    ELSE
        b := b;
    END_IF;
    IF b > 3 THEN
        c := TRUE;
        b := 0;
    ELSE
        c := FALSE;
    END_IF;
END_FUNCTION_BLOCK