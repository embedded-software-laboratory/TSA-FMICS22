NAMESPACE PLCopen

FUNCTION_BLOCK SF_Equivalent
VAR_INPUT
	Activate		: BOOL;
	S_ChannelA		: BOOL;
	S_ChannelB		: BOOL;
	DiscrepancyTime 	: TIME;
END_VAR
VAR_OUTPUT
	Ready		: BOOL;
	S_EquivalentOut	: BOOL;
	Error		: BOOL;
	DiagCode	: WORD;
END_VAR
VAR
 DiscrepancyTimer	: TON;
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
    IF S_ChannelA AND NOT S_ChannelB THEN
	DiagCode := 16#8004;
    ELSIF S_ChannelB AND NOT S_ChannelA THEN
	DiagCode := 16#8014;
    ELSIF S_ChannelA AND S_ChannelB THEN
	DiagCode := 16#8000;
    END_IF;
16#8004:
    IF DiscrepancyTimer.Q THEN
	DiagCode := 16#C001;
    ELSIF NOT S_ChannelA THEN
	DiagCode := 16#8001;
    ELSIF S_ChannelB THEN
	DiagCode := 16#8000;
    END_IF;
16#C001,
16#C002:
    IF NOT S_ChannelA AND NOT S_ChannelB THEN
	DiagCode := 16#8001;
    END_IF;
16#8014:
    IF DiscrepancyTimer.Q THEN
	DiagCode := 16#C002;
    ELSIF NOT S_ChannelB THEN
	DiagCode := 16#8001;
    ELSIF S_ChannelA THEN
	DiagCode := 16#8000;
    END_IF;
16#C003:
    IF NOT S_ChannelA AND NOT S_ChannelB THEN
	DiagCode := 16#8001;
    END_IF;
16#8005:
    IF DiscrepancyTimer.Q THEN
	DiagCode := 16#C003;
    ELSIF NOT S_ChannelA AND NOT S_ChannelB THEN
	DiagCode := 16#8001;
    END_IF;
16#8000:
    IF S_ChannelA XOR S_ChannelB THEN
	DiagCode := 16#8005;
    ELSIF NOT S_ChannelA AND NOT S_ChannelB THEN
	DiagCode := 16#8001;
    END_IF;
END_CASE;

END_IF;

CASE DiagCode OF
16#0000:
    DiscrepancyTimer(IN:=0, PT:=DiscrepancyTime);
    Ready := FALSE;
    Error := FALSE;
    S_EquivalentOut:= FALSE;
16#8000:
    DiscrepancyTimer(IN:=0, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    S_EquivalentOut := TRUE;
16#8001:
    DiscrepancyTimer(IN:=0, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    S_EquivalentOut:= FALSE;
16#8004,
16#8014,
16#8005:
	DiscrepancyTimer(IN:=1, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    S_EquivalentOut:= FALSE;
16#C001,
16#C002,
16#C003:
    DiscrepancyTimer(IN:=0, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := TRUE;
    S_EquivalentOut:= FALSE;
END_CASE;

END_FUNCTION_BLOCK

FUNCTION_BLOCK SF_Antivalent
 VAR_INPUT
        Activate           : BOOL;
        S_ChannelNC        : SAFEBOOL;
        S_ChannelNO        : SAFEBOOL;
        DiscrepancyTime    : TIME;
 END_VAR
 VAR_OUTPUT
        Ready              : BOOL;
        S_AntivalentOut    : BOOL;
        Error              : BOOL;
        DiagCode           : WORD;
 END_VAR
 VAR
        T_1        : TON;
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
    IF S_ChannelNC AND S_ChannelNO THEN
        DiagCode := 16#8004;
    ELSIF NOT S_ChannelNC AND NOT S_ChannelNO THEN
        DiagCode := 16#8014;
    ELSIF S_ChannelNC AND NOT S_ChannelNO THEN
        DiagCode := 16#8000;
    END_IF;
16#8004:
    IF T_1.Q THEN
        DiagCode := 16#C001;
    ELSIF NOT S_ChannelNC THEN
        DiagCode := 16#8001;
    ELSIF NOT S_ChannelNO THEN
        DiagCode := 16#8000;
    END_IF;
16#C001,
16#C002:
    IF NOT S_ChannelNC AND S_ChannelNO THEN
        DiagCode := 16#8001;
    END_IF;
16#8014:
    IF T_1.Q THEN
        DiagCode := 16#C002;
    ELSIF S_ChannelNO THEN
        DiagCode := 16#8001;
    ELSIF S_ChannelNC THEN
        DiagCode := 16#8000;
    END_IF;
16#C003:
    IF NOT S_ChannelNC AND S_ChannelNO THEN
        DiagCode := 16#8001;
    END_IF;
16#8005:
    IF T_1.Q THEN
        DiagCode := 16#C003;
    ELSIF NOT S_ChannelNC AND S_ChannelNO THEN
        DiagCode := 16#8001;
    END_IF;
16#8000:
    IF NOT S_ChannelNC OR S_ChannelNO THEN
        DiagCode := 16#8005;
    ELSIF NOT S_ChannelNC AND S_ChannelNO THEN
        DiagCode := 16#8001;
    END_IF;
END_CASE;
END_IF;

CASE DiagCode OF
16#0000:
    T_1(IN:=0, PT:=DiscrepancyTime);
    Ready := FALSE;
    Error := FALSE;
    S_AntivalentOut:= FALSE;
16#8000:
    T_1(IN:=0, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    S_AntivalentOut := TRUE;
16#8001:
    T_1(IN:=0, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    S_AntivalentOut:= FALSE;
16#8004,
16#8014,
16#8005:
    T_1(IN:=1, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    S_AntivalentOut:= FALSE;
16#C001,
16#C002,
16#C003:
    T_1(IN:=0, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := TRUE;
    S_AntivalentOut:= FALSE;
END_CASE;

END_FUNCTION_BLOCK

FUNCTION_BLOCK SF_ModeSelector
VAR_INPUT
        Activate        : BOOL;
        S_Mode0         : BOOL;
        S_Mode1         : BOOL;
        S_Mode2         : BOOL;
        S_Mode3         : BOOL;
        S_Mode4         : BOOL;
        S_Mode5         : BOOL;
        S_Mode6         : BOOL;
        S_Mode7         : BOOL;
        S_Unlock        : BOOL;
        S_SetMode       : BOOL;
        AutoSetMode     : BOOL;
        ModeMonitorTime : TIME; 
        Reset           : BOOL;
 END_VAR
 VAR_OUTPUT
        Ready           : BOOL;
        S_Mode0Sel      : BOOL;
        S_Mode1Sel      : BOOL;
        S_Mode2Sel      : BOOL;
        S_Mode3Sel      : BOOL;
        S_Mode4Sel      : BOOL;
        S_Mode5Sel      : BOOL;
        S_Mode6Sel      : BOOL;
        S_Mode7Sel      : BOOL;
        S_AnyModeSel    : BOOL;
        Error           : BOOL;
        DiagCode        : WORD;
 END_VAR
 VAR
        T_1        : TON;                 (* Timer for S2, transition T27 *)
 END_VAR

 VAR
        R_TRIGatReset: R_TRIG;
        R_TRIGatSetMode: R_TRIG;
        F_TRIGatModeSel0: F_TRIG;
        R_TRIGatModeSel0: R_TRIG;
        fallingEdgeDetected : BOOL;
 END_VAR

 VAR_TEMP
    Error1 : BOOL;
    Error2 : BOOL;
 END_VAR
 
R_TRIGatReset(CLK:=Reset);
R_TRIGatSetMode(CLK:=S_SetMode);

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE

IF (S_Mode0 + S_Mode1 + S_Mode2 + S_Mode3 + S_Mode4 + S_Mode5 + S_Mode6 + S_Mode7 > 1) THEN
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
    IF (NOT S_Mode0 AND NOT S_Mode1 AND NOT S_Mode2 AND NOT S_Mode3 AND NOT S_Mode4 AND NOT S_Mode5 AND NOT S_Mode6 AND NOT S_Mode7) THEN
        F_TRIGatModeSel0(CLK:=false);
        R_TRIGatModeSel0(CLK:=false);
    ELSE
        F_TRIGatModeSel0(CLK:=true);
        R_TRIGatModeSel0(CLK:=true);
    END_IF;
    
    IF F_TRIGatModeSel0.Q THEN
        fallingEdgeDetected := TRUE;
    END_IF;

    IF fallingEdgeDetected THEN
        T_1(IN:=true, PT:=ModeMonitorTime);
    ELSIF R_TRIGatModeSel0.Q THEN
        T_1(IN:=false, PT:=ModeMonitorTime);
        fallingEdgeDetected := FALSE;
    END_IF;

    Error2 := T_1.Q;

    IF Error1 THEN
        DiagCode := 16#C001;
    ELSIF Error2 THEN
        DiagCode := 16#C002;
    ELSIF ((AutoSetMode OR R_TRIGatSetMode.Q) AND S_Unlock AND (S_Mode0 + S_Mode1 + S_Mode2 + S_Mode3 + S_Mode4 + S_Mode5 + S_Mode6 + S_Mode7 = 1)) THEN
        DiagCode := 16#8000;
    END_IF;
16#8000:
    IF Error1 THEN
        DiagCode := 16#C001;
    ELSIF (S_Mode0 <> S_Mode0Sel OR S_Mode1 <> S_Mode1Sel OR S_Mode2 <> S_Mode2Sel OR S_Mode3 <> S_Mode3Sel
          OR S_Mode4 <> S_Mode4Sel OR S_Mode5 <> S_Mode5Sel OR S_Mode6 <> S_Mode6Sel OR S_Mode7 <> S_Mode7Sel) THEN
        DiagCode := 16#8005;
        fallingEdgeDetected := FALSE;
    ELSIF (NOT S_Unlock) THEN
        DiagCode := 16#8004;
    END_IF;
16#8004:
    IF S_Unlock THEN
        DiagCode := 16#8000;
    END_IF;
16#C001:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C003;
    ELSIF R_TRIGatReset.Q THEN
        DiagCode := 16#8005;
        fallingEdgeDetected := FALSE;
    END_IF;
16#C002:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C004;
    ELSIF R_TRIGatReset.Q THEN
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
    S_Mode0Sel := false;
    S_Mode1Sel := false;
    S_Mode2Sel := false;
    S_Mode3Sel := false;
    S_Mode4Sel := false;
    S_Mode5Sel := false;
    S_Mode6Sel := false;
    S_Mode7Sel := false;
    S_AnyModeSel := false;
    Error := false;
16#8005:
    Ready := true;
    S_Mode0Sel := false;
    S_Mode1Sel := false;
    S_Mode2Sel := false;
    S_Mode3Sel := false;
    S_Mode4Sel := false;
    S_Mode5Sel := false;
    S_Mode6Sel := false;
    S_Mode7Sel := false;
    S_AnyModeSel := false;
    Error := false;
16#8000:
    Ready := true;
    S_Mode0Sel := S_Mode0;
    S_Mode1Sel := S_Mode1;
    S_Mode2Sel := S_Mode2;
    S_Mode3Sel := S_Mode3;
    S_Mode4Sel := S_Mode4;
    S_Mode5Sel := S_Mode5;
    S_Mode6Sel := S_Mode6;
    S_Mode7Sel := S_Mode7;
    S_AnyModeSel := true;
    Error := false;
16#8004:
    Ready := true;
    S_AnyModeSel := true;
    Error := false;
16#C001,
16#C002,
16#C003,
16#C004:
    Ready := true;
    S_Mode0Sel := false;
    S_Mode1Sel := false;
    S_Mode2Sel := false;
    S_Mode3Sel := false;
    S_Mode4Sel := false;
    S_Mode5Sel := false;
    S_Mode6Sel := false;
    S_Mode7Sel := false;
    S_AnyModeSel := false;
    Error := true;
END_CASE;

IF (DiagCode <> 16#8005) THEN
    T_1(IN:=false, PT:=ModeMonitorTime);
END_IF;
END_FUNCTION_BLOCK

FUNCTION_BLOCK SF_EmergencyStop
VAR_INPUT
        Activate      : BOOL;
        S_EStopIn     : BOOL;
        S_StartReset  : BOOL;
        S_AutoReset   : BOOL;
        Reset         : BOOL;
 END_VAR

 VAR_OUTPUT
        Ready         : BOOL;
        S_EStopOut    : BOOL;
        Error         : BOOL;
        DiagCode      : WORD;
 END_VAR


 VAR
        R_TRIGatReset: R_TRIG;
 END_VAR

R_TRIGatReset(CLK:=Reset);

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE

CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8001:
    IF NOT S_StartReset THEN
        DiagCode := 16#8002;
    ELSIF S_StartReset AND NOT S_EStopIn THEN
        DiagCode := 16#8004;
    ELSIF S_EStopIn AND S_StartReset THEN
        DiagCode := 16#8000;
    END_IF;
16#8002:
    IF S_EStopIn THEN
        DiagCode := 16#8003;
    END_IF;
16#8003:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C001;
    ELSIF NOT S_EStopIn THEN
        DiagCode := 16#8002;
    ELSIF R_TRIGatReset.Q THEN
        DiagCode := 16#8000;
    END_IF;
16#C001:
    IF NOT Reset THEN
        DiagCode := 16#8003;
    END_IF;
16#8004:
    IF S_EStopIn AND NOT S_AutoReset THEN
        DiagCode := 16#8005;
    ELSIF S_EStopIn AND S_AutoReset THEN
        DiagCode := 16#8000;
    END_IF;
16#8005:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C002;
    ELSIF NOT S_EStopIn THEN
        DiagCode := 16#8004;
    ELSIF R_TRIGatReset.Q OR S_AutoReset THEN
        DiagCode := 16#8000;
    END_IF;
16#C002:
    IF NOT Reset THEN
        DiagCode := 16#8005;
    END_IF;
16#8000:
    IF NOT S_EStopIn THEN
        DiagCode := 16#8004;
    END_IF;
END_CASE;
END_IF;

CASE DiagCode OF
16#0000:
    Ready := FALSE;
    Error := FALSE;
    S_EStopOut := FALSE;
16#8001:
    Ready := TRUE;
    Error := FALSE;
    S_EStopOut := FALSE;
16#8002:
    Ready := TRUE;
    Error := FALSE;
    S_EStopOut := FALSE;
16#8003:
    Ready := TRUE;
    Error := FALSE;
    S_EStopOut := FALSE;
16#C001:
    Ready := TRUE;
    Error := TRUE;
    S_EStopOut := FALSE;
16#8004:
    Ready := TRUE;
    Error := FALSE;
    S_EStopOut := FALSE;
16#8005:
    Ready := TRUE;
    Error := FALSE;
    S_EStopOut := FALSE;
16#C002:
    Ready := TRUE;
    Error := TRUE;
    S_EStopOut := FALSE;
16#8000:
    Ready := TRUE;
    Error := FALSE;
    S_EStopOut := TRUE;
END_CASE;

END_FUNCTION_BLOCK



FUNCTION_BLOCK SF_ESPE
VAR_INPUT
        Activate : BOOL;
        S_ESPE_In : BOOL;
        S_StartReset : BOOL;
        S_AutoReset : BOOL;
        Reset : BOOL;
 END_VAR

 VAR_OUTPUT
        Ready       : BOOL;
        S_ESPE_Out  : BOOL;
        Error       : BOOL;
        DiagCode    : WORD;
 END_VAR

VAR
        R_TRIGatReset: R_TRIG;
END_VAR

R_TRIGatReset(CLK:=Reset);

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE

CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8001:
    IF NOT S_StartReset THEN
        DiagCode := 16#8002;
    ELSIF S_StartReset AND NOT S_ESPE_In THEN
        DiagCode := 16#8004;
    ELSIF S_ESPE_In AND S_StartReset THEN
        DiagCode := 16#8000;
    END_IF;
16#8002:
    IF S_ESPE_In THEN
        DiagCode := 16#8003;
    END_IF;
16#8003:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C001;
    ELSIF NOT S_ESPE_In THEN
        DiagCode := 16#8002;
    ELSIF R_TRIGatReset.Q THEN
        DiagCode := 16#8000;
    END_IF;
16#C001:
    IF NOT Reset THEN
        DiagCode := 16#8003;
    END_IF;
16#8004:
    IF S_ESPE_In AND NOT S_AutoReset THEN
        DiagCode := 16#8005;
    ELSIF S_ESPE_In AND S_AutoReset THEN
        DiagCode := 16#8000;
    END_IF;
16#8005:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C002;
    ELSIF NOT S_ESPE_In THEN
        DiagCode := 16#8004;
    ELSIF R_TRIGatReset.Q OR S_AutoReset THEN
        DiagCode := 16#8000;
    END_IF;
16#C002:
    IF NOT Reset THEN
        DiagCode := 16#8005;
    END_IF;
16#8000:
    IF NOT S_ESPE_In THEN
        DiagCode := 16#8004;
    END_IF;
END_CASE;
END_IF;

CASE DiagCode OF
16#0000:
    Ready := FALSE;
    Error := FALSE;
    S_ESPE_Out := FALSE;
16#8001:
    Ready := TRUE;
    Error := FALSE;
    S_ESPE_Out := FALSE;
16#8002:
    Ready := TRUE;
    Error := FALSE;
    S_ESPE_Out := FALSE;
16#8003:
    Ready := TRUE;
    Error := FALSE;
    S_ESPE_Out := FALSE;
16#C001:
    Ready := TRUE;
    Error := TRUE;
    S_ESPE_Out := FALSE;
16#8004:
    Ready := TRUE;
    Error := FALSE;
    S_ESPE_Out := FALSE;
16#8005:
    Ready := TRUE;
    Error := FALSE;
    S_ESPE_Out := FALSE;
16#C002:
    Ready := TRUE;
    Error := TRUE;
    S_ESPE_Out := FALSE;
16#8000:
    Ready := TRUE;
    Error := FALSE;
    S_ESPE_Out := TRUE;
END_CASE;

END_FUNCTION_BLOCK

FUNCTION_BLOCK SF_SafetyRequest
VAR_INPUT
        Activate        : BOOL;
        S_OpMode        : SAFEBOOL;
        S_Acknowledge   : SAFEBOOL;
        MonitoringTime  : TIME := T#0s;
        Reset           : BOOL ;
 END_VAR

 VAR_OUTPUT
        Ready : BOOL ;
        S_SafetyActive : SAFEBOOL ;
        S_SafetyRequest : SAFEBOOL ;
        Error : BOOL ;
        DiagCode : WORD:=16#0000 ;
 END_VAR

VAR
        R_TRIGatReset: R_TRIG;
        T1: TON;
END_VAR

R_TRIGatReset(CLK:=Reset);

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE

CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8000:
    IF NOT S_Acknowledge THEN
        DiagCode := 16#C002;
    ELSIF S_OpMode THEN
        DiagCode := 16#8012;
    END_IF;
16#8001:
    IF R_TRIGatReset.Q AND NOT S_Acknowledge THEN
        DiagCode := 16#8002;
    ELSIF R_TRIGatReset.Q AND S_Acknowledge THEN
        DiagCode := 16#8012;
    END_IF;
16#8002:
    IF NOT S_OpMode THEN
        DiagCode := 16#8003;
    ELSIF S_Acknowledge THEN 
        DiagCode := 16#8012;
    END_IF;
16#8003:
    IF T1.Q THEN
        DiagCode := 16#C003;
    ELSIF S_OpMode THEN
        DiagCode := 16#8012;
    ELSIF S_Acknowledge THEN
        DiagCode := 16#8000;
    END_IF;
16#8005:
    IF S_OpMode THEN
        DiagCode := 16#8012;
    END_IF;
16#8012:
    IF NOT S_Acknowledge THEN
        DiagCode := 16#8002;
    ELSIF NOT S_OpMode THEN 
        DiagCode := 16#8003;
    END_IF;
16#C002:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C004;
    ELSIF R_TRIGatReset.Q THEN
        DiagCode := 16#8005;
    END_IF;
16#C003:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C005;
    ELSIF R_TRIGatReset.Q THEN
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
    T1(IN:=false, PT:=MonitoringTime);
ELSE
	T1(IN:=true, PT:=MonitoringTime);
END_IF;

CASE DiagCode OF
16#0000:
    Ready := FALSE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    S_SafetyActive := TRUE;
    S_SafetyRequest := FALSE;
    Error := FALSE;
16#8001:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := FALSE;
    Error := FALSE;
16#8002:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := TRUE;
    Error := FALSE;
16#8003:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := FALSE;
    Error := FALSE;
16#8005:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := FALSE;
    Error := FALSE;
16#8012:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := TRUE;
    Error := FALSE;
16#C002:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := FALSE;
    Error := TRUE;
16#C003:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := FALSE;
    Error := TRUE;
16#C004:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := FALSE;
    Error := TRUE;
16#C005:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    S_SafetyRequest := FALSE;
    Error := TRUE;
END_CASE;

END_FUNCTION_BLOCK;

FUNCTION_BLOCK SF_GuardLocking
VAR_INPUT
        Activate          : BOOL;
        S_GuardMonitoring : SAFEBOOL;
        S_SafetyActive    : SAFEBOOL;
        S_GuardLock       : SAFEBOOL;
        UnlockRequest     : BOOL;
        S_StartReset      : SAFEBOOL;
        S_AutoReset       : SAFEBOOL;
        Reset             : BOOL;
 END_VAR

 VAR_OUTPUT
    Ready             : BOOL;
    S_GuardLocked     : SAFEBOOL;
    S_UnlockGuard     : SAFEBOOL;
    Error             : BOOL;
         DiagCode                  : WORD:=16#0000;
 END_VAR

VAR
        R_TRIGatReset: R_TRIG;
        R_TRIGatUnlockRequest: R_TRIG;
END_VAR

R_TRIGatReset(CLK:=Reset);
R_TRIGatUnlockRequest(CLK:=UnlockRequest);

IF NOT Activate THEN
    DiagCode := 16#0000;
END_IF;

CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8000:
    IF NOT S_GuardLock OR NOT S_GuardMonitoring THEN
        DiagCode := 16#C004;
    ELSIF R_TRIGatUnlockRequest.Q AND S_SafetyActive THEN
        DiagCode := 16#8013;
    END_IF;
16#8001:
    IF Reset AND NOT R_TRIGatReset.Q AND NOT S_StartReset THEN
        DiagCode := 16#C001;
    ELSIF NOT S_GuardLock OR NOT S_GuardMonitoring THEN
        DiagCode := 16#8011;
    ELSIF (R_TRIGatReset.Q OR S_StartReset) AND S_GuardMonitoring AND S_GuardLock THEN
        DiagCode := 16#8000;
    END_IF;
16#8003:
    IF Reset AND NOT R_TRIGatReset.Q AND NOT S_AutoReset THEN
        DiagCode := 16#C003;
    ELSIF NOT S_GuardMonitoring OR NOT S_GuardLock THEN
        DiagCode := 16#8011;
    ELSIF R_TRIGatReset.Q OR S_AutoReset THEN
        DiagCode := 16#8000;
    END_IF;
16#8011:
    IF NOT S_SafetyActive AND (NOT S_GuardMonitoring OR NOT S_GuardLock) THEN
        DiagCode := 16#C004;
    ELSIF (S_SafetyActive AND UnlockRequest) OR NOT S_GuardMonitoring THEN
        DiagCode := 16#8012;
    ELSIF S_GuardMonitoring AND S_GuardLock THEN
        DiagCode := 16#8003;
    END_IF;
16#8012:
    IF S_GuardMonitoring THEN
        DiagCode := 16#8013;
    ELSIF NOT S_SafetyActive AND NOT S_GuardMonitoring THEN
        DiagCode := 16#C004;
    END_IF;
16#8013:
    IF NOT S_GuardMonitoring THEN
        DiagCode := 16#8012;
    ELSIF NOT UnlockRequest THEN
        DiagCode := 16#8011;
    END_IF;
16#8014:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C002;
    ELSIF NOT S_SafetyActive THEN
        DiagCode := 16#C004;
    ELSIF R_TRIGatReset.Q THEN
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
    IF S_SafetyActive THEN
        DiagCode := 16#8014;
    END_IF;
END_CASE;


CASE DiagCode OF
16#0000:
    Ready := FALSE;
    S_GuardLocked := FALSE;
    S_UnlockGuard := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    S_GuardLocked := TRUE;
    S_UnlockGuard := FALSE;
    Error := FALSE;
16#8001,
16#8003,
16#8011,
16#8014:
    Ready := TRUE;
    S_GuardLocked := FALSE;
    S_UnlockGuard := FALSE;
    Error := FALSE;
16#8012,
16#8013:
    Ready := TRUE;
    S_GuardLocked := FALSE;
    S_UnlockGuard := TRUE;
    Error := FALSE;
16#C001,
16#C002,
16#C003,
16#C004:
    Ready := TRUE;
    S_GuardLocked := FALSE;
    S_UnlockGuard := FALSE;
    Error := TRUE;
END_CASE;

END_FUNCTION_BLOCK


FUNCTION_BLOCK SF_SafelyLimitSpeed
 VAR_INPUT
    Activate          : BOOL;
    S_OpMode          : SAFEBOOL;
    S_Enabled         : SAFEBOOL;
    AxisID            : INT;
    MonitoringTime    : TIME;
    Reset             : BOOL;
    Acknowledge       : BOOL;      (* This is an internal input *)
 END_VAR

 VAR_OUTPUT
    Ready             : BOOL;
    S_SafetyActive    : SAFEBOOL;
    Error             : BOOL;
    DiagCode          : WORD:=16#0000;
 END_VAR

 VAR
    R_TRIGatReset: R_TRIG;
    T_1: TON;
 END_VAR

R_TRIGatReset(CLK:=Reset);

IF NOT Activate THEN
    DiagCode := 16#0000;
END_IF;

T_1(IN:=Activate AND NOT S_OpMode, PT:=MonitoringTime);

CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8000:
    IF NOT Acknowledge THEN
        DiagCode := 16#C002;
    ELSIF S_OpMode THEN
        DiagCode := 16#8012;
    ELSIF S_Enabled THEN
        DiagCode := 16#8004;
    END_IF;
16#8001:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C001;
    ELSIF R_TRIGatReset.Q AND NOT Acknowledge THEN
        DiagCode := 16#8002;
    ELSIF R_TRIGatReset.Q AND Acknowledge THEN
        DiagCode := 16#8012;
    END_IF;
16#8002:
    IF NOT S_OpMode THEN
        DiagCode := 16#8003;
    ELSIF Acknowledge THEN
        DiagCode := 16#8012;
    END_IF;
16#8003:
    IF T_1.Q THEN
        DiagCode := 16#C003;
    ELSIF S_OpMode THEN
        DiagCode := 16#8012;
    ELSIF Acknowledge AND NOT S_Enabled THEN
        DiagCode := 16#8000;
    ELSIF Acknowledge AND S_Enabled THEN
        DiagCode := 16#8004;
    END_IF;
16#8004:
    IF NOT Acknowledge THEN
        DiagCode := 16#C002;
    ELSIF S_OpMode THEN
        DiagCode := 16#8012;
    ELSIF NOT S_Enabled THEN
        DiagCode := 16#8000;
    END_IF;
16#8005:
    IF S_OpMode THEN
        DiagCode := 16#8012;
    END_IF;
16#8012:
    IF NOT Acknowledge THEN
        DiagCode := 16#8002;
    ELSIF NOT S_OpMode THEN
        DiagCode := 16#8003;
    END_IF;
16#C001:
    IF NOT Reset THEN
        DiagCode := 16#8001;
    END_IF;
16#C002:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C004;
    ELSIF R_TRIGatReset.Q THEN
        DiagCode := 16#8005;
    END_IF;
16#C003:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C005;
    ELSIF R_TRIGatReset.Q THEN
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
    S_SafetyActive := FALSE;
    Error := FALSE;
16#8000,
16#8004:
    Ready := TRUE;
    S_SafetyActive := TRUE;
    Error := FALSE;
16#8001,
16#8002,
16#8003,
16#8005,
16#8012:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    Error := FALSE;
16#C001,
16#C002,
16#C003,
16#C004,
16#C005:
    Ready := TRUE;
    S_SafetyActive := FALSE;
    Error := TRUE;
END_CASE;

END_FUNCTION_BLOCK

FUNCTION_BLOCK SF_TwoHandControlTypeII
 VAR_INPUT
        Activate       : BOOL := FALSE;
        S_Button1      : SAFEBOOL := FALSE;
        S_Button2      : SAFEBOOL := FALSE;
 END_VAR
 VAR_OUTPUT
        Ready          : BOOL := FALSE;
        S_TwoHandOut   : BOOL := FALSE;
        Error          : BOOL := FALSE;
        DiagCode       : WORD := 16#0;
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
     IF S_Button1 AND S_Button2 THEN
	     DiagCode := 16#C003;
     ELSIF S_Button1 THEN
	     DiagCode := 16#C001;
	 ELSIF S_Button2 THEN
	     DiagCode := 16#C002;
     ELSE
	     DiagCode := 16#8004;
     END_IF;
 16#8004:
     IF S_Button1 AND S_Button2 THEN
	     DiagCode := 16#8000;
     ELSIF S_Button1 THEN
	     DiagCode := 16#8005;
	 ELSIF S_Button2 THEN
	     DiagCode := 16#8006;
	 END_IF;
 16#8005:
     IF NOT (S_Button1 OR S_Button2) THEN
	     DiagCode := 16#8004;
	 ELSIF NOT S_Button1 AND S_Button2 THEN
	     DiagCode := 16#8019;
	 ELSIF S_Button1 AND S_Button2 THEN
	     DiagCode := 16#8000;
     END_IF;
 16#8006:
     IF NOT (S_Button1 OR S_Button2) THEN
	     DiagCode := 16#8004;
     ELSIF S_Button1 AND NOT S_Button2 THEN
	     DiagCode := 16#8019;
	 ELSIF S_Button1 AND S_Button2 THEN
	     DiagCode := 16#8000;
     END_IF;
 16#8007:
     IF S_Button1 AND S_Button2 THEN
	     DiagCode := 16#8009;
	 ELSIF NOT S_Button1 AND S_Button2 THEN 
	     DiagCode := 16#8008;
	 ELSIF NOT (S_Button1 OR S_Button2) THEN
	     DiagCode := 16#8004;
	 END_IF;
 16#8008:
     IF S_Button1 AND S_Button2 THEN
         DiagCode := 16#8009;
     ELSIF S_Button1 AND NOT S_Button2 THEN
	     DiagCode := 16#8007;		 
     ELSIF NOT (S_Button1 OR S_Button2) THEN
	     DiagCode := 16#8004;
	 END_IF;
 16#8009:
     IF NOT S_Button1 AND S_Button2 THEN
	    DiagCode := 16#8008;
	 ELSIF S_Button1 AND NOT S_Button2 THEN
	     DiagCode := 16#8007;
	 ELSIF NOT (S_Button1 OR S_Button2) THEN
	     DiagCode := 16#8004;
	 END_IF;
 16#8019:
     IF NOT (S_Button1 OR S_Button2) THEN
	     DiagCode := 16#8004;
     END_IF;
 16#8000:
     IF S_Button1 AND NOT S_Button2 THEN
	     DiagCode := 16#8007;
	 ELSIF NOT S_Button1 AND S_Button2 THEN 
	     DiagCode := 16#8008;
	 ELSIF NOT (S_Button1 OR S_Button2) THEN
	     DiagCode := 16#8004;
	 END_IF;
 16#C001,
 16#C002,
 16#C003:
     IF NOT (S_Button1 OR S_Button2) THEN
	     DiagCode := 16#8004;
	 END_IF;
	 
 END_CASE;
 END_IF;

 CASE DiagCode OF
  16#0000:
      Ready := FALSE;
      ERROR := FALSE;
      S_TwoHandOut := FALSE;
 16#8000:
      Ready := TRUE;
      ERROR := FALSE;
      S_TwoHandOut := TRUE; 
 16#8001,
 16#8004,
 16#8005,
 16#8006,
 16#8007,
 16#8008,
 16#8009,
 16#8019:
      Ready := TRUE;
      ERROR := FALSE;
      S_TwoHandOut := FALSE;
 16#C001,
 16#C002,
 16#C003:
      Ready := TRUE;
      ERROR := TRUE;
      S_TwoHandOut := FALSE;
 END_CASE;
 
END_FUNCTION_BLOCK

FUNCTION_BLOCK SF_TwoHandControlTypeIII
 VAR_INPUT
    Activate          : BOOL;
    S_Button1         : SAFEBOOL;
    S_Button2         : SAFEBOOL;
 END_VAR

 VAR_OUTPUT
    Ready             : BOOL;
    S_TwoHandOut      : SAFEBOOL;
    Error             : BOOL;
    DiagCode          : WORD:=16#0000;
 END_VAR

 VAR
    T_1: TON;
 END_VAR

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE

CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8000:
    IF S_Button1 AND NOT S_Button2 THEN
        DiagCode := 16#8007;
    ELSIF NOT S_Button1 AND S_Button2 THEN
        DiagCode := 16#8008;
    ELSIF NOT (S_Button1 OR S_Button2) THEN
        DiagCode := 16#8004;
    END_IF;
16#8001:
    IF S_Button1 AND S_Button2 THEN
        DiagCode := 16#C003;
    ELSIF S_Button1 THEN
        DiagCode := 16#C001;
    ELSIF S_Button2 THEN
        DiagCode := 16#C002;
    ELSIF NOT (S_Button1 OR S_Button2) THEN
        DiagCode := 16#8004;
    END_IF;
16#8004:
    IF S_Button1 AND S_Button2 THEN
        DiagCode := 16#8000;
    ELSIF S_Button1 THEN
        DiagCode := 16#8005;
        T_1(IN:=1, PT:=TIME#500ms);
    ELSIF S_Button2 THEN
        DiagCode := 16#8006;
        T_1(IN:=1, PT:=TIME#500ms);
    END_IF;
16#8005:
    IF T_1.Q THEN
        IF S_Button1 AND S_Button2 THEN
            DiagCode := 16#C006;
        ELSIF S_Button1 THEN
            DiagCode := 16#C005;
        ELSIF S_Button2 THEN
            DiagCode := 16#C004;
        END_IF;
    ELSIF NOT (S_Button1 OR S_Button2) THEN
        DiagCode := 16#8004;
    ELSIF NOT S_Button1 AND S_Button2 THEN
        DiagCode := 16#8019;
    ELSIF S_Button1 AND S_Button2 THEN
        DiagCode := 16#8000;
    END_IF;
16#8006:
    IF T_1.Q THEN
        IF S_Button1 AND S_Button2 THEN
            DiagCode := 16#C006;
        ELSIF S_Button1 THEN
            DiagCode := 16#C005;
        ELSIF S_Button2 THEN
            DiagCode := 16#C004;
        END_IF;
    ELSIF NOT (S_Button1 OR S_Button2) THEN
        DiagCode := 16#8004;
    ELSIF S_Button1 AND NOT S_Button2 THEN
        DiagCode := 16#8019;
    ELSIF S_Button1 AND S_Button2 THEN
        DiagCode := 16#8000;
    END_IF;
16#8007:
    IF S_Button1 AND S_Button2 THEN
        DiagCode := 16#8009;
    ELSIF NOT S_Button1 AND S_Button2 THEN
        DiagCode := 16#8008;
    ELSIF NOT (S_Button1 OR S_Button2) THEN
        DiagCode := 16#8004;
    END_IF;
16#8008:
    IF S_Button1 AND S_Button2 THEN
        DiagCode := 16#8009;
    ELSIF S_Button1 AND NOT S_Button2 THEN
        DiagCode := 16#8007;
    ELSIF NOT (S_Button1 OR S_Button2) THEN
        DiagCode := 16#8004;
    END_IF;
16#8009:
    IF NOT S_Button1 AND S_Button2 THEN
        DiagCode := 16#8008;
    ELSIF S_Button1 AND NOT S_Button2 THEN
        DiagCode := 16#8007;
    ELSIF NOT (S_Button1 OR S_Button2) THEN
        DiagCode := 16#8004;
    END_IF;
16#8019:
    IF NOT (S_Button1 OR S_Button2) THEN
        DiagCode := 16#8004;
    END_IF;
16#C001,
16#C002,
16#C003,
16#C004,
16#C005,
16#C006:
    IF NOT (S_Button1 OR S_Button2) THEN
        DiagCode := 16#8004;
    END_IF;
END_CASE;
END_IF;


CASE DiagCode OF
16#0000:
    Ready := FALSE;
    S_TwoHandOut := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    S_TwoHandOut := TRUE;
    Error := FALSE;
16#8001,
16#8004,
16#8005,
16#8006,
16#8007,
16#8008,
16#8009,
16#8019:
    Ready := TRUE;
    S_TwoHandOut := FALSE;
    Error := FALSE;
16#C001,
16#C002,
16#C003,
16#C004,
16#C005,
16#C006:
    Ready := TRUE;
    S_TwoHandOut := FALSE;
    Error := TRUE;
END_CASE;

END_FUNCTION_BLOCK

FUNCTION_BLOCK SF_GuardMonitoring
VAR_INPUT
        Activate        : BOOL := FALSE;
        S_GuardSwitch1  : SAFEBOOL := FALSE;
        S_GuardSwitch2  : SAFEBOOL := FALSE;
        DiscrepancyTime : TIME := T#0s;
        S_StartReset    : SAFEBOOL := FALSE;
        S_AutoReset     : SAFEBOOL := FALSE;
        Reset           : BOOL := FALSE;
END_VAR
VAR_OUTPUT
        Ready             : BOOL := FALSE;
        S_GuardMonitoring : BOOL := FALSE;
        Error             : BOOL := FALSE;
        DiagCode          : WORD := 16#0000;
END_VAR
VAR
    R_TRIGatReset : R_TRIG;
    DiscrepancyTimer : TON;
END_VAR

R_TRIGatReset(CLK:=Reset);

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE

CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8001:
    IF NOT S_GuardSwitch1 OR NOT S_GuardSwitch2 THEN
        DiagCode := 16#8002;
    ELSIF S_GuardSwitch1 AND S_GuardSwitch2 AND NOT S_StartReset THEN
        DiagCode := 16#8003;
    ELSIF S_GuardSwitch1 AND S_GuardSwitch2 AND S_StartReset THEN
        DiagCode := 16#8000;
    END_IF;
16#8002:
    IF NOT S_GuardSwitch1 AND NOT S_GuardSwitch2 THEN
        DiagCode := 16#8012;
    END_IF;
16#8003:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C001;
    ELSIF NOT S_GuardSwitch1 OR NOT S_GuardSwitch2 THEN
        DiagCode := 16#8002;
    ELSIF R_TRIGatReset.Q THEN
        DiagCode := 16#8000;
    END_IF;
16#8012:
    IF S_GuardSwitch1 THEN
        DiagCode := 16#8004;
        DiscrepancyTimer(IN:=TRUE, PT:=DiscrepancyTime);
    ELSIF S_GuardSwitch2 THEN
        DiagCode := 16#8014;
        DiscrepancyTimer(IN:=TRUE, PT:=DiscrepancyTime);
    END_IF;
16#8004:
    IF DiscrepancyTimer.Q THEN
        DiagCode := 16#C011;
    ELSIF NOT S_GuardSwitch1 THEN
        DiagCode := 16#8002;
    ELSIF S_GuardSwitch2 THEN
        DiagCode := 16#8005;
    END_IF;
16#8014:
    IF DiscrepancyTimer.Q THEN
        DiagCode := 16#C012;
    ELSIF NOT S_GuardSwitch2 THEN
        DiagCode := 16#8002;
    ELSIF S_GuardSwitch1 THEN
        DiagCode := 16#8005;
    END_IF;
16#8005:
    IF NOT S_GuardSwitch1 OR NOT S_GuardSwitch2 THEN
        DiagCode := 16#8002;
    ELSIF NOT S_AutoReset THEN
        DiagCode := 16#8003;
    ELSIF S_AutoReset THEN
        DiagCode := 16#8000;
    END_IF;
16#8000:
    IF NOT S_GuardSwitch1 OR NOT S_GuardSwitch2 THEN
        DiagCode := 16#8002;
    END_IF;
16#C001:
    IF NOT Reset THEN
        DiagCode := 16#8003;
    END_IF;
16#C011:
    IF NOT S_GuardSwitch1 AND NOT S_GuardSwitch2 THEN
        DiagCode := 16#8012;
    END_IF;
16#C012:
    IF NOT S_GuardSwitch1 AND NOT S_GuardSwitch2 THEN
        DiagCode := 16#8012;
    END_IF;
END_CASE;
END_IF;

CASE DiagCode OF
16#0000:
    DiscrepancyTimer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := FALSE;
    Error := FALSE;
    S_GuardMonitoring := FALSE;
16#8000:
    DiscrepancyTimer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    S_GuardMonitoring  := TRUE;
16#8001,
16#8002,
16#8003,
16#8012,
16#8005:
    DiscrepancyTimer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := FALSE;
    S_GuardMonitoring := FALSE;
16#8004,
16#8014:
    Ready := TRUE;
    Error := FALSE;
    S_GuardMonitoring := FALSE;
16#C001,
16#C011,
16#C012:
    DiscrepancyTimer(IN:=FALSE, PT:=DiscrepancyTime);
    Ready := TRUE;
    Error := TRUE;
    S_GuardMonitoring := FALSE;
END_CASE;

END_FUNCTION_BLOCK

FUNCTION_BLOCK SF_SafeStop1
 VAR_INPUT
    Activate          : BOOL;
    S_StopIn          : SAFEBOOL;
    AxisID            : INT;
    MonitoringTime    : TIME;
    Reset             : BOOL;
    Acknowledge       : BOOL;      (* This is an internal input *)
 END_VAR

 VAR_OUTPUT
    Ready             : BOOL;
    S_Stopped         : SAFEBOOL;
    Error             : BOOL;
    DiagCode          : WORD:=16#0000;
 END_VAR

 VAR
    R_TRIGatReset: R_TRIG;
    T_1: TON;
 END_VAR

R_TRIGatReset(CLK:=Reset);

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE

T_1(IN:=Activate AND NOT S_StopIn, PT:=MonitoringTime);

CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8000:
    IF NOT Acknowledge THEN
        DiagCode := 16#C002;
    ELSIF S_StopIn THEN
        DiagCode := 16#8012;
    END_IF;
16#8001:
    IF R_TRIGatReset.Q AND NOT Acknowledge THEN
        DiagCode := 16#8002;
    ELSIF R_TRIGatReset.Q AND Acknowledge THEN
        DiagCode := 16#8012;
    END_IF;
16#8002:
    IF NOT S_StopIn THEN
        DiagCode := 16#8003;
    ELSIF Acknowledge THEN
        DiagCode := 16#8012;
    END_IF;
16#8003:
    IF T_1.Q THEN
        DiagCode := 16#C003;
    ELSIF S_StopIn THEN
        DiagCode := 16#8012;
    ELSIF Acknowledge THEN
        DiagCode := 16#8000;
    END_IF;
16#8005:
    IF S_StopIn THEN
        DiagCode := 16#8012;
    END_IF;
16#8012:
    IF NOT Acknowledge THEN
        DiagCode := 16#8002;
    ELSIF NOT S_StopIn THEN
        DiagCode := 16#8003;
    ELSIF Acknowledge THEN
        DiagCode := 16#8000;
    END_IF;
16#C002:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C004;
    ELSIF R_TRIGatReset.Q THEN
        DiagCode := 16#8005;
    END_IF;
16#C003:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C005;
    ELSIF R_TRIGatReset.Q THEN
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
    S_Stopped := FALSE;
    Error := FALSE;
16#8000:
    Ready := TRUE;
    S_Stopped := TRUE;
    Error := FALSE;
16#8001,
16#8002,
16#8003,
16#8005,
16#8012:
    Ready := TRUE;
    S_Stopped := FALSE;
    Error := FALSE;
16#C002,
16#C003,
16#C004,
16#C005:
    Ready := TRUE;
    S_Stopped := FALSE;
    Error := TRUE;
END_CASE;

END_FUNCTION_BLOCK



FUNCTION_BLOCK SF_EnableSwitch
VAR_INPUT
	Activate          : BOOL := FALSE;
	S_SafetyActive    : SAFEBOOL := FALSE;
	S_EnableSwitchCh1 : SAFEBOOL := FALSE;
	S_EnableSwitchCh2 : SAFEBOOL := FALSE;
	S_AutoReset       : SAFEBOOL := FALSE;
	Reset             : BOOL := FALSE;
END_VAR

VAR_OUTPUT
	Ready: BOOL := FALSE;
	S_EnableSwitchOut: SAFEBOOL := FALSE;
	Error: BOOL := FALSE;
	DiagCode: WORD := 16#0000;
END_VAR

VAR
	R_TRIGatReset: R_TRIG;
END_VAR

R_TRIGatReset(CLK:=Reset);

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSIF NOT S_SafetyActive THEN
    DiagCode := 16#8004;
ELSE

CASE DiagCode OF
16#0000:
    IF Activate THEN
	DiagCode := 16#8004;
    END_IF;
	
16#8000:
	IF (NOT (S_EnableSwitchCh1 OR S_EnableSwitchCh2)) OR (S_EnableSwitchCh1 AND NOT S_EnableSwitchCh2) THEN
		DiagCode := 16#8007;
	ELSIF NOT S_EnableSwitchCh1 AND S_EnableSwitchCh2 THEN
		DiagCode := 16#8006;
	END_IF;
	
16#8004:
    IF S_SafetyActive THEN
		DiagCode := 16#8005;
	END_IF;

16#8005:
	IF NOT (NOT S_EnableSwitchCh1 AND S_EnableSwitchCh2) THEN
		DiagCode := 16#C010;
	ELSIF NOT S_EnableSwitchCh1 AND S_EnableSwitchCh2 THEN
		DiagCode := 16#8006;
	END_IF;
	
16#8006:
	IF S_EnableSwitchCh1 AND S_EnableSwitchCh2 THEN
		DiagCode := 16#8000;
	ELSIF (NOT (S_EnableSwitchCh1 OR S_EnableSwitchCh2)) OR (S_EnableSwitchCh1 AND NOT S_EnableSwitchCh2) THEN
		DiagCode := 16#8007;
	END_IF;
	
16#8007:
	IF S_EnableSwitchCh1 AND S_EnableSwitchCh2 THEN
		DiagCode := 16#C030;
	ELSIF NOT S_EnableSwitchCh1 AND S_EnableSwitchCh2 THEN
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
	IF NOT S_EnableSwitchCh1 AND S_EnableSwitchCh2 THEN
		DiagCode := 16#C020;
	END_IF;

16#C020:
	IF NOT (NOT S_EnableSwitchCh1 AND S_EnableSwitchCh2) THEN
		DiagCode := 16#C010;
	ELSIF Reset AND NOT R_TRIGatReset.Q AND NOT S_AutoReset THEN
		DiagCode := 16#C001;
	ELSIF R_TRIGatReset.Q OR S_AutoReset THEN
		DiagCode := 16#8006;
	END_IF;

16#C030:
	IF NOT (S_EnableSwitchCh1 AND S_EnableSwitchCh2) THEN
		DiagCode := 16#C040;
	END_IF;
	
16#C040:
	IF S_EnableSwitchCh1 AND S_EnableSwitchCh2 THEN
		DiagCode := 16#C030;
	ELSIF Reset AND R_TRIGatReset.Q AND NOT S_AutoReset THEN
		DiagCode := 16#C002;
	ELSIF R_TRIGatReset.Q OR S_AutoReset THEN
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
	S_EnableSwitchOut := FALSE;
	Error := TRUE;
16#0000:
	Ready := FALSE; 
	S_EnableSwitchOut := FALSE; 
	Error := FALSE;
16#8000:
	Ready := TRUE; 
	S_EnableSwitchOut := TRUE;
	Error := FALSE;
	
16#8004,
16#8005,
16#8006,
16#8007:
	Ready := TRUE;
	S_EnableSwitchOut := FALSE;
	Error := FALSE;

END_CASE;

END_FUNCTION_BLOCK


FUNCTION_BLOCK SF_MutingSeq
 VAR_INPUT
    Activate        : BOOL := FALSE;
    S_AOPD_In       : SAFEBOOL := FALSE;
    MutingSwitch11  : BOOL := FALSE;
    MutingSwitch12  : BOOL := FALSE;
    MutingSwitch21  : BOOL := FALSE;
    MutingSwitch22  : BOOL := FALSE;
    S_MutingLamp    : SAFEBOOL := FALSE;
    DiscTime11_12   : TIME := T#0s;
    DiscTime21_22   : TIME := T#0s;
    MaxMutingTime   : TIME := T#0s;
    MutingEnable    : BOOL := FALSE;
    S_StartReset    : SAFEBOOL := FALSE;
    Reset           : BOOL := FALSE;
 END_VAR
 VAR_OUTPUT
   Ready          : BOOL := FALSE;
   S_AOPD_Out     : SAFEBOOL := FALSE;
   S_MutingActive : SAFEBOOL := FALSE;
   Error          : BOOL := FALSE;
   DiagCode       : WORD := 16#0000;
 END_VAR
 VAR
    R_TRIGatReset  : R_TRIG;
    R_TRIGatMS_11  : R_TRIG;
    R_TRIGatMS_12  : R_TRIG;
    R_TRIGatMS_21  : R_TRIG;
    R_TRIGatMS_22  : R_TRIG;
    F_TRIGatMS_11  : F_TRIG;
    F_TRIGatMS_12  : F_TRIG;
    F_TRIGatMS_21  : F_TRIG;
    F_TRIGatMS_22  : F_TRIG;
    MutingTimer    : TON;
 END_VAR
 
R_TRIGatReset(CLK:=Reset);
R_TRIGatMS_11(CLK:=MutingSwitch11);
R_TRIGatMS_12(CLK:=MutingSwitch12);
R_TRIGatMS_21(CLK:=MutingSwitch21);
R_TRIGatMS_22(CLK:=MutingSwitch22);
F_TRIGatMS_11(CLK:=MutingSwitch11);
F_TRIGatMS_12(CLK:=MutingSwitch12);
F_TRIGatMS_21(CLK:=MutingSwitch21);
F_TRIGatMS_22(CLK:=MutingSwitch22);

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE

CASE DiagCode OF
16#0000:
    IF Activate THEN
        DiagCode := 16#8001;
    END_IF;
16#8000:

    IF (NOT MutingEnable AND R_TRIGatMS_11.Q) OR (NOT MutingEnable AND R_TRIGatMS_22.Q) OR (MutingSwitch12 OR MutingSwitch21) OR (MutingSwitch11 AND MutingSwitch22) THEN
        (* Wrong muting sequence *)
        DiagCode := 16#C004;
    ELSIF NOT S_AOPD_In THEN
        DiagCode := 16#8002;
    ELSIF NOT S_MutingLamp THEN
        DiagCode := 16#C003;
    ELSIF MutingEnable AND (R_TRIGatMS_11.Q AND NOT MutingSwitch12 AND NOT MutingSwitch21 AND NOT MutingSwitch22) THEN
        DiagCode := 16#8011;
        MutingTimer(IN:=TRUE, PT:=MaxMutingTime);
    ELSIF MutingEnable AND (NOT MutingSwitch11 AND NOT MutingSwitch12 AND NOT MutingSwitch21 AND R_TRIGatMS_22.Q) THEN
        DiagCode := 16#8122;
        MutingTimer(IN:=TRUE, PT:=MaxMutingTime);
    END_IF;
16#8001:
    IF Reset AND NOT R_TRIGatReset.Q AND NOT S_StartReset THEN
        DiagCode := 16#C001;
    ELSIF MaxMutingTime < T#0s OR MaxMutingTime > T#10m THEN
        DiagCode := 16#C005; 
    ELSIF R_TRIGatReset.Q OR S_StartReset THEN
        DiagCode := 16#8005;
    END_IF;
16#8002:
    IF S_AOPD_In THEN
        DiagCode := 16#8003;
    END_IF;
16#8003:
    IF Reset AND NOT R_TRIGatReset.Q THEN
        DiagCode := 16#C002;
    ELSIF R_TRIGatReset.Q THEN
        DiagCode := 16#8005;
    END_IF;
16#8005:
    IF NOT S_AOPD_In THEN
        DiagCode := 16#8002;
    ELSIF NOT S_MutingLamp THEN
        DiagCode := 16#C003;
    ELSIF S_AOPD_In THEN
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
16#C005:
    IF MaxMutingTime >= T#0s AND MaxMutingTime <= T#10m AND R_TRIGatReset.Q THEN
        DiagCode := 16#8001;    
    END_IF;
16#C003:
    IF S_MutingLamp THEN
        DiagCode := 16#8003;
    END_IF;
16#C006:
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
           ELSIF MutingTimer.Q THEN
               DiagCode := 16#C006;
           ELSIF NOT S_AOPD_In THEN
               DiagCode := 16#8002;
           ELSIF NOT S_MutingLamp THEN
               DiagCode := 16#C003;
           ELSIF MutingEnable AND (MutingSwitch11 AND R_TRIGatMS_12.Q AND NOT MutingSwitch21 AND NOT MutingSwitch22) THEN
               DiagCode := 16#8012;
           END_IF;
       16#8012:
           IF R_TRIGatMS_11.Q OR R_TRIGatMS_12.Q OR F_TRIGatMS_22.Q THEN
               DiagCode := 16#C104;
           ELSIF MutingTimer.Q THEN
               DiagCode := 16#C006;
           ELSIF NOT S_MutingLamp THEN
               DiagCode := 16#C003;
           ELSIF NOT MutingSwitch11 AND NOT MutingSwitch12 AND F_TRIGatMS_21.Q AND MutingSwitch22 THEN
               MutingTimer(IN:=FALSE, PT:=MaxMutingTime);
               DiagCode := 16#8000;
           END_IF;
       16#8122:
           IF NOT MutingEnable OR MutingSwitch11 OR MutingSwitch12 OR NOT MutingSwitch22 THEN
               DiagCode := 16#C104;
           ELSIF MutingTimer.Q THEN
               DiagCode := 16#C006;
           ELSIF NOT S_AOPD_In THEN
               DiagCode := 16#8002;
           ELSIF NOT S_MutingLamp THEN
               DiagCode := 16#C003;
           ELSIF MutingEnable AND (NOT MutingSwitch11 AND NOT MutingSwitch12 AND R_TRIGatMS_21.Q AND MutingSwitch22) THEN
               DiagCode := 16#8112;
           END_IF;
       16#8112:
           IF F_TRIGatMS_11.Q OR R_TRIGatMS_21.Q OR R_TRIGatMS_22.Q THEN
               DiagCode := 16#C104;
           ELSIF MutingTimer.Q THEN
               DiagCode := 16#C006;
           ELSIF NOT S_MutingLamp THEN
               DiagCode := 16#C003;
           ELSIF MutingSwitch11 AND F_TRIGatMS_12.Q AND NOT MutingSwitch21 AND NOT MutingSwitch22 THEN
               DiagCode := 16#8000;
               MutingTimer(IN:=FALSE, PT:=MaxMutingTime);
           END_IF;
       END_CASE;
    END_IF;
END_CASE;
END_IF;

CASE DiagCode OF
16#0000:
    Ready := FALSE;
    S_AOPD_Out := FALSE;
    S_MutingActive := FALSE;
    Error := FALSE;
16#8000,
16#8011,
16#8122:
    Ready := TRUE;
    S_AOPD_Out := TRUE;
    S_MutingActive := FALSE;
    Error := FALSE;
16#8001,
16#8002,
16#8003,
16#8005:
    Ready := TRUE;
    S_AOPD_Out := FALSE;
    S_MutingActive := FALSE;
    Error := FALSE;
16#8012,
16#8112:
    Ready := TRUE;
    S_AOPD_Out := TRUE;
    S_MutingActive := TRUE;
    Error := FALSE;
ELSE
    Ready := TRUE;
    S_AOPD_Out := FALSE;
    S_MutingActive := FALSE;
    Error := TRUE;
END_CASE;
END_FUNCTION_BLOCK

END_NAMESPACE (* PLCopen *)
