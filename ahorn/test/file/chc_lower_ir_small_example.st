PROGRAM P
VAR
    x : BOOL;
    y : INT;
    z : Fb;
END_VAR
    IF x THEN
        IF x > 0 THEN
            y := 2;
        ELSE
            IF x > 10 THEN
                y := 14;
            ELSE
                z(x:=y);
            END_IF;
        END_IF;
    ELSE
        y := 30;
    END_IF;
END_PROGRAM

FUNCTION_BLOCK Fb
VAR_INPUT
    x : INT;
END_VAR
    x := x + 1;
END_FUNCTION_BLOCK