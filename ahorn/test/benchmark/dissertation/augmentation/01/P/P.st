PROGRAM P
VAR_INPUT
    a : INT;
END_VAR
VAR
    f : Fb;
END_VAR
VAR_OUTPUT
    b : BOOL;
END_VAR
    f(x:=a,z=>b);
END_PROGRAM

FUNCTION_BLOCK Fb
VAR_INPUT
    x : INT;
END_VAR
VAR
    y : INT;
END_VAR
VAR_OUTPUT
    z : BOOL;
END_VAR;
    IF x >= 32 THEN
        y := y + 1;
    ELSE
        y := y;
    END_IF;
    IF y >= 3 THEN
        z := TRUE;
        y := 0;
    ELSE
        z := FALSE;
    END_IF;
END_FUNCTION_BLOCK