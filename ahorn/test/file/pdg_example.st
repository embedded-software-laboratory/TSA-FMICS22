PROGRAM P
VAR_INPUT
    n_in : INT;
END_VAR
VAR
    n : INT;
    i : INT;
    sum : INT;
    prod : INT;
END_VAR
VAR_OUTPUT
    sum_out : INT;
    prod_out : INT;
END_VAR;
    n := n_input;
    i := 1;
    sum := 0;
    prod := 0;
    WHILE i <= n DO
        sum := sum + i;
        prod := prod * i;
        i := i + 1;
    END_WHILE;
    sum_out := sum;
    prod_out := prod;
END_PROGRAM