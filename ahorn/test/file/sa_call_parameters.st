PROGRAM P
VAR_INPUT
    activate : BOOL;
END_VAR
VAR
    k : Fb;
    b : BOOL;
    y : INT;
    z1 : INT;
    z2 : INT;
    z3 : INT;
    z4 : INT;
END_VAR
    b := TRUE;
    k(r:=b AND b AND b,t:=b,q:=b OR TRUE,s:=FALSE,j=>z3,h=>z1,k=>z4,i=>z2);
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
    r : BOOL;
    s : BOOL;
    t : BOOL;
END_VAR
VAR_OUTPUT
    h : INT;
    i : INT;
    j : INT;
    k : INT;
END_VAR
    IF q THEN
        h := 1;
        i := 1;
        j := 1;
        k := 1;
    ELSE
        h := 2;
        i := 2;
        j := 2;
        k := 2;
    END_IF;
END_FUNCTION_BLOCK