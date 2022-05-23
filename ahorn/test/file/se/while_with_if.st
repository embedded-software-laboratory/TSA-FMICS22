PROGRAM P
VAR_INPUT
    y : INT;
    n : INT;
END_VAR
VAR
    x : INT;
END_VAR
    x := 0;
    WHILE x < n DO
        IF y > 0 THEN
            x := x + y;
        ELSE
            x := x - y;
        END_IF;
        y := -1 * y;
    END_WHILE;
END_PROGRAM