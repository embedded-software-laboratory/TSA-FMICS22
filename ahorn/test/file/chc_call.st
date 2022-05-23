PROGRAM P
VAR
    x : BOOL;
    y : INT;
    z : Fb;
END_VAR
    z(x:=x,y=>y);
END_PROGRAM

FUNCTION_BLOCK Fb
VAR_INPUT
    x : BOOL;
END_VAR
VAR_OUTPUT
    y : INT;
END_VAR
    IF x THEN
        y := 10;
    ELSE
        y := 20;
    END_IF;
END_FUNCTION_BLOCK