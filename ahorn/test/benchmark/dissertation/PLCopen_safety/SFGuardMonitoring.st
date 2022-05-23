PROGRAM SFGuardMonitoring
VAR_INPUT
    Activate : BOOL := FALSE;
    SGuardSwitch1 : SAFEBOOL := FALSE;
    SGuardSwitch2 : SAFEBOOL := FALSE;
    DiscrepancyTime : TIME := T#0s;
    SStartReset : SAFEBOOL := FALSE;
    SAutoReset : SAFEBOOL := FALSE;
    Reset : BOOL := FALSE;
END_VAR
VAR
    Timer : TON;
    RTRIGatReset : RTRIG;
END_VAR
VAR_OUTPUT
    Ready : BOOL := FALSE;
    SGuardMonitoring : BOOL := FALSE;
    Error : BOOL := FALSE;
    DiagCode : WORD := 16#0000;
END_VAR
RTRIGatReset(CLK:=Reset);
IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE
    CASE DiagCode OF
    16#0000:
        IF Activate THEN
            DiagCode := 16#8001;
        END_IF;
    16#8001:
        IF NOT SGuardSwitch1 OR NOT SGuardSwitch2 THEN
            DiagCode := 16#8002;
        ELSIF SGuardSwitch1 AND SGuardSwitch2 AND NOT SStartReset THEN
            DiagCode := 16#8003;
        ELSIF SGuardSwitch1 AND SGuardSwitch2 AND SStartReset THEN
            DiagCode := 16#8000;
        END_IF;
    16#8002:
        IF NOT SGuardSwitch1 AND NOT SGuardSwitch2 THEN
            DiagCode := 16#8012;
        END_IF;
    16#8003:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C001;
        ELSIF NOT SGuardSwitch1 OR NOT SGuardSwitch2 THEN
            DiagCode := 16#8002;
        ELSIF RTRIGatReset.Q THEN
            DiagCode := 16#8000;
        END_IF;
    16#8012:
        IF SGuardSwitch1 THEN
            DiagCode := 16#8004;
        ELSIF SGuardSwitch2 THEN
            DiagCode := 16#8014;
        END_IF;
    16#8004:
        IF Timer.Q THEN
            DiagCode := 16#C011;
        ELSIF NOT SGuardSwitch1 THEN
            DiagCode := 16#8002;
        ELSIF SGuardSwitch2 THEN
            DiagCode := 16#8005;
        END_IF;
    16#8014:
        IF Timer.Q THEN
            DiagCode := 16#C012;
        ELSIF NOT SGuardSwitch2 THEN
            DiagCode := 16#8002;
        ELSIF SGuardSwitch1 THEN
            DiagCode := 16#8005;
        END_IF;
    16#8005:
        IF NOT SGuardSwitch1 OR NOT SGuardSwitch2 THEN
            DiagCode := 16#8002;
        ELSIF NOT SAutoReset THEN
            DiagCode := 16#8003;
        ELSIF SAutoReset THEN
            DiagCode := 16#8000;
        END_IF;
    16#8000:
        IF NOT SGuardSwitch1 OR NOT SGuardSwitch2 THEN
            DiagCode := 16#8002;
        END_IF;
    16#C001:
        IF NOT Reset THEN
            DiagCode := 16#8003;
        END_IF;
    16#C011:
        IF NOT SGuardSwitch1 AND NOT SGuardSwitch2 THEN
            DiagCode := 16#8012;
        END_IF;
    16#C012:
        IF NOT SGuardSwitch1 AND NOT SGuardSwitch2 THEN
            DiagCode := 16#8012;
        END_IF;
    END_CASE;
END_IF;
CASE DiagCode OF
16#0000:
    Timer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := FALSE;
    Error := FALSE;
    SGuardMonitoring := FALSE;
16#8000:
    Timer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    SGuardMonitoring  := TRUE;
16#8001,
16#8002,
16#8003,
16#8012,
16#8005:
    Timer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    SGuardMonitoring := FALSE;
16#8004,
16#8014:
    Timer(IN:=TRUE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    SGuardMonitoring := FALSE;
16#C001,
16#C011,
16#C012:
    Timer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := TRUE;
    SGuardMonitoring := FALSE;
END_CASE;
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