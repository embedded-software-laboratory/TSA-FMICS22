PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR
    f : Fb1;
    g : Fb2;
    y : BOOL;
END_VAR
VAR_OUTPUT
    z : BOOL;
END_VAR
    IF x >= 32 THEN
        f(a:=16,b=>y);
    ELSE
        g(a:=0,b=>y);
    END_IF;
    IF y THEN
        z := TRUE;
    ELSE
        z := FALSE;
    END_IF;
END_PROGRAM

FUNCTION_BLOCK Fb1
VAR_INPUT
    a : INT;
END_VAR
VAR
    h : Fb3;
END_VAR
VAR_OUTPUT
    b : BOOL;
END_VAR;
    h(a:=a,b=>b);
    IF a >= 16 THEN
        b := TRUE;
    ELSE
        b := FALSE;
    END_IF;
END_FUNCTION_BLOCK

FUNCTION_BLOCK Fb2
VAR_INPUT
    a : INT;
END_VAR
VAR
    h : Fb3;
END_VAR
VAR_OUTPUT
    b : BOOL;
END_VAR;
    h(a:=a,b=>b);
    IF a >= 16 THEN
        b := TRUE;
    ELSE
        b := FALSE;
    END_IF;
END_FUNCTION_BLOCK

FUNCTION_BLOCK Fb3
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
    b := FALSE;
END_FUNCTION_BLOCK