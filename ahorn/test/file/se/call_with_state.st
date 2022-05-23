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
    f(a:=x,c=>y);
END_PROGRAM

FUNCTION_BLOCK Fb
VAR_INPUT
    a : INT;
END_VAR
VAR
    b : INT;
END_VAR
VAR_OUTPUT
    c : INT;
END_VAR;
    IF a >= 32 THEN
        b := b + 1;
    ELSE
        b := b;
    END_IF;
    c := b;
END_FUNCTION_BLOCK