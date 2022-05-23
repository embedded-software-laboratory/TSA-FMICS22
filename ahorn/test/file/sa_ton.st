PROGRAM P
VAR
    x : INT;
    y : TIME;
    T_1 : TON;
END_VAR
    IF T_1.Q THEN
        x := 0;
    ELSE
        x := 1;
    END_IF;
    T_1(IN:=TRUE,PT:=y);
END_PROGRAM

FUNCTION_BLOCK TON
VAR_INPUT
  IN : BOOL;
  PT : TIME;
END_VAR
VAR
  running : BOOL;
END_VAR
VAR_TEMP
    timeout : BOOL;
END_VAR
VAR_OUTPUT
  Q : BOOL;
  ET : TIME;
END_VAR
  timeout := NONDETERMINISTIC_BOOL();
  IF NOT IN THEN
    Q := FALSE;
    running := FALSE;
  ELSIF NOT running AND NOT Q THEN
    running := TRUE;
  ELSIF running AND NOT Q THEN
    Q := timeout;
    running := NOT Q;
  END_IF;

  ET := NONDETERMINISTIC_TIME();
END_FUNCTION_BLOCK