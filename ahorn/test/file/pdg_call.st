PROGRAM P
VAR_INPUT
    activate : BOOL;
END_VAR
VAR
    k : FbB;
    j : FbC;
    b : BOOL;
    y : INT;
    z : INT;
END_VAR
    IF b THEN
        k(q:=b,h=>z);
        j(q:=b,h=>z);
    ELSE
        WHILE y > 0 DO
            j(q:=b,h=>z);
            k(q:=b,h=>z);
        END_WHILE;
    END_IF;
    b := TRUE;
END_PROGRAM

FUNCTION_BLOCK FbB
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

FUNCTION_BLOCK FbC
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