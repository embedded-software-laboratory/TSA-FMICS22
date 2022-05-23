PROGRAM SFTestableSafetySensor
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