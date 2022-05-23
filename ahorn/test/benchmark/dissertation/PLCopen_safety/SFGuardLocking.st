PROGRAM SFGuardLocking
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
