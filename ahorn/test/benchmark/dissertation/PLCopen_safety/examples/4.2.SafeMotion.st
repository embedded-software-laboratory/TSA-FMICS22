PROGRAM SafeMotion
VAR_INPUT
    S1SEStopIn : SAFEBOOL;
    S2SMode0 : SAFEBOOL;
    S2SMode1 : SAFEBOOL;
    S4SEnableSwitchCh1 : SAFEBOOL;
    S4SEnableSwitchCh2 : SAFEBOOL;
    S5SGuardSwitch1 : SAFEBOOL;
    S6SGuardSwitch2 : SAFEBOOL;
    S7SGuardLock : SAFEBOOL;
    S0Reset : BOOL;
    S8UnlockGuard : BOOL;
    SFSafeStop11ACK: BOOL;
    SFSafeStop12ACK: BOOL;
    SFSLS1ACK: BOOL;
    SFSLS2ACK: BOOL;
END_VAR
VAR_OUTPUT
    SUnlockGuardK1 : SAFEBOOL;
END_VAR
VAR
    SSafeStopAxis1 : SAFEBOOL;
    SSafeStopAxis2 : SAFEBOOL;
    SSafeMoveAxis1 : SAFEBOOL;
    SSafeMoveAxis2 : SAFEBOOL;
    SSafetyActive : SAFEBOOL;
    SModeAutoActive : SAFEBOOL;
    SModeSetSel : SAFEBOOL;
    SSLSEnable : SAFEBOOL;
    SGuardLocked : SAFEBOOL;
    SFEmergencyStop1 : SFEmergencyStop;
    SFSafeStop11 : SFSafeStop;
    SFSafeStop12 : SFSafeStop;
    SFModeSelector1 : SFModeSelector;
    SFGuardMonitoring1 : SFGuardMonitoring;
    SFGuardLocking1 : SFGuardLocking;
    SFEnableSwitch1 : SFEnableSwitch;
    SFSafelyLimitedSpeed1 : SFSafelyLimitSpeed;
    SFSafelyLimitedSpeed2 : SFSafelyLimitSpeed;
END_VAR

SFEmergencyStop1(Activate:=TRUE, SEStopIn:=S1SEStopIn, SStartReset:=FALSE, SAutoReset:=FALSE, Reset:=S0Reset);
SFSafeStop11(Activate:=TRUE, SStopIn:=SFEmergencyStop1.SEStopOut, AxisID:=1, MonitoringTime:=T#1s, Reset:=S0Reset, Acknowledge:=SFSafeStop11ACK);
SSafeStopAxis1 := SFSafeStop11.SStopped;
SFSafeStop12(Activate:=TRUE, SStopIn:=SFEmergencyStop1.SEStopOut, AxisID:=2, MonitoringTime:=T#1s, Reset:=S0Reset, Acknowledge:=SFSafeStop12ACK);
SSafeStopAxis2 := SFSafeStop12.SStopped;

SFModeSelector1(Activate:=TRUE, SMode0:=S2SMode0, SMode1:=S2SMode1, SMode2:=FALSE, SMode3:=FALSE, SMode4:=FALSE, SMode5:=FALSE, SMode6:=FALSE, SMode7:=FALSE, SUnlock:=TRUE, SSetMode:=FALSE,
 AutoSetMode:=TRUE, ModeMonitorTime:=T#50ms, Reset:=S0Reset);
SModeSetSel := SFModeSelector1.SMode1Sel;
SModeAutoActive := SFModeSelector1.SMode0Sel AND SGuardLocked;

SFGuardMonitoring1(Activate:=TRUE, SGuardSwitch1:=S5SGuardSwitch1, SGuardSwitch2:=S6SGuardSwitch2, DiscrepancyTime:=T#50ms, SStartReset:=FALSE, SAutoReset:=FALSE, Reset:=S0Reset);
SSafetyActive := (SSafeStopAxis1 AND SSafeStopAxis2) OR (SFSafelyLimitedSpeed1.SSafetyActive AND SFSafelyLimitedSpeed2.SSafetyActive);

SFGuardLocking1(Activate:=TRUE, SGuardMonitoring:=SFGuardMonitoring1.SGuardMonitoring, SSafetyActive:=SSafetyActive, SGuardLock:=S7SGuardLock, UnlockRequest:=S8UnlockGuard, SStartReset:=FALSE,
 SAutoReset:=FALSE, Reset:=S0Reset);
SGuardLocked := SFGuardLocking1.SGuardLocked;
SUnlockGuardK1 := SFGuardLocking1.SUnlockGuard;

SFEnableSwitch1(Activate:=TRUE, SSafetyActive:=SSafetyActive AND SModeSetSel, SEnableSwitchCh1:=S4SEnableSwitchCh1, SEnableSwitchCh2:=S4SEnableSwitchCh2, SAutoReset:=FALSE, Reset:=S0Reset);
SSLSEnable := SFEnableSwitch1.SEnableSwitchOut;

SFSafelyLimitedSpeed1(Activate:=TRUE, SOpMode:=SFModeSelector1.SMode0Sel AND SGuardLocked, SEnabled:=SSLSEnable, AxisID:=1, MonitoringTime:=T#1s, Reset:=S0Reset, Acknowledge:=SFSLS1ACK);
SSafeMoveAxis1 := SFSafelyLimitedSpeed1.SSafetyActive;

SFSafelyLimitedSpeed2(Activate:=TRUE, SOpMode:=SFModeSelector1.SMode0Sel AND SGuardLocked, SEnabled:=SSLSEnable, AxisID:=2, MonitoringTime:=T#1s, Reset:=S0Reset, Acknowledge:=SFSLS2ACK);
SSafeMoveAxis2 := SFSafelyLimitedSpeed2.SSafetyActive;
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

FUNCTION_BLOCK SFModeSelector
VAR_INPUT
    Activate : BOOL;
    SMode0 : BOOL;
    SMode1 : BOOL;
    SMode2 : BOOL;
    SMode3 : BOOL;
    SMode4 : BOOL;
    SMode5 : BOOL;
    SMode6 : BOOL;
    SMode7 : BOOL;
    SUnlock : BOOL;
    SSetMode : BOOL;
    AutoSetMode : BOOL;
    ModeMonitorTime : TIME;
    Reset : BOOL;
END_VAR
VAR
    Timer : TON;
    RTRIGatReset: RTRIG;
    RTRIGatSetMode: RTRIG;
    FTRIGatModeSel0: FTRIG;
    RTRIGatModeSel0: RTRIG;
    fallingEdgeDetected : BOOL;
END_VAR
VAR_TEMP
    Error1 : BOOL;
    Error2 : BOOL;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    SMode0Sel : BOOL;
    SMode1Sel : BOOL;
    SMode2Sel : BOOL;
    SMode3Sel : BOOL;
    SMode4Sel : BOOL;
    SMode5Sel : BOOL;
    SMode6Sel : BOOL;
    SMode7Sel : BOOL;
    SAnyModeSel : BOOL;
    Error : BOOL;
    DiagCode : WORD;
END_VAR
RTRIGatReset(CLK:=Reset);
RTRIGatSetMode(CLK:=SSetMode);
IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE
    IF (SMode0 + SMode1 + SMode2 + SMode3 + SMode4 + SMode5 + SMode6 + SMode7 > 1) THEN
        Error1 := true;
    ELSE
        Error1 := false;
    END_IF;
    CASE DiagCode OF
    16#0000:
        IF Activate THEN
            DiagCode := 16#8005;
        END_IF;
    16#8005:
        IF (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7) THEN
            FTRIGatModeSel0(CLK:=false);
            RTRIGatModeSel0(CLK:=false);
        ELSE
            FTRIGatModeSel0(CLK:=true);
            RTRIGatModeSel0(CLK:=true);
        END_IF;
        IF FTRIGatModeSel0.Q THEN
            fallingEdgeDetected := TRUE;
        END_IF;
        IF fallingEdgeDetected THEN
            Timer(IN:=TRUE, PT:=ModeMonitorTime);
        ELSIF RTRIGatModeSel0.Q THEN
            Timer(IN:=FALSE, PT:=ModeMonitorTime);
            fallingEdgeDetected := FALSE;
        END_IF;
        Error2 := Timer.Q;
        IF Error1 THEN
            DiagCode := 16#C001;
        ELSIF Error2 THEN
            DiagCode := 16#C002;
        ELSIF ((AutoSetMode OR RTRIGatSetMode.Q) AND SUnlock AND (SMode0 + SMode1 + SMode2 + SMode3 + SMode4 + SMode5 + SMode6 + SMode7 = 1)) THEN
            DiagCode := 16#8000;
        END_IF;
    16#8000:
        IF Error1 THEN
            DiagCode := 16#C001;
        ELSIF (SMode0 <> SMode0Sel OR SMode1 <> SMode1Sel OR SMode2 <> SMode2Sel OR SMode3 <> SMode3Sel
              OR SMode4 <> SMode4Sel OR SMode5 <> SMode5Sel OR SMode6 <> SMode6Sel OR SMode7 <> SMode7Sel) THEN
            DiagCode := 16#8005;
            fallingEdgeDetected := FALSE;
        ELSIF (NOT SUnlock) THEN
            DiagCode := 16#8004;
        END_IF;
    16#8004:
        IF SUnlock THEN
            DiagCode := 16#8000;
        END_IF;
    16#C001:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C003;
        ELSIF RTRIGatReset.Q THEN
            DiagCode := 16#8005;
            fallingEdgeDetected := FALSE;
        END_IF;
    16#C002:
        IF Reset AND NOT RTRIGatReset.Q THEN
            DiagCode := 16#C004;
        ELSIF RTRIGatReset.Q THEN
            DiagCode := 16#8005;
            fallingEdgeDetected := FALSE;
        END_IF;
    16#C003:
        IF (NOT Reset) THEN
            DiagCode := 16#C001;
        END_IF;
    16#C004:
        IF (NOT Reset) THEN
            DiagCode := 16#C002;
        END_IF;
    END_CASE;
END_IF;

CASE DiagCode OF
16#0000:
    Ready := false;
    SMode0Sel := false;
    SMode1Sel := false;
    SMode2Sel := false;
    SMode3Sel := false;
    SMode4Sel := false;
    SMode5Sel := false;
    SMode6Sel := false;
    SMode7Sel := false;
    SAnyModeSel := false;
    Error := false;
16#8005:
    Ready := true;
    SMode0Sel := false;
    SMode1Sel := false;
    SMode2Sel := false;
    SMode3Sel := false;
    SMode4Sel := false;
    SMode5Sel := false;
    SMode6Sel := false;
    SMode7Sel := false;
    SAnyModeSel := false;
    Error := false;
16#8000:
    Ready := true;
    SMode0Sel := SMode0;
    SMode1Sel := SMode1;
    SMode2Sel := SMode2;
    SMode3Sel := SMode3;
    SMode4Sel := SMode4;
    SMode5Sel := SMode5;
    SMode6Sel := SMode6;
    SMode7Sel := SMode7;
    SAnyModeSel := true;
    Error := false;
16#8004:
    Ready := true;
    SAnyModeSel := true;
    Error := false;
16#C001,
16#C002,
16#C003,
16#C004:
    Ready := true;
    SMode0Sel := false;
    SMode1Sel := false;
    SMode2Sel := false;
    SMode3Sel := false;
    SMode4Sel := false;
    SMode5Sel := false;
    SMode6Sel := false;
    SMode7Sel := false;
    SAnyModeSel := false;
    Error := true;
END_CASE;
IF (DiagCode <> 16#8005) THEN
    Timer(IN:=FALSE, PT:=ModeMonitorTime);
END_IF;
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

FUNCTION_BLOCK SFGuardLocking
VAR_INPUT
    Activate : BOOL;
    SGuardMonitoring : SAFEBOOL;
    SSafetyActive : SAFEBOOL;
    SGuardLock : SAFEBOOL;
    UnlockRequest : BOOL;
    SStartReset : SAFEBOOL;
    SAutoReset : SAFEBOOL;
    Reset : BOOL;
END_VAR
VAR
    RTRIGatReset : RTRIG;
    RTRIGatUnlockRequest : RTRIG;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    SGuardLocked : SAFEBOOL;
    SUnlockGuard : SAFEBOOL;
    Error : BOOL;
    DiagCode : WORD;
END_VAR
RTRIGatReset(CLK:=Reset);
RTRIGatUnlockRequest(CLK:=UnlockRequest);
IF NOT Activate THEN
    DiagCode := 16#0000;
END_IF;
CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8000:
    IF NOT SGuardLock OR NOT SGuardMonitoring THEN
        DiagCode := 16#C004;
    ELSIF RTRIGatUnlockRequest.Q AND SSafetyActive THEN
        DiagCode := 16#8013;
    END_IF;
16#8001:
    IF Reset AND NOT RTRIGatReset.Q AND NOT SStartReset THEN
        DiagCode := 16#C001;
    ELSIF NOT SGuardLock OR NOT SGuardMonitoring THEN
        DiagCode := 16#8011;
    ELSIF (RTRIGatReset.Q OR SStartReset) AND SGuardMonitoring AND SGuardLock THEN
        DiagCode := 16#8000;
    END_IF;
16#8003:
    IF Reset AND NOT RTRIGatReset.Q AND NOT SAutoReset THEN
        DiagCode := 16#C003;
    ELSIF NOT SGuardMonitoring OR NOT SGuardLock THEN
        DiagCode := 16#8011;
    ELSIF RTRIGatReset.Q OR SAutoReset THEN
        DiagCode := 16#8000;
    END_IF;
16#8011:
    IF NOT SSafetyActive AND (NOT SGuardMonitoring OR NOT SGuardLock) THEN
        DiagCode := 16#C004;
    ELSIF (SSafetyActive AND UnlockRequest) OR NOT SGuardMonitoring THEN
        DiagCode := 16#8012;
    ELSIF SGuardMonitoring AND SGuardLock THEN
        DiagCode := 16#8003;
    END_IF;
16#8012:
    IF SGuardMonitoring THEN
        DiagCode := 16#8013;
    ELSIF NOT SSafetyActive AND NOT SGuardMonitoring THEN
        DiagCode := 16#C004;
    END_IF;
16#8013:
    IF NOT SGuardMonitoring THEN
        DiagCode := 16#8012;
    ELSIF NOT UnlockRequest THEN
        DiagCode := 16#8011;
    END_IF;
16#8014:
    IF Reset AND NOT RTRIGatReset.Q THEN
        DiagCode := 16#C002;
    ELSIF NOT SSafetyActive THEN
        DiagCode := 16#C004;
    ELSIF RTRIGatReset.Q THEN
        DiagCode := 16#8011;
    END_IF;
16#C001:
    IF NOT Reset THEN
        DiagCode := 16#8001;
    END_IF;
16#C002:
    IF NOT Reset THEN
        DiagCode := 16#8014;
    END_IF;
16#C003:
    IF NOT Reset THEN
        DiagCode := 16#8003;
    END_IF;
16#C004:
    IF SSafetyActive THEN
        DiagCode := 16#8014;
    END_IF;
END_CASE;
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    SGuardLocked := FALSE;
    SUnlockGuard := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    SGuardLocked := TRUE;
    SUnlockGuard := FALSE;
    Error := FALSE;
16#8001,
16#8003,
16#8011,
16#8014:
    Ready := TRUE;
    SGuardLocked := FALSE;
    SUnlockGuard := FALSE;
    Error := FALSE;
16#8012,
16#8013:
    Ready := TRUE;
    SGuardLocked := FALSE;
    SUnlockGuard := TRUE;
    Error := FALSE;
16#C001,
16#C002,
16#C003,
16#C004:
    Ready := TRUE;
    SGuardLocked := FALSE;
    SUnlockGuard := FALSE;
    Error := TRUE;
END_CASE;
END_FUNCTION_BLOCK

FUNCTION_BLOCK SFEnableSwitch
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
END_FUNCTION_BLOCK

FUNCTION_BLOCK SFSafelyLimitSpeed
VAR_INPUT
    Activate : BOOL;
    SOpMode : SAFEBOOL;
    SEnabled : SAFEBOOL;
    AxisID : INT;
    MonitoringTime : TIME;
    Reset : BOOL;
    Acknowledge : BOOL;      (* This is an internal input *)
END_VAR
VAR
    Timer: TON;
    RTRIGatReset: RTRIG;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    SSafetyActive : SAFEBOOL;
    Error : BOOL;
    DiagCode : WORD;
END_VAR
RTRIGatReset(CLK:=Reset);
IF NOT Activate THEN
    DiagCode := 16#0000;
END_IF;
Timer(IN:=Activate AND NOT SOpMode, PT:=MonitoringTime);
CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8000:
    IF NOT Acknowledge THEN
        DiagCode := 16#C002;
    ELSIF SOpMode THEN
        DiagCode := 16#8012;
    ELSIF SEnabled THEN
        DiagCode := 16#8004;
    END_IF;
16#8001:
    IF Reset AND NOT RTRIGatReset.Q THEN
        DiagCode := 16#C001;
    ELSIF RTRIGatReset.Q AND NOT Acknowledge THEN
        DiagCode := 16#8002;
    ELSIF RTRIGatReset.Q AND Acknowledge THEN
        DiagCode := 16#8012;
    END_IF;
16#8002:
    IF NOT SOpMode THEN
        DiagCode := 16#8003;
    ELSIF Acknowledge THEN
        DiagCode := 16#8012;
    END_IF;
16#8003:
    IF Timer.Q THEN
        DiagCode := 16#C003;
    ELSIF SOpMode THEN
        DiagCode := 16#8012;
    ELSIF Acknowledge AND NOT SEnabled THEN
        DiagCode := 16#8000;
    ELSIF Acknowledge AND SEnabled THEN
        DiagCode := 16#8004;
    END_IF;
16#8004:
    IF NOT Acknowledge THEN
        DiagCode := 16#C002;
    ELSIF SOpMode THEN
        DiagCode := 16#8012;
    ELSIF NOT SEnabled THEN
        DiagCode := 16#8000;
    END_IF;
16#8005:
    IF SOpMode THEN
        DiagCode := 16#8012;
    END_IF;
16#8012:
    IF NOT Acknowledge THEN
        DiagCode := 16#8002;
    ELSIF NOT SOpMode THEN
        DiagCode := 16#8003;
    END_IF;
16#C001:
    IF NOT Reset THEN
        DiagCode := 16#8001;
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
CASE DiagCode OF
16#0000:
    Ready := FALSE;
    SSafetyActive := FALSE;
    Error := FALSE;
16#8000,
16#8004:
    Ready := TRUE;
    SSafetyActive := TRUE;
    Error := FALSE;
16#8001,
16#8002,
16#8003,
16#8005,
16#8012:
    Ready := TRUE;
    SSafetyActive := FALSE;
    Error := FALSE;
16#C001,
16#C002,
16#C003,
16#C004,
16#C005:
    Ready := TRUE;
    SSafetyActive := FALSE;
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