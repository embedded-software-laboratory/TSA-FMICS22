PROGRAM Muting
VAR_INPUT
    S1SEStopIn : SAFEBOOL;
    S2MutingSwitch11 : BOOL;
    S3MutingSwitch12 : BOOL;
    S4MutingSwitch21 : BOOL;
    S5MutingSwitch22 : BOOL;
    S6SGuardSwitch : SAFEBOOL;
    L1SMutingLamp : SAFEBOOL;
    S7SAOPDIn : SAFEBOOL;
    K1SEDM : SAFEBOOL;
    K2SEDM : SAFEBOOL;
    S9Reset : BOOL;
    S0Reset : BOOL;
    ApplCtrl1 : BOOL;
    StartTestLC1 : BOOL;
    ApplMutingEnable1 : BOOL;
END_VAR
VAR
    SSafeControl : SAFEBOOL := TRUE;
    SFEmergencyStop1 : SFEmergencyStop;
    SFGuardMonitoring1 : SFGuardMonitoring;
    SFMutingSeq1 : SFMutingSeq;
    SFLightCurtain1 : SFTestableSafetySensor;
    SFOutControl1 : SFOutControl;
    SFEDM1 : SFEDM;
END_VAR
VAR_OUTPUT
    SEDMOutK : SAFEBOOL;
    SMutingActiveL1 : SAFEBOOL;
    STestOutLightCurtainS8 : SAFEBOOL;
    ErrorEStop1 : BOOL;
    DiagEStop1 : WORD;
    ErrorGuard1 : BOOL;
    DiagGuard1 : WORD;
    ErrorMuting1 : BOOL;
    DiagMuting1 : WORD;
    ErrorLightCurtain1 : BOOL;
    DiagLightCurtain1 : WORD;
    ErrorOutControl1 : BOOL;
    DiagOutControl1 : WORD;
    ErrorEDM1 : BOOL;
    DiagEDM1 : WORD;
    TestPossibleLC1 : BOOL;
    TestExecutedLC1 : BOOL;
END_VAR
SFEmergencyStop1(Activate:=TRUE, SEStopIn:=S1SEStopIn, SStartReset:=TRUE, SAutoReset:=FALSE, Reset:=S9Reset);
ErrorEStop1 := SFEmergencyStop1.Error;
DiagEStop1 := SFEmergencyStop1.DiagCode;

SFGuardMonitoring1(Activate:=TRUE, SGuardSwitch1:=S6SGuardSwitch, SGuardSwitch2:=S6SGuardSwitch, DiscrepancyTime:=T#0ms, SStartReset:=TRUE, SAutoReset:=FALSE, Reset:=S9Reset);
ErrorGuard1 := SFGuardMonitoring1.Error;
DiagGuard1 := SFGuardMonitoring1.DiagCode;

SFLightCurtain1(Activate:=TRUE, SOSSDIn:=S7SAOPDIn, StartTest:=StartTestLC1, TestTime:=T#100ms, NoExternalTest:=TRUE, SStartReset:=TRUE, SAutoReset:=FALSE, Reset:=S0Reset);
SFMutingSeq1(Activate:=TRUE, SAOPDIn:=SFLightCurtain1.SOSSDOut, MutingSwitch11:=S2MutingSwitch11, MutingSwitch12:=S3MutingSwitch12, MutingSwitch21:=S4MutingSwitch21, MutingSwitch22:=S5MutingSwitch22,
SMutingLamp:=L1SMutingLamp, MaxMutingTime:=T#30s, MutingEnable:=ApplMutingEnable1, SStartReset:=TRUE, Reset:=S9Reset);
STestOutLightCurtainS8 := SFLightCurtain1.STestOut;
TestPossibleLC1 := SFLightCurtain1.TestPossible;
TestExecutedLC1 := SFLightCurtain1.TestExecuted;
ErrorLightCurtain1 := SFLightCurtain1.Error;
DiagLightCurtain1 := SFLightCurtain1.DiagCode;
SMutingActiveL1 := SFMutingSeq1.SMutingActive;
ErrorMuting1 := SFMutingSeq1.Error;
DiagMuting1 := SFMutingSeq1.DiagCode;

SSafeControl := SFEmergencyStop1.SEStopOut AND SFGuardMonitoring1.SGuardMonitoring AND SFMutingSeq1.SAOPDOut;

SFOutControl1(Activate:=TRUE, SSafeControl:=SSafeControl, ProcessControl:=ApplCtrl1, StaticControl:=FALSE, SStartReset:=FALSE, SAutoReset:=FALSE, Reset:=S0Reset);
ErrorOutControl1 := SFOutControl1.Error;
DiagOutControl1 := SFOutControl1.DiagCode;

SFEDM1(Activate:=TRUE, SOutControl:=SFOutControl1.SOutControl, SEDM1:=K1SEDM, SEDM2:=K2SEDM, MonitoringTime:=T#30ms, SStartReset:=FALSE, Reset:=S0Reset);
SEDMOutK := SFEDM1.SEDMOut;
ErrorEDM1 := SFEDM1.Error;
DiagEDM1 := SFEDM1.DiagCode;
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

FUNCTION_BLOCK SFGuardMonitoring
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
END_FUNCTION_BLOCK

FUNCTION_BLOCK SFMutingSeq
VAR_INPUT
    Activate : BOOL := FALSE;
    SAOPDIn : SAFEBOOL := FALSE;
    MutingSwitch11 : BOOL := FALSE;
    MutingSwitch12 : BOOL := FALSE;
    MutingSwitch21 : BOOL := FALSE;
    MutingSwitch22 : BOOL := FALSE;
    SMutingLamp : SAFEBOOL := FALSE;
    DiscTime1112 : TIME := T#0s;
    DiscTime2122 : TIME := T#0s;
    MaxMutingTime : TIME := T#0s;
    MutingEnable : BOOL := FALSE;
    SStartReset : SAFEBOOL := FALSE;
    Reset : BOOL := FALSE;
END_VAR
VAR
    Timer : TON;
    RTRIGatReset : RTRIG;
    RTRIGatMS11 : RTRIG;
    RTRIGatMS12 : RTRIG;
    RTRIGatMS21 : RTRIG;
    RTRIGatMS22 : RTRIG;
    FTRIGatMS11 : FTRIG;
    FTRIGatMS12 : FTRIG;
    FTRIGatMS21 : FTRIG;
    FTRIGatMS22 : FTRIG;
END_VAR
VAR_OUTPUT
    Ready : BOOL := FALSE;
    SAOPDOut : SAFEBOOL := FALSE;
    SMutingActive : SAFEBOOL := FALSE;
    Error : BOOL := FALSE;
    DiagCode : WORD := 16#0000;
END_VAR
RTRIGatReset(CLK:=Reset);
RTRIGatMS11(CLK:=MutingSwitch11);
RTRIGatMS12(CLK:=MutingSwitch12);
RTRIGatMS21(CLK:=MutingSwitch21);
RTRIGatMS22(CLK:=MutingSwitch22);
FTRIGatMS11(CLK:=MutingSwitch11);
FTRIGatMS12(CLK:=MutingSwitch12);
FTRIGatMS21(CLK:=MutingSwitch21);
FTRIGatMS22(CLK:=MutingSwitch22);
IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE
    CASE DiagCode OF
    16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
    16#8000:
        IF (NOT MutingEnable AND RTRIGatMS11.Q) OR (NOT MutingEnable AND RTRIGatMS22.Q) OR (MutingSwitch12 OR MutingSwitch21) OR (MutingSwitch11 AND MutingSwitch22) THEN
            (* Wrong muting sequence *)
            DiagCode := 16#C004;
        ELSIF NOT SAOPDIn THEN
            DiagCode := 16#8002;
        ELSIF NOT SMutingLamp THEN
            DiagCode := 16#C003;
        ELSIF MutingEnable AND (RTRIGatMS11.Q AND NOT MutingSwitch12 AND NOT MutingSwitch21 AND NOT MutingSwitch22) THEN
            DiagCode := 16#8011;
            Timer(IN:=TRUE, PT:=MaxMutingTime);
        ELSIF MutingEnable AND (NOT MutingSwitch11 AND NOT MutingSwitch12 AND NOT MutingSwitch21 AND RTRIGatMS22.Q) THEN
            DiagCode := 16#8122;
            Timer(IN:=TRUE, PT:=MaxMutingTime);
        END_IF;
    16#8001:
        IF Reset AND NOT RTRIGatReset.Q AND NOT SStartReset THEN
            DiagCode := 16#C001;
        (*
        ELSIF (Time parameters out of range)
        DiagCode := 16#C005;
        *)
        ELSIF RTRIGatReset.Q OR SStartReset THEN
            DiagCode := 16#8005;
        END_IF;
    16#8002:
        IF SAOPDIn THEN
            DiagCode := 16#8003;
        END_IF;
    16#8003:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C002;
        ELSIF RTRIGatReset.Q THEN
            DiagCode := 16#8005;
        END_IF;
    16#8005:
        IF NOT SAOPDIn THEN
            DiagCode := 16#8002;
        ELSIF NOT SMutingLamp THEN
            DiagCode := 16#C003;
        ELSIF SAOPDIn THEN
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
    16#C003:
        IF SMutingLamp THEN
            DiagCode := 16#8003;
        END_IF;
    16#C006,
    16#C007,
    16#C008:
        IF NOT (MutingSwitch11 OR MutingSwitch12 OR MutingSwitch21 OR MutingSwitch22) THEN
            DiagCode := 16#8003;
        END_IF;
    ELSE
        IF DiagCode >= 16#C000 THEN
        (* 16#CYx4 *)
            IF NOT (MutingSwitch11 OR MutingSwitch12 OR MutingSwitch21 OR MutingSwitch22) THEN
                DiagCode := 16#8003;
            END_IF;
        ELSE
            (* cases 16#8011 .. 16#8422 *)
            CASE DiagCode OF
            16#8011:
                IF NOT MutingEnable OR NOT MutingSwitch11 OR MutingSwitch21 OR MutingSwitch22 THEN
                    DiagCode := 16#C104;
                ELSIF Timer.Q THEN
                    DiagCode := 16#C006;
                ELSIF NOT SAOPDIn THEN
                    DiagCode := 16#8002;
                ELSIF NOT SMutingLamp THEN
                    DiagCode := 16#C003;
                ELSIF MutingEnable AND (MutingSwitch11 AND RTRIGatMS12.Q AND NOT MutingSwitch21 AND NOT MutingSwitch22) THEN
                    DiagCode := 16#8012;
                END_IF;
            16#8012:
                IF RTRIGatMS11.Q OR RTRIGatMS12.Q OR FTRIGatMS22.Q THEN
                    DiagCode := 16#C104;
                ELSIF Timer.Q THEN
                    DiagCode := 16#C006;
                ELSIF NOT SMutingLamp THEN
                    DiagCode := 16#C003;
                ELSIF NOT MutingSwitch11 AND NOT MutingSwitch12 AND FTRIGatMS21.Q AND MutingSwitch22 THEN
                    Timer(IN:=FALSE, PT:=MaxMutingTime);
                    DiagCode := 16#8000;
                END_IF;
            16#8122:
                IF NOT MutingEnable OR MutingSwitch11 OR MutingSwitch12 OR NOT MutingSwitch22 THEN
                    DiagCode := 16#C104;
                ELSIF Timer.Q THEN
                    DiagCode := 16#C006;
                ELSIF NOT SAOPDIn THEN
                    DiagCode := 16#8002;
                ELSIF NOT SMutingLamp THEN
                    DiagCode := 16#C003;
                ELSIF MutingEnable AND (NOT MutingSwitch11 AND NOT MutingSwitch12 AND RTRIGatMS21.Q AND MutingSwitch22) THEN
                    DiagCode := 16#8112;
                END_IF;
            16#8112:
                IF FTRIGatMS11.Q OR RTRIGatMS21.Q OR RTRIGatMS22.Q THEN
                    DiagCode := 16#C104;
                ELSIF Timer.Q THEN
                    DiagCode := 16#C006;
                ELSIF NOT SMutingLamp THEN
                    DiagCode := 16#C003;
                ELSIF MutingSwitch11 AND FTRIGatMS12.Q AND NOT MutingSwitch21 AND NOT MutingSwitch22 THEN
                    DiagCode := 16#8000;
                    Timer(IN:=FALSE, PT:=MaxMutingTime);
                END_IF;
            END_CASE;
        END_IF;
    END_CASE;
END_IF;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    SAOPDOut := FALSE;
    SMutingActive := FALSE;
    Error := FALSE;
16#8000,
16#8011,
16#8122:
    Ready := TRUE;
    SAOPDOut := TRUE;
    SMutingActive := FALSE;
    Error := FALSE;
16#8001,
16#8002,
16#8003,
16#8005:
    Ready := TRUE;
    SAOPDOut := FALSE;
    SMutingActive := FALSE;
    Error := FALSE;
16#8012,
16#8112:
    Ready := TRUE;
    SAOPDOut := TRUE;
    SMutingActive := TRUE;
    Error := FALSE;
ELSE
    Ready := TRUE;
    SAOPDOut := FALSE;
    SMutingActive := FALSE;
    Error := TRUE;
END_CASE;
END_FUNCTION_BLOCK

FUNCTION_BLOCK SFTestableSafetySensor
VAR_INPUT
    Activate : BOOL;
    SOSSDIn : SAFEBOOL;
    StartTest : BOOL;
    TestTime : TIME := T#10ms;
    NoExternalTest : BOOL;
    SStartReset : SAFEBOOL;
    SAutoReset : SAFEBOOL;
    Reset : BOOL;
END_VAR
VAR
    Timer1: TON;
    Timer2: TON;
    RTRIGatReset : RTRIG;
    RTRIGatStartTest : RTRIG;
    FTRIGatSOSSDIn : FTRIG;
END_VAR
VAR_OUTPUT
    Ready : SAFEBOOL;
    SOSSDOut : SAFEBOOL;
    STestOut : SAFEBOOL := TRUE;
    TestPossible : BOOL;
    TestExecuted : BOOL;
    Error : BOOL;
    DiagCode : WORD := 16#0000;
END_VAR
RTRIGatReset(CLK:=Reset);
RTRIGatStartTest(CLK:=StartTest);
FTRIGatSOSSDIn(CLK:=SOSSDIn);
IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE
    CASE DiagCode OF
    16#0000:
        IF Activate THEN
            DiagCode := 16#8001;
        END_IF;
    16#8000:
        IF NOT SOSSDIn THEN
            DiagCode := 16#8012;
        ELSIF RTRIGatStartTest.Q THEN
            DiagCode := 16#8020;
            Timer1(IN:=FALSE);
        END_IF;
    16#8001:
        IF TestTime > T#150ms THEN
            DiagCode := 16#C000;
        ELSIF Reset AND NOT RTRIGatReset.Q AND NOT SStartReset THEN
            DiagCode := 16#C001;
        ELSIF NOT SOSSDIn THEN
            DiagCode := 16#8002;
        ELSIF SOSSDIn AND (RTRIGatReset.Q OR SStartReset) THEN
            DiagCode := 16#8010;
        END_IF;
    16#8002:
        IF SOSSDIn THEN
            DiagCode := 16#8003;
        END_IF;
    16#8003:
        IF Reset AND NOT RTRIGatReset.Q AND NOT SAutoReset THEN
            DiagCode := 16#C002;
        ELSIF NOT SOSSDIn THEN
            DiagCode := 16#8002;
        ELSIF SOSSDIn AND (RTRIGatReset.Q OR SAutoReset) THEN
            DiagCode := 16#8010;
        END_IF;
    16#8004:
        IF FTRIGatSOSSDIn.Q THEN
            DiagCode := 16#8005;
        END_IF;
    16#8005:
        IF SOSSDIn THEN
            DiagCode := 16#8006;
        END_IF;
    16#8006:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C005;
        ELSIF RTRIGatReset.Q AND NOT SOSSDIn THEN
            DiagCode := 16#8002;
        ELSIF RTRIGatReset.Q AND SOSSDIn THEN
            DiagCode := 16#8010;
        END_IF;
    16#8010:
        IF NOT SOSSDIn THEN
            DiagCode := 16#8002;
        ELSIF RTRIGatStartTest.Q THEN
            DiagCode := 16#8020;
            Timer1(IN:=FALSE);
        END_IF;
    16#8012:
        IF SOSSDIn THEN
            DiagCode := 16#8013;
        END_IF;
    16#8013:
        IF Reset AND NOT RTRIGatReset.Q AND NOT SAutoReset THEN
            DiagCode := 16#C007;
        ELSIF NOT SOSSDIn THEN
            DiagCode := 16#8012;
        ELSIF SOSSDIn AND (RTRIGatReset.Q OR SAutoReset) THEN
            DiagCode := 16#8000;
        END_IF;
    16#8020:
        IF Timer1.Q THEN
            DiagCode := 16#C020;
        ELSIF NOT SOSSDIn THEN
            DiagCode := 16#8030;
            Timer2(IN:=FALSE);
        END_IF;
    16#8030:
        IF Timer2.Q THEN
            DiagCode := 16#C030;
        ELSIF SOSSDIn THEN
            DiagCode := 16#8000;
        END_IF;
    16#C000:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C006;
        ELSIF RTRIGatReset.Q AND TestTime <= T#150ms THEN
            DiagCode := 16#8001;
        END_IF;
    16#C001:
        IF NOT Reset THEN
            DiagCode := 16#8001;
        END_IF;
    16#C002:
        IF NOT Reset THEN
            DiagCode := 16#8003;
        END_IF;
    16#C003:
        IF NOT Reset THEN
            DiagCode := 16#C010;
        END_IF;
    16#C004:
        IF NOT Reset THEN
            DiagCode := 16#C020;
        END_IF;
    16#C005:
        IF NOT Reset THEN
            DiagCode := 16#8006;
        END_IF;
    16#C006:
        IF NOT Reset THEN
            DiagCode := 16#C000;
        END_IF;
    16#C007:
        IF NOT Reset THEN
            DiagCode := 16#8013;
        END_IF;
    16#C010:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C003;
        ELSIF RTRIGatReset.Q AND NOT NoExternalTest THEN
            DiagCode := 16#8004;
        ELSIF SOSSDIn AND (RTRIGatReset.Q AND NoExternalTest) THEN
            DiagCode := 16#8010;
        END_IF;
    16#C020:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C004;
        ELSIF RTRIGatReset.Q AND NOT NoExternalTest THEN
            DiagCode := 16#8004;
        ELSIF SOSSDIn AND (RTRIGatReset.Q AND NoExternalTest) THEN
            DiagCode := 16#8010;
        END_IF;
    END_CASE;
END_IF;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    SOSSDOut := FALSE;
    STestOut := TRUE;
    TestPossible := FALSE;
    TestExecuted := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    SOSSDOut := TRUE;
    STestOut := TRUE;
    TestPossible := TRUE;
    TestExecuted := TRUE;
    Error := FALSE;
16#8001, 16#8002, 16#8003, 16#8004, 16#8005, 16#8006:
    Ready := TRUE;
    SOSSDOut := FALSE;
    STestOut := TRUE;
    TestPossible := FALSE;
    TestExecuted := FALSE;
    Error := FALSE;
16#8010:
    Ready := TRUE;
    SOSSDOut := TRUE;
    STestOut := TRUE;
    TestPossible := TRUE;
    TestExecuted := FALSE;
    Error := FALSE;
16#8012, 16#8013:
    Ready := TRUE;
    SOSSDOut := FALSE;
    STestOut := TRUE;
    TestPossible := FALSE;
    TestExecuted := TRUE;
    Error := FALSE;
16#8020:
    Ready := TRUE;
    SOSSDOut := TRUE;
    STestOut := FALSE;
    TestPossible := FALSE;
    TestExecuted := FALSE;
    Error := FALSE;
    Timer1(IN:=TRUE, PT:=TestTime);
16#8030:
    Ready := TRUE;
    SOSSDOut := TRUE;
    STestOut := TRUE;
    TestPossible := FALSE;
    TestExecuted := FALSE;
    Error := FALSE;
    Timer2(IN:=TRUE, PT:=TestTime);
16#C000, 16#C001, 16#C002, 16#C003, 16#C004, 16#C005, 16#C006, 16#C010, 16#C020:
    Ready := TRUE;
    SOSSDOut := FALSE;
    STestOut := TRUE;
    TestPossible := FALSE;
    TestExecuted := FALSE;
    Error := TRUE;
16#C007:
    Ready := TRUE;
    SOSSDOut := FALSE;
    STestOut := TRUE;
    TestPossible := FALSE;
    TestExecuted := TRUE;
    Error := TRUE;
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

FUNCTION_BLOCK FTRIG
VAR_INPUT
    CLK : BOOL;
END_VAR
VAR
   M : BOOL;
END_VAR
VAR_OUTPUT
   Q : BOOL;
END_VAR
    Q := NOT CLK AND M;
    M := CLK;
END_FUNCTION_BLOCK