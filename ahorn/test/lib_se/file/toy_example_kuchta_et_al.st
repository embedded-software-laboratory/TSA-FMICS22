PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR
    y : INT;
    z : INT;
END_VAR
VAR_OUTPUT
    e : BOOL;
    r : INT;
END_VAR
    y := x - 1;
    IF y > 7 THEN
        z := x - 8;
        IF z < 4 THEN
            e := TRUE;
        END_IF;
        r := 0;
    ELSE
        r := 1;
    END_IF;
END_PROGRAM
