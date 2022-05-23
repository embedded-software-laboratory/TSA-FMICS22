PROGRAM P
VAR
    x : INT;
    y : INT;
    z : BOOL;
END_VAR
    x := 1 + change(y, y + 1);
    IF (y + 1) = x THEN
        z := TRUE;
    ELSE
        z := FALSE;
    END_IF;
END_PROGRAM