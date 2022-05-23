PROGRAM P
VAR_INPUT
    activate : BOOL;
END_VAR
VAR
    b : BOOL;
    y : INT;
END_VAR
    b := TRUE;
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