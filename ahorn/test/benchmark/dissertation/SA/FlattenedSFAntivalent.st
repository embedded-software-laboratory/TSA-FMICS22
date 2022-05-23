PROGRAM SFAntivalent
VAR_INPUT
    Activate : BOOL;
    SChannelNC : SAFEBOOL;
    SChannelNO : SAFEBOOL;
    DiscrepancyTime : TIME;
END_VAR
VAR
    Timer : TON;
    TimerQ : BOOL;
    TimerET : TIME;
END_VAR
VAR_OUTPUT
    Ready : BOOL;
    SAntivalentOut : BOOL;
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
        IF SChannelNC AND SChannelNO THEN
            DiagCode := 16#8004;
            Timer(IN:=TRUE, PT:=DiscrepancyTime, Q=>TimerQ, ET=>TimerET);
        ELSIF NOT SChannelNC AND NOT SChannelNO THEN
            DiagCode := 16#8014;
            Timer(IN:=TRUE, PT:=DiscrepancyTime, Q=>TimerQ, ET=>TimerET);
        ELSIF SChannelNC AND NOT SChannelNO THEN
            DiagCode := 16#8000;
        END_IF;
    16#8004:
        IF TimerQ THEN
            DiagCode := 16#C001;
        ELSIF NOT SChannelNC THEN
            DiagCode := 16#8001;
        ELSIF NOT SChannelNO THEN
            DiagCode := 16#8000;
        END_IF;
    16#C001,
    16#C002:
        IF NOT SChannelNC AND SChannelNO THEN
            DiagCode := 16#8001;
        END_IF;
    16#8014:
        IF TimerQ THEN
            DiagCode := 16#C002;
        ELSIF SChannelNO THEN
            DiagCode := 16#8001;
        ELSIF SChannelNC THEN
            DiagCode := 16#8000;
        END_IF;
    16#C003:
        IF NOT SChannelNC AND SChannelNO THEN
            DiagCode := 16#8001;
        END_IF;
    16#8005:
        IF TimerQ THEN
            DiagCode := 16#C003;
        ELSIF NOT SChannelNC AND SChannelNO THEN
            DiagCode := 16#8001;
        END_IF;
    16#8000:
        IF NOT SChannelNC OR SChannelNO THEN
            DiagCode := 16#8005;
            Timer(IN:=TRUE, PT:=DiscrepancyTime, Q=>TimerQ, ET=>TimerET);
        ELSIF NOT SChannelNC AND SChannelNO THEN
            DiagCode := 16#8001;
        END_IF;
    END_CASE;
END_IF;
    
CASE DiagCode OF
16#0000:
    Timer(IN:=FALSE, PT:=DiscrepancyTime, Q=>TimerQ, ET=>TimerET);
    Ready := FALSE;
    Error := FALSE;
    SAntivalentOut:= FALSE;
16#8000:
    Timer(IN:=FALSE, PT:=DiscrepancyTime, Q=>TimerQ, ET=>TimerET);
    Ready := TRUE;
    Error := FALSE;
    SAntivalentOut := TRUE;
16#8001:
    Timer(IN:=FALSE, PT:=DiscrepancyTime, Q=>TimerQ, ET=>TimerET);
    Ready := TRUE;
    Error := FALSE;
    SAntivalentOut:= FALSE;
16#8004,
16#8014,
16#8005:
    Timer(IN:=TRUE, PT:=DiscrepancyTime, Q=>TimerQ, ET=>TimerET);
    Ready := TRUE;
    Error := FALSE;
    SAntivalentOut:= FALSE;
16#C001,
16#C002,
16#C003:
    Timer(IN:=FALSE, PT:=DiscrepancyTime, Q=>TimerQ, ET=>TimerET);
    Ready := TRUE;
    Error := TRUE;
    SAntivalentOut:= FALSE;
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
