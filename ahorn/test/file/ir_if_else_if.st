PROGRAM P
VAR
    x : BOOL;
    y : INT;
END_VAR
    IF x > 0 THEN
        y := 1;
    ELSIF x > 1 THEN
        y := 2;
        IF x > 5 THEN
            y := 5;
        ELSIF x > 40 THEN
            y := 20;
        ELSIF x > 10 THEN
            IF x > 10 THEN

            ELSIF x > 1 THEN
                y := 1;
            ELSE

            END_IF;
        ELSE
            y := 1;
        END_IF;
    ELSE
        IF x > 2 THEN
            y := 3;
        ELSE
            y := 4;
        END_IF;
    END_IF;
END_PROGRAM