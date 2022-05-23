PROGRAM SFEnableSwitch
VAR_INPUT
    Activate : BOOL;
    SSafetyActive : SAFEBOOL;
    SEnableSwitchCh1 : SAFEBOOL;
    SEnableSwitchCh2 : SAFEBOOL;
    SAutoReset : SAFEBOOL;
    Reset : BOOL;
END_VAR
VAR
    RTRIGatReset : RTRIG;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    SEnableSwitchOut : SAFEBOOL;
    Error : BOOL;
    DiagCode : WORD;
END_VAR
RTRIGatReset(CLK:=Reset);
IF NOT Activate THEN
    DiagCode := 16#0000;
ELSIF NOT SSafetyActive THEN
    DiagCode := 16#8004;
ELSE
    CASE DiagCode OF
    16#0000:
        IF Activate THEN
            DiagCode := 16#8004;
        END_IF;
    16#8000:
        IF (NOT (SEnableSwitchCh1 OR SEnableSwitchCh2)) OR (SEnableSwitchCh1 AND NOT SEnableSwitchCh2) THEN
            DiagCode := 16#8007;
        ELSIF NOT SEnableSwitchCh1 AND SEnableSwitchCh2 THEN
            DiagCode := 16#8006;
        END_IF;
    16#8004:
        IF SSafetyActive THEN
            DiagCode := 16#8005;
        END_IF;
    16#8005:
        IF NOT (NOT SEnableSwitchCh1 AND SEnableSwitchCh2) THEN
            DiagCode := 16#C010;
        ELSIF NOT SEnableSwitchCh1 AND SEnableSwitchCh2 THEN
            DiagCode := 16#8006;
        END_IF;
    16#8006:
        IF SEnableSwitchCh1 AND SEnableSwitchCh2 THEN
            DiagCode := 16#8000;
        ELSIF (NOT (SEnableSwitchCh1 OR SEnableSwitchCh2)) OR (SEnableSwitchCh1 AND NOT SEnableSwitchCh2) THEN
            DiagCode := 16#8007;
        END_IF;
    16#8007:
        IF SEnableSwitchCh1 AND SEnableSwitchCh2 THEN
            DiagCode := 16#C030;
        ELSIF NOT SEnableSwitchCh1 AND SEnableSwitchCh2 THEN
            DiagCode := 16#8006;
        END_IF;
    16#C001:
        IF NOT Reset THEN
            DiagCode := 16#C020;
        END_IF;
    16#C002:
        IF NOT Reset THEN
            DiagCode := 16#C040;
        END_IF;
    16#C010:
        IF NOT SEnableSwitchCh1 AND SEnableSwitchCh2 THEN
            DiagCode := 16#C020;
        END_IF;
    16#C020:
        IF NOT (NOT SEnableSwitchCh1 AND SEnableSwitchCh2) THEN
            DiagCode := 16#C010;
        ELSIF Reset AND NOT RTRIGatReset.Q AND NOT SAutoReset THEN
            DiagCode := 16#C001;
        ELSIF RTRIGatReset.Q OR SAutoReset THEN
            DiagCode := 16#8006;
        END_IF;
    16#C030:
        IF NOT (SEnableSwitchCh1 AND SEnableSwitchCh2) THEN
            DiagCode := 16#C040;
        END_IF;
    16#C040:
        IF SEnableSwitchCh1 AND SEnableSwitchCh2 THEN
            DiagCode := 16#C030;
        ELSIF Reset AND RTRIGatReset.Q AND NOT SAutoReset THEN
            DiagCode := 16#C002;
        ELSIF RTRIGatReset.Q OR SAutoReset THEN
            DiagCode := 16#8007;
        END_IF;
    END_CASE;
END_IF;

CASE DiagCode OF
16#C001,
16#C002,
16#C010,
16#C020,
16#C030,
16#C040:
    Ready := TRUE;
    SEnableSwitchOut := FALSE;
    Error := TRUE;
16#0000:
    Ready := FALSE;
    SEnableSwitchOut := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    SEnableSwitchOut := TRUE;
    Error := FALSE;
16#8004,
16#8005,
16#8006,
16#8007:
    Ready := TRUE;
    SEnableSwitchOut := FALSE;
    Error := FALSE;
END_CASE;
END_PROGRAM

FUNCTION_BLOCK RTRIG
VAR_INPUT
    CLK : BOOL;
END_VAR
VAR
    M : BOOL;
END_VAR
VAR_OUTPUT
    Q : BOOL;
END_VAR
    Q := CLK AND NOT M;
    M := CLK;
END_FUNCTION_BLOCK
