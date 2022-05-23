PROGRAM P
VAR_INPUT
    x : INT;
    y : INT;
END_VAR
VAR
    a : INT;
    b : BOOL;
END_VAR
    IF x <= 128 THEN
        a := a + 1;
        IF y <= 128 THEN
            IF x <= 64 THEN
                IF x <= 32 THEN
                    a := 1;
                ELSE
                    a := 2;
                END_IF;
            ELSE
                IF x <= 96 THEN
                    a := 3;
                ELSE
                    a := 4;
                END_IF;
                a := a + a;
            END_IF;
        ELSE
            a := 0;
        END_IF;
        a := a + 1;
    ELSE
        a := a;
    END_IF;
    IF a >= 3 THEN
        b := TRUE;
        a := 0;
    ELSE
        b := FALSE;
    END_IF;
END_PROGRAM