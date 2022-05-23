PROGRAM SFEDM
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