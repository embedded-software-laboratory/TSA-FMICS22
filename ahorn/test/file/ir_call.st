PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR_OUTPUT
    y : INT;
END_VAR
VAR
    z : INT;
    f : Fb1;
    g : Fb2;
END_VAR
    f(z:=x,c=>y);
    IF y > 1 THEN
        g(x:=x);
    ELSE
        g(x:=x,b=>y);
    END_IF;
END_PROGRAM

FUNCTION_BLOCK Fb1
VAR_INPUT
    x : INT;
    y : INT;
    z : INT;
END_VAR
VAR_OUTPUT
    a : INT;
    b : INT;
    c : INT;
END_VAR
VAR
    g : Fb2;
END_VAR;
    a := x + 1;
    g(x:=y,y:=z,a=>b);
    IF g.b > 0 THEN
        c := 0;
    ELSE
        c := b + a;
    END_IF;
END_FUNCTION_BLOCK

FUNCTION_BLOCK Fb2
VAR_INPUT
    x : INT;
    y : INT;
END_VAR
VAR_OUTPUT
    a : INT;
    b : INT;
END_VAR
    a := x + y;
    b := 2 * a;
END_FUNCTION_BLOCK