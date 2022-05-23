PROGRAM DiagnosticsConcept
VAR_INPUT
    S1SEStopIn1 : SAFEBOOL;
    S1SEStopIn2 : SAFEBOOL;
    S2SESPEIn : SAFEBOOL;
    S0Reset : BOOL;
    S3DriveReset : BOOL;
    AxisD1 : INT;
    InputDevice1active : BOOL;
    InputDevice2active : BOOL;
    InternalAcknowledge: BOOL;
END_VAR
VAR
    SEStopOut : SAFEBOOL;
    SFEquivalent1 : SFEquivalent;
    SFEmergencyStop1 : SFEmergencyStop;
    SFESPE1 : SFESPE;
    SFSafeStop11 : SFSafeStop;
END_VAR
VAR_OUTPUT
    ErrorEquiv1 : BOOL;
    DiagEquiv1 : WORD;
    ErrorEStop1 : BOOL;
    DiagEStop1 : WORD;
    ErrorESPE1 : BOOL;
    DiagESPE1 : WORD;
    ErrorSafeStop1 : BOOL;
    DiagSafeStop1 : WORD;
    SStopped : BOOL;
END_VAR
SFEquivalent1(Activate:=InputDevice1active, SChannelA:=S1SEStopIn1, SChannelB:=S1SEStopIn2, DiscrepancyTime:=T#10ms);
SFEmergencyStop1(Activate:=SFEquivalent1.Ready, SEStopIn:=SFEquivalent1.SEquivalentOut, SStartReset:=FALSE, SAutoReset:=FALSE, Reset:=S0Reset);
SFESPE1(Activate:=InputDevice2active, SESPEIn:=S2SESPEIn, SStartReset:=FALSE, SAutoReset:=FALSE, Reset:=S0Reset);
SFSafeStop11(Activate:=SFEmergencyStop1.Ready AND SFESPE1.Ready, SStopIn:=SEStopOut AND SFESPE1.SESPEOut, AxisID:=AxisD1, MonitoringTime:=T#100ms, Reset:=S3DriveReset, Acknowledge := InternalAcknowledge);
SEStopOut := SFEmergencyStop1.SEStopOut;
ErrorEquiv1 := SFEquivalent1.Error;
DiagEquiv1 := SFEquivalent1.DiagCode;
ErrorEStop1 := SFEmergencyStop1.Error;
DiagEStop1 := SFEmergencyStop1.DiagCode;
ErrorESPE1 := SFESPE1.Error;
DiagESPE1 := SFESPE1.DiagCode;
ErrorSafeStop1 := SFSafeStop11.Error;
DiagSafeStop1 := SFSafeStop11.DiagCode;
SStopped := SFSafeStop11.SStopped;
END_PROGRAM

FUNCTION_BLOCK SFEquivalent
VAR_INPUT
    Activate : BOOL;
    SChannelA : BOOL;
    SChannelB : BOOL;
    DiscrepancyTime : TIME;
END_VAR
VAR
    Timer : TON;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
	SEquivalentOut : BOOL;
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
        IF SChannelA AND NOT SChannelB THEN
            DiagCode := 16#8004;
        ELSIF SChannelB AND NOT SChannelA THEN
            DiagCode := 16#8014;
        ELSIF SChannelA AND SChannelB THEN
            DiagCode := 16#8000;
        END_IF;
    16#8004:
        IF Timer.Q THEN
            DiagCode := 16#C001;
        ELSIF NOT SChannelA THEN
            DiagCode := 16#8001;
        ELSIF SChannelB THEN
            DiagCode := 16#8000;
        END_IF;
    16#C001,
    16#C002:
        IF NOT SChannelA AND NOT SChannelB THEN
            DiagCode := 16#8001;
        END_IF;
    16#8014:
        IF Timer.Q THEN
            DiagCode := 16#C002;
        ELSIF NOT SChannelB THEN
            DiagCode := 16#8001;
        ELSIF SChannelA THEN
            DiagCode := 16#8000;
        END_IF;
    16#C003:
        IF NOT SChannelA AND NOT SChannelB THEN
            DiagCode := 16#8001;
        END_IF;
    16#8005:
        IF Timer.Q THEN
            DiagCode := 16#C003;
        ELSIF NOT SChannelA AND NOT SChannelB THEN
            DiagCode := 16#8001;
        END_IF;
    16#8000:
        IF SChannelA XOR SChannelB THEN
            DiagCode := 16#8005;
        ELSIF NOT SChannelA AND NOT SChannelB THEN
            DiagCode := 16#8001;
        END_IF;
    END_CASE;
END_IF;

CASE DiagCode OF
16#0000:
    Timer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := FALSE;
    Error := FALSE;
    SEquivalentOut:= FALSE;
16#8000:
    Timer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    SEquivalentOut := TRUE;
16#8001:
    Timer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    SEquivalentOut:= FALSE;
16#8004,
16#8014,
16#8005:
	Timer(IN:=TRUE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    SEquivalentOut:= FALSE;
16#C001,
16#C002,
16#C003:
    Timer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := TRUE;
    SEquivalentOut:= FALSE;
END_CASE;
END_FUNCTION_BLOCK

FUNCTION_BLOCK SFEmergencyStop
VAR_INPUT
    Activate : BOOL;
    SEStopIn : BOOL;
    SStartReset : BOOL;
    SAutoReset : BOOL;
    Reset : BOOL;
END_VAR
VAR
    RTRIGatReset : RTRIG;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    SEStopOut : BOOL;
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
    16#8001:
        IF NOT SStartReset THEN
            DiagCode := 16#8002;
        ELSIF SStartReset AND NOT SEStopIn THEN
            DiagCode := 16#8004;
        ELSIF SEStopIn AND SStartReset THEN
            DiagCode := 16#8000;
        END_IF;
    16#8002:
        IF SEStopIn THEN
            DiagCode := 16#8003;
        END_IF;
    16#8003:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C001;
        ELSIF NOT SEStopIn THEN
            DiagCode := 16#8002;
        ELSIF RTRIGatReset.Q THEN
            DiagCode := 16#8000;
        END_IF;
    16#C001:
        IF NOT Reset THEN
            DiagCode := 16#8003;
        END_IF;
    16#8004:
        IF SEStopIn AND NOT SAutoReset THEN
            DiagCode := 16#8005;
        ELSIF SEStopIn AND SAutoReset THEN
            DiagCode := 16#8000;
        END_IF;
    16#8005:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C002;
        ELSIF NOT SEStopIn THEN
            DiagCode := 16#8004;
        ELSIF RTRIGatReset.Q OR SAutoReset THEN
            DiagCode := 16#8000;
        END_IF;
    16#C002:
        IF NOT Reset THEN
            DiagCode := 16#8005;
        END_IF;
    16#8000:
        IF NOT SEStopIn THEN
            DiagCode := 16#8004;
        END_IF;
    END_CASE;
END_IF;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    Error := FALSE;
    SEStopOut := FALSE;
16#8001:
    Ready := TRUE;
    Error := FALSE;
    SEStopOut := FALSE;
16#8002:
    Ready := TRUE;
    Error := FALSE;
    SEStopOut := FALSE;
16#8003:
    Ready := TRUE;
    Error := FALSE;
    SEStopOut := FALSE;
16#C001:
    Ready := TRUE;
    Error := TRUE;
    SEStopOut := FALSE;
16#8004:
    Ready := TRUE;
    Error := FALSE;
    SEStopOut := FALSE;
16#8005:
    Ready := TRUE;
    Error := FALSE;
    SEStopOut := FALSE;
16#C002:
    Ready := TRUE;
    Error := TRUE;
    SEStopOut := FALSE;
16#8000:
    Ready := TRUE;
    Error := FALSE;
    SEStopOut := TRUE;
END_CASE;
END_FUNCTION_BLOCK

FUNCTION_BLOCK SFESPE
VAR_INPUT
    Activate : BOOL;
    SESPEIn : BOOL;
    SStartReset : BOOL;
    SAutoReset : BOOL;
    Reset : BOOL;
END_VAR
VAR
    RTRIGatReset : RTRIG;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    SESPEOut : BOOL;
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
    16#8001:
        IF NOT SStartReset THEN
            DiagCode := 16#8002;
        ELSIF SStartReset AND NOT SESPEIn THEN
            DiagCode := 16#8004;
        ELSIF SESPEIn AND SStartReset THEN
            DiagCode := 16#8000;
        END_IF;
    16#8002:
        IF SESPEIn THEN
            DiagCode := 16#8003;
        END_IF;
    16#8003:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C001;
        ELSIF NOT SESPEIn THEN
            DiagCode := 16#8002;
        ELSIF RTRIGatReset.Q THEN
            DiagCode := 16#8000;
        END_IF;
    16#C001:
        IF NOT Reset THEN
            DiagCode := 16#8003;
        END_IF;
    16#8004:
        IF SESPEIn AND NOT SAutoReset THEN
            DiagCode := 16#8005;
        ELSIF SESPEIn AND SAutoReset THEN
            DiagCode := 16#8000;
        END_IF;
    16#8005:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C002;
        ELSIF NOT SESPEIn THEN
            DiagCode := 16#8004;
        ELSIF RTRIGatReset.Q OR SAutoReset THEN
            DiagCode := 16#8000;
        END_IF;
    16#C002:
        IF NOT Reset THEN
            DiagCode := 16#8005;
        END_IF;
    16#8000:
        IF NOT SESPEIn THEN
            DiagCode := 16#8004;
        END_IF;
    END_CASE;
END_IF;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    Error := FALSE;
    SESPEOut := FALSE;
16#8001:
    Ready := TRUE;
    Error := FALSE;
    SESPEOut := FALSE;
16#8002:
    Ready := TRUE;
    Error := FALSE;
    SESPEOut := FALSE;
16#8003:
    Ready := TRUE;
    Error := FALSE;
    SESPEOut := FALSE;
16#C001:
    Ready := TRUE;
    Error := TRUE;
    SESPEOut := FALSE;
16#8004:
    Ready := TRUE;
    Error := FALSE;
    SESPEOut := FALSE;
16#8005:
    Ready := TRUE;
    Error := FALSE;
    SESPEOut := FALSE;
16#C002:
    Ready := TRUE;
    Error := TRUE;
    SESPEOut := FALSE;
16#8000:
    Ready := TRUE;
    Error := FALSE;
    SESPEOut := TRUE;
END_CASE;
END_FUNCTION_BLOCK

FUNCTION_BLOCK SFSafeStop
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