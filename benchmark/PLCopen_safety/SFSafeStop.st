PROGRAM SFSafeStop
VAR_INPUT
    Activate : BOOL;
    SStopIn : SAFEBOOL;
    AxisID : INT;
    MonitoringTime : TIME;
    Reset : BOOL;
    Acknowledge : BOOL;      (* This is an internal input *)
END_VAR
VAR
    Timer : TON;
    RTRIGatReset : RTRIG;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    SStopped : SAFEBOOL;
    Error : BOOL;
    DiagCode : WORD:=16#0000;
END_VAR
RTRIGatReset(CLK:=Reset);
IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE
    Timer(IN:=Activate AND NOT SStopIn, PT:=MonitoringTime);
    CASE DiagCode OF
    16#0000:
        IF Activate THEN
            DiagCode := 16#8001;
        END_IF;
    16#8000:
        IF NOT Acknowledge THEN
            DiagCode := 16#C002;
        ELSIF SStopIn THEN
            DiagCode := 16#8012;
        END_IF;
    16#8001:
        IF RTRIGatReset.Q AND NOT Acknowledge THEN
            DiagCode := 16#8002;
        ELSIF RTRIGatReset.Q AND Acknowledge THEN
            DiagCode := 16#8012;
        END_IF;
    16#8002:
        IF NOT SStopIn THEN
            DiagCode := 16#8003;
        ELSIF Acknowledge THEN
            DiagCode := 16#8012;
        END_IF;
    16#8003:
        IF Timer.Q THEN
            DiagCode := 16#C003;
        ELSIF SStopIn THEN
            DiagCode := 16#8012;
        ELSIF Acknowledge THEN
            DiagCode := 16#8000;
        END_IF;
    16#8005:
        IF SStopIn THEN
            DiagCode := 16#8012;
        END_IF;
    16#8012:
        IF NOT Acknowledge THEN
            DiagCode := 16#8002;
        ELSIF NOT SStopIn THEN
            DiagCode := 16#8003;
        ELSIF Acknowledge THEN
            DiagCode := 16#8000;
        END_IF;
    16#C002:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C004;
        ELSIF RTRIGatReset.Q THEN
            DiagCode := 16#8005;
        END_IF;
    16#C003:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C005;
        ELSIF RTRIGatReset.Q THEN
            DiagCode := 16#8005;
        END_IF;
    16#C004:
        IF NOT Reset THEN
            DiagCode := 16#C002;
        END_IF;
    16#C005:
        IF NOT Reset THEN
            DiagCode := 16#C003;
        END_IF;
    END_CASE;
END_IF;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    SStopped := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    SStopped := TRUE;
    Error := FALSE;
16#8001,
16#8002,
16#8003,
16#8005,
16#8012:
    Ready := TRUE;
    SStopped := FALSE;
    Error := FALSE;
16#C002,
16#C003,
16#C004,
16#C005:
    Ready := TRUE;
    SStopped := FALSE;
    Error := TRUE;
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