PROGRAM P
VAR_INPUT
    i : INT;
END_VAR
VAR
    x : INT;
    z : INT;
END_VAR
    WHILE i > 0 DO
        IF x = 1 THEN
            z := 0;
        ELSE
            z := 1;
        END_IF;
        i := i - 1;
   END_WHILE;
END_PROGRAM