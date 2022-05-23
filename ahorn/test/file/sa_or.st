PROGRAM P
VAR_INPUT
    Activate       : BOOL := FALSE;
END_VAR
VAR
    Ready          : BOOL := FALSE;
    S_TwoHandOut   : BOOL := FALSE;
    DiagCode       : WORD := 16#0;
END_VAR
IF NOT Activate THEN
 DiagCode := 16#0000;
ELSE
 CASE DiagCode OF
 16#0000:
     IF Activate THEN
          DiagCode := 16#8001;
     END_IF;
 END_CASE;
END_IF;

CASE DiagCode OF
16#0000:
  Ready := FALSE;
  S_TwoHandOut := FALSE;
16#8000:
  Ready := TRUE;
  S_TwoHandOut := TRUE;
16#8001:
  Ready := TRUE;
  S_TwoHandOut := FALSE;
END_CASE;
END_PROGRAM