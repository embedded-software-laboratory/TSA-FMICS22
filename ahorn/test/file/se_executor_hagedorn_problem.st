Program P
VAR_INPUT
    i : INT;
END_VAR
VAR
    f : Fb1;
    x : INT;
END_VAR
VAR_OUTPUT
    o : INT;
END_VAR
    x := i;
    IF change(FALSE,TRUE) THEN
        f(a:=x,b=>x);
    END_IF;
    o := change(i,x);
END_PROGRAM

FUNCTION_BLOCK Fb1
VAR_INPUT
    a : INT;
END_VAR
VAR_OUTPUT
    b : INT;
END_VAR
    b := a * 2;
END_FUNCTION_BLOCK