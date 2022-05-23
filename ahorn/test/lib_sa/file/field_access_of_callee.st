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
    f(x:=a,z=>b);
    IF f.z THEN
        c := TRUE;
    ELSE
        c := FALSE;
    END_IF;
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
        y := y;
    ELSE
        y := y + 1;
    END_IF;
    IF y >= 3 THEN
        z := TRUE;
    ELSE
        z := FALSE;
    END_IF;
END_FUNCTION_BLOCK