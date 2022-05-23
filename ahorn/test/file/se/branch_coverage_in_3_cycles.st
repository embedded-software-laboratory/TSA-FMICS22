PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR
    y : INT;
    z : BOOL;
END_VAR
    IF x >= 32 THEN
        y := y + 1;
    ELSE
        y := y;
    END_IF;
    IF y >= 3 THEN
        z := TRUE;
        y := 0;
    ELSE
        z := FALSE;
    END_IF;
END_PROGRAM