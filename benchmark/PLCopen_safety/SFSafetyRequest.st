PROGRAM SFSafetyRequest
VAR_INPUT
    Activate : BOOL;
    SOpMode : SAFEBOOL;
    SAcknowledge : SAFEBOOL;
    MonitoringTime : TIME;
    Reset : BOOL;
END_VAR
VAR
    RTRIGatReset : RTRIG;
    Timer : TON;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    SSafetyActive : SAFEBOOL;
    SSafetyRequest : SAFEBOOL;
    Error : BOOL;
    DiagCode : WORD;
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
    16#8000:
        IF NOT SAcknowledge THEN
            DiagCode := 16#C002;
        ELSIF SOpMode THEN
            DiagCode := 16#8012;
        END_IF;
    16#8001:
        IF RTRIGatReset.Q AND NOT SAcknowledge THEN
            DiagCode := 16#8002;
        ELSIF RTRIGatReset.Q AND SAcknowledge THEN
            DiagCode := 16#8012;
        END_IF;
    16#8002:
        IF NOT SOpMode THEN
            DiagCode := 16#8003;
        ELSIF SAcknowledge THEN
            DiagCode := 16#8012;
        END_IF;
    16#8003:
        IF Timer.Q THEN
            DiagCode := 16#C003;
        ELSIF SOpMode THEN
            DiagCode := 16#8012;
        ELSIF SAcknowledge THEN
            DiagCode := 16#8000;
        END_IF;
    16#8005:
        IF SOpMode THEN
            DiagCode := 16#8012;
        END_IF;
    16#8012:
        IF NOT SAcknowledge THEN
            DiagCode := 16#8002;
        ELSIF NOT SOpMode THEN
            DiagCode := 16#8003;
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
IF DiagCode <> 16#8003 THEN 
    Timer(IN:=FALSE, PT:=MonitoringTime);
ELSE
    Timer(IN:=TRUE, PT:=MonitoringTime);
END_IF;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    SSafetyActive := FALSE;
    SSafetyRequest := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    SSafetyActive := TRUE;
    SSafetyRequest := FALSE;
    Error := FALSE;
16#8001:
    Ready := TRUE;
    SSafetyActive := FALSE;
    SSafetyRequest := FALSE;
    Error := FALSE;
16#8002:
    Ready := TRUE;
    SSafetyActive := FALSE;
    SSafetyRequest := TRUE;
    Error := FALSE;
16#8003:
    Ready := TRUE;
    SSafetyActive := FALSE;
    SSafetyRequest := FALSE;
    Error := FALSE;
16#8005:
    Ready := TRUE;
    SSafetyActive := FALSE;
    SSafetyRequest := FALSE;
    Error := FALSE;
16#8012:
    Ready := TRUE;
    SSafetyActive := FALSE;
    SSafetyRequest := TRUE;
    Error := FALSE;
16#C002:
    Ready := TRUE;
    SSafetyActive := FALSE;
    SSafetyRequest := FALSE;
    Error := TRUE;
16#C003:
    Ready := TRUE;
    SSafetyActive := FALSE;
    SSafetyRequest := FALSE;
    Error := TRUE;
16#C004:
    Ready := TRUE;
    SSafetyActive := FALSE;
    SSafetyRequest := FALSE;
    Error := TRUE;
16#C005:
    Ready := TRUE;
    SSafetyActive := FALSE;
    SSafetyRequest := FALSE;
    Error := TRUE;
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