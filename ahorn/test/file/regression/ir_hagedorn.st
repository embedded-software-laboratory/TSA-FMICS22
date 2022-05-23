PROGRAM P
VAR_INPUT
    i: INT;
END_VAR

VAR_OUTPUT
    o: INT;
END_VAR

VAR
    x : BOOL := TRUE;
    y : INT := 0;
END_VAR

    IF x = TRUE THEN
        y := 1;
        y := change(3,4);
        y := y+y;
    ELSE
        y := change(2,1);
    END_IF;
    o := change(y,y+1);
END_PROGRAM