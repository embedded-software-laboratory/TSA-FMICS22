PROGRAM SFTwoHandControlTypeII
VAR_INPUT
    Activate : BOOL;
    SButton1 : SAFEBOOL;
    SButton2 : SAFEBOOL;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    STwoHandOut : BOOL;
    Error : BOOL;
    DiagCode : WORD;
END_VAR
IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE
    CASE DiagCode OF
    16#0000:
        IF Activate THEN
            DiagCode := 16#8001;
        END_IF;
    16#8001:
        IF SButton1 AND SButton2 THEN
            DiagCode := 16#C003;
        ELSIF SButton1 THEN
            DiagCode := 16#C001;
        ELSIF SButton2 THEN
            DiagCode := 16#C002;
        ELSE
            DiagCode := 16#8004;
        END_IF;
    16#8004:
        IF SButton1 AND SButton2 THEN
            DiagCode := 16#8000;
        ELSIF SButton1 THEN
            DiagCode := 16#8005;
        ELSIF SButton2 THEN
            DiagCode := 16#8006;
        END_IF;
    16#8005:
        IF NOT (SButton1 OR SButton2) THEN
            DiagCode := 16#8004;
        ELSIF NOT SButton1 AND SButton2 THEN
            DiagCode := 16#8019;
        ELSIF SButton1 AND SButton2 THEN
            DiagCode := 16#8000;
        END_IF;
    16#8006:
        IF NOT (SButton1 OR SButton2) THEN
            DiagCode := 16#8004;
        ELSIF SButton1 AND NOT SButton2 THEN
            DiagCode := 16#8019;
        ELSIF SButton1 AND SButton2 THEN
            DiagCode := 16#8000;
        END_IF;
    16#8007:
        IF SButton1 AND SButton2 THEN
            DiagCode := 16#8009;
        ELSIF NOT SButton1 AND SButton2 THEN
            DiagCode := 16#8008;
        ELSIF NOT (SButton1 OR SButton2) THEN
            DiagCode := 16#8004;
        END_IF;
    16#8008:
        IF SButton1 AND SButton2 THEN
            DiagCode := 16#8009;
        ELSIF SButton1 AND NOT SButton2 THEN
            DiagCode := 16#8007;
        ELSIF NOT (SButton1 OR SButton2) THEN
            DiagCode := 16#8004;
        END_IF;
    16#8009:
        IF NOT SButton1 AND SButton2 THEN
            DiagCode := 16#8008;
        ELSIF SButton1 AND NOT SButton2 THEN
            DiagCode := 16#8007;
        ELSIF NOT (SButton1 OR SButton2) THEN
            DiagCode := 16#8004;
        END_IF;
    16#8019:
        IF NOT (SButton1 OR SButton2) THEN
            DiagCode := 16#8004;
        END_IF;
    16#8000:
        IF SButton1 AND NOT SButton2 THEN
            DiagCode := 16#8007;
        ELSIF NOT SButton1 AND SButton2 THEN
            DiagCode := 16#8008;
        ELSIF NOT (SButton1 OR SButton2) THEN
            DiagCode := 16#8004;
        END_IF;
    16#C001,
    16#C002,
    16#C003:
        IF NOT (SButton1 OR SButton2) THEN
            DiagCode := 16#8004;
        END_IF;
    END_CASE;
END_IF;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    Error := FALSE;
    STwoHandOut := FALSE;
16#8000:
    Ready := TRUE;
    Error := FALSE;
    STwoHandOut := TRUE;
16#8001,
16#8004,
16#8005,
16#8006,
16#8007,
16#8008,
16#8009,
16#8019:
    Ready := TRUE;
    Error := FALSE;
    STwoHandOut := FALSE;
16#C001,
16#C002,
16#C003:
    Ready := TRUE;
    Error := TRUE;
    STwoHandOut := FALSE;
END_CASE;
END_PROGRAM
