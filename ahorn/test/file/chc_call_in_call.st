PROGRAM P
VAR
    x : BOOL;
    y : INT;
    z : Fb1;
END_VAR
    z(x:=x,y=>y);
END_PROGRAM

FUNCTION_BLOCK Fb1
VAR_INPUT
    x : BOOL;
END_VAR
VAR
    z : Fb2;
END_VAR
VAR_OUTPUT
    y : INT;
END_VAR
    IF x THEN
        y := 10;
    ELSE
        z(x:=TRUE,y=>y);
    END_IF;
END_FUNCTION_BLOCK

FUNCTION_BLOCK Fb2
VAR_INPUT
    x : BOOL;
END_VAR
VAR_OUTPUT
    y : INT;
END_VAR
    IF x THEN
        y := 50;
    ELSE
        y := 20;
    END_IF;
END_FUNCTION_BLOCK