PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR
    f : Fb;
END_VAR
VAR_OUTPUT
    y : INT;
END_VAR
    f(a:=x,b=>y);
    f(a:=x,b=>y);
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