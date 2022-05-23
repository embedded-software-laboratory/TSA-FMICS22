PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR
    f : Fb1;
END_VAR
VAR_OUTPUT
    y : INT;
END_VAR
    f(a:=x,b=>y);
END_PROGRAM

FUNCTION_BLOCK Fb1
VAR_INPUT
    a : INT;
END_VAR
VAR
    f : Fb2;
END_VAR
VAR_OUTPUT
    b : INT;
END_VAR;
    IF a >= 32 THEN
        f(c:=x,d=>b);
    ELSE
        b := 16;
    END_IF;
END_FUNCTION_BLOCK

FUNCTION_BLOCK Fb2
VAR_INPUT
    c : INT;
END_VAR
VAR_OUTPUT
    d : INT;
END_VAR;
    IF c >= 32 THEN
        d := 64;
    ELSE
        d := 16;
    END_IF;
END_FUNCTION_BLOCK

