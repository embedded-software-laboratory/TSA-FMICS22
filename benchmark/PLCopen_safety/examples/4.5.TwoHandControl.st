PROGRAM TwoHandControl
VAR_INPUT
    S1SEStopIn : SAFEBOOL;
    S2SSwitch1 : SAFEBOOL;
    S3SSwitch2 : SAFEBOOL;
    K1SEDM1 : SAFEBOOL;
    K2SEDM2 : SAFEBOOL;
    S0Reset : BOOL;
    Process : BOOL;
END_VAR
VAR
    EStopS1 : SFEmergencyStop;
    OCK1K2 : SFOutControl;
    THCS2S3 : SFTwoHandControlTypeII;
    EDMK1K2 : SFEDM;
END_VAR
VAR_OUTPUT
    SEDMOutEDMK1K2 : SAFEBOOL;
END_VAR

EStopS1(Activate:=TRUE, SEStopIn:=S1SEStopIn, SStartReset:=FALSE, SAutoReset:=FALSE, Reset:=S0Reset);

THCS2S3(Activate:=Process, SButton1:=S2SSwitch1, SButton2:=S3SSwitch2);
OCK1K2(Activate:=TRUE, SSafeControl:=EStopS1.SEStopOut, ProcessControl:=THCS2S3.STwoHandOut, StaticControl:=FALSE, SStartReset:=TRUE,
SAutoReset:=TRUE, Reset:=S0Reset);

EDMK1K2(Activate:=TRUE, SOutControl:=OCK1K2.SOutControl, SEDM1:=K1SEDM1, SEDM2:=K2SEDM2, MonitoringTime:=T#200ms, SStartReset:=FALSE, Reset:=S0Reset);
SEDMOutEDMK1K2 := EDMK1K2.SEDMOut;
END_PROGRAM

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

FUNCTION_BLOCK SFTwoHandControlTypeII
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
END_FUNCTION_BLOCK

FUNCTION_BLOCK SFOutControl
VAR_INPUT
    Activate : BOOL ;
    SSafeControl : SAFEBOOL ;
    ProcessControl : BOOL ;
    StaticControl : BOOL ;
    SStartReset : SAFEBOOL ;
    SAutoReset	: SAFEBOOL ;
    Reset : BOOL ;
END_VAR
VAR
    RTRIGatReset : RTRIG ;
    RTRIGatProcessControl : RTRIG ;
END_VAR
VAR_OUTPUT
    Ready : BOOL ;
    SOutControl : SAFEBOOL ;
    Error : BOOL ;
    DiagCode : WORD:=16#0000 ;
END_VAR
RTRIGatReset(CLK:=Reset);
RTRIGatProcessControl(CLK:=ProcessControl);
IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE
    CASE DiagCode OF
    16#0000:
        IF Activate THEN
            DiagCode := 16#8001;
        END_IF;
    16#8000:
        IF NOT SSafeControl THEN
            DiagCode := 16#8002;
        ELSIF NOT ProcessControl THEN
            DiagCode := 16#8010;
        END_IF;
    16#8001:
        IF Reset AND NOT RTRIGatReset.Q AND NOT SStartReset THEN
            DiagCode := 16#C001;
        ELSIF RTRIGatReset.Q AND RTRIGatProcessControl.Q THEN
            DiagCode := 16#C111;
        ELSIF RTRIGatReset.Q OR SStartReset THEN
            DiagCode := 16#8010;
        END_IF;
    16#8002:
        IF SSafeControl THEN
            DiagCode := 16#8003;
        END_IF;
    16#8003:
        IF NOT SSafeControl THEN
            DiagCode := 16#8002;
        ELSIF Reset AND NOT RTRIGatReset.Q AND NOT SAutoReset THEN
            DiagCode := 16#C002;
        ELSIF RTRIGatReset.Q AND RTRIGatProcessControl.Q THEN
            DiagCode := 16#C211;
        ELSIF RTRIGatReset.Q OR SAutoReset THEN
            DiagCode := 16#8010;
        END_IF;
    16#8010:
        IF NOT SSafeControl THEN
            DiagCode := 16#8002;
        ELSIF NOT RTRIGatProcessControl.Q AND ProcessControl AND NOT StaticControl THEN
            DiagCode := 16#C010;
        ELSIF SSafeControl AND (RTRIGatProcessControl.Q OR (StaticControl AND ProcessControl)) THEN
            DiagCode := 16#8000;
        END_IF;
    16#C001:
        IF NOT Reset THEN
            DiagCode := 16#8001;
        END_IF;
    16#C002:
        IF NOT Reset THEN
            DiagCode := 16#8003;
        END_IF;
    16#C010:
        IF NOT ProcessControl THEN
            DiagCode := 16#8010;
        END_IF;
    16#C111:
        IF NOT Reset THEN
            DiagCode := 16#8001;
        END_IF;
    16#C211:
        IF NOT Reset THEN
            DiagCode := 16#8003;
        END_IF;
    END_CASE;
END_IF;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    SOutControl := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    SOutControl := TRUE;
    Error := FALSE;
16#8001, 16#8002, 16#8003, 16#8010:
    Ready := TRUE;
    SOutControl := FALSE;
    Error := FALSE;
16#C001, 16#C002, 16#C010, 16#C111, 16#C211:
    Ready := TRUE;
    SOutControl := FALSE;
    Error := TRUE;
END_CASE;
END_FUNCTION_BLOCK

FUNCTION_BLOCK SFEDM
VAR_INPUT
    Activate : BOOL ;
    SOutControl	: SAFEBOOL ;
    SEDM1 : SAFEBOOL ;
    SEDM2 : SAFEBOOL ;
    MonitoringTime : TIME;
    SStartReset	: SAFEBOOL ;
    Reset : BOOL ;
END_VAR
VAR
    Timer : TON;
    RTRIGatReset : RTRIG ;
    RTRIGatOutControl : RTRIG ;
    RTRIGatEDM1 : RTRIG ;
    RTRIGatEDM2	: RTRIG ;
END_VAR
VAR_OUTPUT
    Ready : BOOL ;
    SEDMOut	: SAFEBOOL ;
    Error : BOOL ;
    DiagCode : WORD:=16#0000 ;
END_VAR
(* Rising edge detection *)
RTRIGatReset(CLK:=Reset);
RTRIGatOutControl(CLK:=SOutControl);
RTRIGatEDM1(CLK:=SEDM1);
RTRIGatEDM2(CLK:=SEDM2);
IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE
    CASE DiagCode OF
    16#0000:
        IF Activate THEN
            DiagCode := 16#8001;
        END_IF;
    16#8000:
        IF Timer.Q AND SEDM1 AND NOT SEDM2 THEN
            DiagCode := 16#C070;
        ELSIF Timer.Q AND NOT SEDM1 AND SEDM2 THEN
            DiagCode := 16#C080;
        ELSIF Timer.Q AND SEDM1 AND SEDM2 THEN
            DiagCode := 16#C090;
        ELSIF NOT SOutControl THEN
            DiagCode := 16#8010;
            Timer(IN:=FALSE);
        END_IF;
    16#8001:
        IF Reset AND NOT RTRIGatReset.Q AND NOT SStartReset THEN
            DiagCode := 16#C001;
        ELSIF RTRIGatReset.Q AND RTRIGatOutControl.Q AND NOT SStartReset THEN
            DiagCode := 16#C111;
        ELSIF RTRIGatReset.Q OR SStartReset THEN
            DiagCode := 16#8010;
            Timer(IN:=FALSE);
        END_IF;
    16#8010:
        IF SOutControl AND NOT SEDM1 AND SEDM2 THEN
            DiagCode := 16#C010;
        ELSIF SOutControl AND SEDM1 AND NOT SEDM2 THEN
            DiagCode := 16#C020;
        ELSIF SOutControl AND NOT SEDM1 AND NOT SEDM2 THEN
            DiagCode := 16#C030;
        ELSIF Timer.Q AND NOT SEDM1 AND SEDM2 THEN
            DiagCode := 16#C040;
        ELSIF Timer.Q AND SEDM1 AND NOT SEDM2 THEN
            DiagCode := 16#C050;
        ELSIF Timer.Q AND NOT SEDM1 AND NOT SEDM2 THEN
            DiagCode := 16#C060;
        ELSIF SOutControl THEN
            DiagCode := 16#8000;
            Timer(IN:=FALSE);
        END_IF;
    16#C001:
        IF NOT Reset THEN
            DiagCode := 16#8001;
        END_IF;
    16#C010, 16#C020, 16#C030:
        IF (Reset AND NOT RTRIGatReset.Q AND SEDM1 AND SEDM2) OR (RTRIGatReset.Q AND RTRIGatEDM1.Q AND NOT RTRIGatEDM2.Q) THEN
            DiagCode := 16#C011;
        ELSIF (Reset AND NOT RTRIGatReset.Q AND SEDM1 AND SEDM2) OR (RTRIGatReset.Q AND NOT RTRIGatEDM1.Q AND RTRIGatEDM2.Q) THEN
            DiagCode := 16#C021;
        ELSIF (Reset AND NOT RTRIGatReset.Q AND SEDM1 AND SEDM2) OR (RTRIGatReset.Q AND RTRIGatEDM1.Q AND RTRIGatEDM2.Q) THEN
            DiagCode := 16#C031;
        ELSIF RTRIGatReset.Q AND SEDM1 AND SEDM2 THEN
            DiagCode := 16#8010;
            Timer(IN:=FALSE);
        END_IF;
    16#C011:
        IF NOT Reset THEN
            DiagCode := 16#C010;
        END_IF;
    16#C021:
        IF NOT Reset THEN
            DiagCode := 16#C020;
        END_IF;
    16#C031:
        IF NOT Reset THEN
            DiagCode := 16#C030;
        END_IF;
    16#C040, 16#C050, 16#C060:
        IF (Reset AND NOT RTRIGatReset.Q AND SEDM1 AND SEDM2) OR (RTRIGatReset.Q AND RTRIGatEDM1.Q AND NOT RTRIGatEDM2.Q) THEN
            DiagCode := 16#C041;
        ELSIF (Reset AND NOT RTRIGatReset.Q AND SEDM1 AND SEDM2) OR (RTRIGatReset.Q AND NOT RTRIGatEDM1.Q AND RTRIGatEDM2.Q) THEN
            DiagCode := 16#C051;
        ELSIF (Reset AND NOT RTRIGatReset.Q AND SEDM1 AND SEDM2) OR (RTRIGatReset.Q AND RTRIGatEDM1.Q AND RTRIGatEDM2.Q) THEN
            DiagCode := 16#C061;
        ELSIF RTRIGatReset.Q AND SEDM1 AND SEDM2 THEN
            DiagCode := 16#8010;
            Timer(IN:=FALSE);
        END_IF;
    16#C041:
        IF NOT Reset THEN
            DiagCode := 16#C040;
        END_IF;
    16#C051:
        IF NOT Reset THEN
            DiagCode := 16#C050;
        END_IF;
    16#C061:
        IF NOT Reset THEN
            DiagCode := 16#C060;
        END_IF;
    16#C070:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C071;
        ELSIF RTRIGatReset.Q AND NOT (RTRIGatEDM1.Q OR RTRIGatEDM2.Q) THEN
            DiagCode := 16#8010;
            Timer(IN:=FALSE);
        END_IF;
    16#C080:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C081;
        ELSIF RTRIGatReset.Q AND NOT (RTRIGatEDM1.Q OR RTRIGatEDM2.Q) THEN
            DiagCode := 16#8010;
            Timer(IN:=FALSE);
        END_IF;
    16#C090:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C091;
        ELSIF RTRIGatReset.Q AND NOT (RTRIGatEDM1.Q OR RTRIGatEDM2.Q) THEN
            DiagCode := 16#8010;
            Timer(IN:=FALSE);
        END_IF;
    16#C071:
        IF NOT Reset THEN
            DiagCode := 16#C070;
        END_IF;
    16#C081:
        IF NOT Reset THEN
            DiagCode := 16#C080;
        END_IF;
    16#C091:
        IF NOT Reset THEN
            DiagCode := 16#C090;
        END_IF;
    16#C111:
        IF NOT Reset THEN
            DiagCode := 16#8001;
        END_IF;
    END_CASE;
END_IF;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    SEDMOut := FALSE;
    Error := FALSE;
16#8001:
    Ready := TRUE;
    SEDMOut := FALSE;
    Error := FALSE;
16#8010:
    Ready := TRUE;
    SEDMOut := FALSE;
    Error := FALSE;
    Timer(IN:=TRUE, PT:=MonitoringTime);
16#8000:
    Ready := TRUE;
    SEDMOut := TRUE;
    Error := FALSE;
    Timer(IN:=TRUE, PT:=MonitoringTime);
16#C001, 16#C010, 16#C011, 16#C020, 16#C021, 16#C030, 16#C031, 16#C040, 16#C041, 16#C050, 16#C051, 16#C060, 16#C061, 16#C070, 16#C071, 16#C080, 16#C081, 16#C090, 16#C091, 16#C111:
    Ready := TRUE;
    SEDMOut := FALSE;
    Error := TRUE;
END_CASE;
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
