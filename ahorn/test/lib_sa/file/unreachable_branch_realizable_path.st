PROGRAM P
VAR_INPUT
    a : INT;
END_VAR
VAR
    f : Fb;
END_VAR
VAR
    b : BOOL;
END_VAR
VAR_OUTPUT
    c : BOOL;
END_VAR
    IF a >= 32 THEN
        f(x:=a,z=>b);
        IF b THEN
            c := TRUE;
        ELSE
            c := FALSE;
        END_IF;
    ELSE
        f(x:=a,z=>b);
    END_IF;
END_PROGRAM

FUNCTION_BLOCK Fb
VAR_INPUT
    x : INT;
END_VAR
VAR_OUTPUT
    z : BOOL;
END_VAR;
    IF x < 32 THEN
        z := TRUE;
    ELSE
        z := FALSE;
    END_IF;
END_FUNCTION_BLOCK