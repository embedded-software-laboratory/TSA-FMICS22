PROGRAM SFOutControl
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