PROGRAM ModeSelectorX
VAR_INPUT
        Activate        : BOOL;
        SMode0         : BOOL;
        SMode1         : BOOL;
        SMode2         : BOOL;
        SMode3         : BOOL;
        SMode4         : BOOL;
        SMode5         : BOOL;
        SMode6         : BOOL;
        SMode7         : BOOL;
        SUnlock        : BOOL;
        SSetMode       : BOOL;
        AutoSetMode     : BOOL;
        ModeMonitorTime : TIME; 
        Reset           : BOOL;
 END_VAR
 VAR_OUTPUT
        Ready           : BOOL;
        SMode0Sel      : BOOL;
        SMode1Sel      : BOOL;
        SMode2Sel      : BOOL;
        SMode3Sel      : BOOL;
        SMode4Sel      : BOOL;
        SMode5Sel      : BOOL;
        SMode6Sel      : BOOL;
        SMode7Sel      : BOOL;
        SAnyModeSel    : BOOL;
        Error           : BOOL;
        DiagCode        : WORD;
 END_VAR
 VAR
        Timer1        : TONX;                 (* Timer for S2, transition T27 *)
 END_VAR

 VAR
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
 
RTRIGatReset(CLK:=Reset);
RTRIGatSetMode(CLK:=SSetMode);

IF NOT Activate THEN
    DiagCode := 16#0000;
ELSE

IF (SMode0 OR SMode1 OR SMode2 OR SMode3 OR SMode4 OR SMode5 OR SMode6 OR SMode7) AND NOT ((
                                                                                                  (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND SMode7) OR
                                                                                                  (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND SMode6 AND NOT SMode7) OR
                                                                                                  (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                                                                  (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                                                                  (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                                                                  (NOT SMode0 AND NOT SMode1 AND SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                                                                  (NOT SMode0 AND SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                                                                  (SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7))) THEN
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
        Timer1(IN:=true, PT:=ModeMonitorTime);
    ELSIF RTRIGatModeSel0.Q THEN
        Timer1(IN:=false, PT:=ModeMonitorTime);
        fallingEdgeDetected := FALSE;
    END_IF;

    Error2 := Timer1.Q;

    IF Error1 THEN
        DiagCode := 16#C001;
    ELSIF Error2 THEN
        DiagCode := 16#C002;
    ELSIF ((AutoSetMode OR RTRIGatSetMode.Q) AND SUnlock AND ((NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND SMode7) OR
                                                               (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND SMode6 AND NOT SMode7) OR
                                                               (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                               (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                               (NOT SMode0 AND NOT SMode1 AND NOT SMode2 AND SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                               (NOT SMode0 AND NOT SMode1 AND SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                               (NOT SMode0 AND SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7) OR
                                                               (SMode0 AND NOT SMode1 AND NOT SMode2 AND NOT SMode3 AND NOT SMode4 AND NOT SMode5 AND NOT SMode6 AND NOT SMode7))) THEN
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
    Timer1(IN:=false, PT:=ModeMonitorTime);
END_IF;
END_PROGRAM

FUNCTION_BLOCK TONX
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
   CLK: BOOL;
END_VAR
VAR_OUTPUT
   Q: BOOL;
END_VAR
VAR
   M: BOOL;
END_VAR
Q := CLK AND NOT M;
M := CLK;
END_FUNCTION_BLOCK

FUNCTION_BLOCK FTRIG
VAR_INPUT
   CLK: BOOL;
END_VAR
VAR_OUTPUT
   Q: BOOL;
END_VAR
VAR
   M: BOOL;
END_VAR
Q := NOT CLK AND M;
M := CLK;
END_FUNCTION_BLOCK