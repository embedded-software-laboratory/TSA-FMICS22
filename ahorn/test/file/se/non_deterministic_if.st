PROGRAM P
VAR
    x : BOOL;
    y : BOOL;
END_VAR
    x := NONDETERMINISTIC_BOOL();
    IF x THEN
        y := TRUE;
    ELSE
        y := FALSE;
    END_IF;
END_PROGRAM