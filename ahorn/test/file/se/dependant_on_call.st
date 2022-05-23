PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR
    f : Fb;
END_VAR
VAR
    y : INT;
END_VAR
VAR_OUTPUT
    z : BOOL;
END_VAR
    f(a:=x,b=>y);
    IF y == 64 THEN
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
    b : INT;
END_VAR;
    IF a >= 32 THEN
        b := 64;
    ELSE
        b := 16;
    END_IF;
END_FUNCTION_BLOCK