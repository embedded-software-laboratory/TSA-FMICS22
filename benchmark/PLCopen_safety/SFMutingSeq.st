PROGRAM SFMutingSeq
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