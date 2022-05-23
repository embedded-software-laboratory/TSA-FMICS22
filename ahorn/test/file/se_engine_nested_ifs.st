PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR
    y : BOOL;
END_VAR
IF x > 32 THEN
    y := TRUE;
    IF x > 64 THEN
        y := FALSE;
    ELSE
        y := TRUE;
    END_IF;
ELSE
    y := FALSE;
    IF x < 16 THEN
        y := TRUE;
    ELSE
        y := FALSE;
    END_IF;
END_IF;
END_PROGRAM