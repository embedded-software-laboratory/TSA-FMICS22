PROGRAM P
VAR
    x : BOOL;
    y : INT;
END_VAR
    IF x THEN
        y := 1;
    ELSE
        IF x THEN
            y := 2;
        ELSE
            y := 3;
        END_IF;
    END_IF;
END_PROGRAM