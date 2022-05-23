PROGRAM P
VAR_INPUT
    activate : BOOL;
END_VAR
VAR
    k : Fb;
    b : BOOL;
    y : INT;
    z : INT;
END_VAR
    b := TRUE;
    k(q:=b,h=>z);
    IF NOT(activate) AND b THEN
        y := 0;
    ELSE
        IF activate OR NOT(b) THEN
            y := 1;
        ELSE
            y := 2;
        END_IF;
    END_IF;
END_PROGRAM

FUNCTION_BLOCK Fb
VAR_INPUT
    q : BOOL;
END_VAR
VAR_OUTPUT
    h : INT;
END_VAR
    IF q THEN
        h := 1;
    ELSE
        h := 2;
    END_IF;
END_FUNCTION_BLOCK