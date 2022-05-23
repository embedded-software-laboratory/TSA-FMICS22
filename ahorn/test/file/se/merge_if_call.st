PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR
    f : Fb;
    y : BOOL;
END_VAR
VAR_OUTPUT
    z : BOOL;
END_VAR
    IF x >= 32 THEN
        f(a:=16,b=>y);
    ELSE
        f(a:=0,b=>y);
    END_IF;
    IF y THEN
        z := TRUE;
    ELSE
        z := FALSE;
    END_IF;
END_PROGRAM

FUNCTION_BLOCK Fb
VAR_INPUT
    a : INT;
END_VAR
VAR_OUTPUT
    b : BOOL;
END_VAR;
    IF a >= 16 THEN
        b := TRUE;
    ELSE
        b := FALSE;
    END_IF;
END_FUNCTION_BLOCK