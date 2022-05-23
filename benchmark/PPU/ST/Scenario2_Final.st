TYPE
    CraneStates : (CraneSFCInit, StartCrane, CraneInit, CraneInit2, Interstep, Interstep2, TimeDelay, TurnRight,
                   InterstepCheckWorkpiece, StackStop, Step0, CraneLiftStack, CraneTurnLeftNOTMetallic,
                   CraneOnConveyorState, CraneLiftStackMetallic, CraneTurnLeftMetallic, CraneOnConveyorMetallic,
                   release, CraneLiftConveyor, CraneStop);
    StackStates : (StackSFCInit, StartStack, GreenLamp, StackInit, StackInit2, Interstep, convey, Step0, SliderMoveBack,
                   Step1);
END_TYPE

FUNCTION_BLOCK Crane
VAR_INPUT
    CraneDown : BOOL := FALSE;
    CraneOnConveyor : BOOL := FALSE;
    CraneOnStack : BOOL := FALSE;
    CraneSucked : BOOL := FALSE;
    CraneUp : BOOL := FALSE;
    StackCapacitiveSensor : BOOL := FALSE;
    SFCReset : BOOL := FALSE;
    SliderMovedOut : BOOL := FALSE;
    SliderNotMovedOut : BOOL := FALSE;
    StartButtonStack : BOOL := FALSE;
    StartVar : BOOL := FALSE;
    WorkpieceReady : BOOL := FALSE;
END_VAR
VAR
    SFCInit : BOOL := FALSE;
    SFCPause : BOOL := FALSE;
    CraneState : CraneStates := CraneStates#CraneSFCInit;
    Timer : TON;
    TimerDuration : TIME := TIME#50.0ms;
    Timerinterconnect : BOOL := FALSE;
    interconnectCraneStartCommand : BOOL := FALSE;
END_VAR
VAR_OUTPUT
    CraneLower : BOOL := FALSE;
    CraneTurnClockwise : BOOL := FALSE;
    CraneTurnCounterClockwise : BOOL := FALSE;
    StackVacuumOff : BOOL := FALSE;
    StackVacuumOn : BOOL := FALSE;
    StartCommandCrane : BOOL := FALSE;
END_VAR

IF (SFCInit OR SFCReset) THEN
    CraneDown := FALSE;
    CraneLower := FALSE;
    CraneOnConveyor := FALSE;
    CraneOnStack := FALSE;
    CraneSucked := FALSE;
    CraneTurnClockwise := FALSE;
    CraneTurnCounterClockwise := FALSE;
    CraneUp := FALSE;
    StackCapacitiveSensor := FALSE;
    StackVacuumOff := FALSE;
    StackVacuumOn := FALSE;
    SFCInit := FALSE;
    SFCReset := FALSE;
    SliderMovedOut := FALSE;
    SliderNotMovedOut := FALSE;
    StartButtonStack := FALSE;
    StartCommandCrane := FALSE;
    StartVar := FALSE;
    Timer.ET := INT#0;
    Timer.IN := FALSE;
    Timer.PT := INT#0;
    Timer.Q := FALSE;
    TimerDuration := TIME#50ms;
    Timerinterconnect := FALSE;
    WorkpieceReady := FALSE;
    CraneState := CraneStates#CraneSFCInit;
    interconnectCraneStartCommand := FALSE;
END_IF;

IF NOT (SFCInit OR SFCPause) THEN
    CASE CraneState OF
        CraneStates#CraneSFCInit:
            IF TRUE THEN
                CraneState := CraneStates#StartCrane;
            END_IF;

        CraneStates#StartCrane:
            CraneLower := FALSE;
            StackVacuumOff := TRUE;
            StackVacuumOn := FALSE;
            CraneTurnCounterClockwise := FALSE;
            CraneTurnClockwise := FALSE;
            StartVar := FALSE;
            StartCommandCrane := FALSE;
            IF (StartButtonStack = TRUE) THEN
                interconnectCraneStartCommand := TRUE;
            END_IF;
            IF interconnectCraneStartCommand THEN
                CraneState := CraneStates#CraneInit;
            END_IF;

        CraneStates#CraneInit:
            CraneLower := TRUE;
            interconnectCraneStartCommand := FALSE;
            IF CraneDown THEN
                CraneState := CraneStates#CraneInit2;
            END_IF;

        CraneStates#CraneInit2:
            CraneLower := FALSE;
            IF CraneUp THEN
                CraneState := CraneStates#Interstep;
            END_IF;

        CraneStates#Interstep:
            StartCommandCrane := TRUE;
            IF StartVar THEN
                CraneState := CraneStates#Interstep2;
            END_IF;

        CraneStates#Interstep2:
            IF SliderMovedOut THEN
                CraneState := CraneStates#TimeDelay;
            END_IF;

        CraneStates#TimeDelay:
            Timer(IN := TRUE, PT := TimerDuration);
            Timerinterconnect := Timer.Q;
            IF Timerinterconnect THEN
                CraneState := CraneStates#TurnRight;
            END_IF;

        CraneStates#TurnRight:
            CraneTurnClockwise := TRUE;
            CraneTurnCounterClockwise := FALSE;
            IF CraneOnStack THEN
                CraneState := CraneStates#InterstepCheckWorkpiece;
            END_IF;

        CraneStates#InterstepCheckWorkpiece:
            IF WorkpieceReady THEN
                CraneState := CraneStates#StackStop;
            ELSIF NOT WorkpieceReady THEN
                CraneState := CraneStates#CraneStop;
            END_IF;

        CraneStates#StackStop:
            CraneTurnClockwise := FALSE;
            CraneTurnCounterClockwise := FALSE;
            StackVacuumOn := TRUE;
            StackVacuumOff := FALSE;
            CraneLower := TRUE;
            IF CraneSucked THEN
                CraneState := CraneStates#Step0;
            END_IF;

        CraneStates#Step0:
            IF NOT StackCapacitiveSensor THEN
                CraneState := CraneStates#CraneLiftStack;
            ELSIF StackCapacitiveSensor THEN
                CraneState := CraneStates#CraneLiftStackMetallic;
            END_IF;

        CraneStates#CraneLiftStack:
            CraneLower := FALSE;
            IF CraneUp THEN
                CraneState := CraneStates#CraneTurnLeftNOTMetallic;
            END_IF;

        CraneStates#CraneTurnLeftNOTMetallic:
            CraneTurnCounterClockwise := TRUE;
            CraneTurnClockwise := FALSE;
            IF CraneOnConveyor THEN
                CraneState := CraneStates#CraneOnConveyorState;
            END_IF;

        CraneStates#CraneOnConveyorState:
            CraneTurnCounterClockwise := FALSE;
            CraneTurnClockwise := FALSE;
            CraneLower := TRUE;
            IF CraneDown THEN
                CraneState := CraneStates#release;
            END_IF;

        CraneStates#CraneLiftStackMetallic:
            CraneLower := FALSE;
            IF CraneUp THEN
                CraneState := CraneStates#CraneTurnLeftMetallic;
            END_IF;

        CraneStates#CraneTurnLeftMetallic:
            CraneTurnCounterClockwise := TRUE;
            CraneTurnClockwise := FALSE;
            IF CraneOnConveyor THEN
                CraneState := CraneStates#CraneOnConveyorMetallic;
            END_IF;

        CraneStates#CraneOnConveyorMetallic:
            CraneTurnCounterClockwise := FALSE;
            CraneTurnClockwise := FALSE;
            CraneLower := TRUE;
            IF CraneDown THEN
                CraneState := CraneStates#release;
            END_IF;

        CraneStates#release:
            StackVacuumOff := TRUE;
            StackVacuumOn := FALSE;
            IF TRUE THEN
                CraneState := CraneStates#CraneLiftConveyor;
            END_IF;

        CraneStates#CraneLiftConveyor:
            CraneLower := FALSE;
            IF TRUE THEN
                CraneState := CraneStates#Interstep;
            END_IF;

        CraneStates#CraneStop:
            IF TRUE THEN
                CraneState := CraneStates#StartCrane;
            END_IF;
    END_CASE;
END_IF;
END_FUNCTION_BLOCK

FUNCTION_BLOCK Stack
VAR_INPUT
    CraneDown : BOOL := FALSE;
    CraneOnConveyor : BOOL := FALSE;
    CraneOnStack : BOOL := FALSE;
    CraneSucked : BOOL := FALSE;
    CraneUp : BOOL := FALSE;
    SFCReset : BOOL := FALSE;
    SliderMovedOut : BOOL := FALSE;
    SliderNotMovedOut : BOOL := FALSE;
    StartButtonStack : BOOL := FALSE;
    StartVar : BOOL := FALSE;
    WorkpieceReady : BOOL := FALSE;
END_VAR
VAR
    SFCInit : BOOL := FALSE;
    SFCPause : BOOL := FALSE;
    StackState : StackStates := StackStates#StackSFCInit;
    interconnectStackStartCommand : BOOL := FALSE;
END_VAR
VAR_OUTPUT
    CraneLower : BOOL := FALSE;
    CraneTurnClockwise : BOOL := FALSE;
    CraneTurnCounterClockwise : BOOL := FALSE;
    StackGreenLamp : BOOL := FALSE;
    StackSlider : BOOL := FALSE;
    StackVacuumOff : BOOL := FALSE;
    StackVacuumOn : BOOL := FALSE;
    StackWhiteLamp : BOOL := FALSE;
    StartCommandStack : BOOL := FALSE;
END_VAR

IF (SFCInit OR SFCReset) THEN
    CraneDown := FALSE;
    CraneLower := FALSE;
    CraneOnConveyor := FALSE;
    CraneOnStack := FALSE;
    CraneSucked := FALSE;
    CraneTurnClockwise := FALSE;
    CraneTurnCounterClockwise := FALSE;
    CraneUp := FALSE;
    StackGreenLamp := FALSE;
    StackSlider := FALSE;
    StackVacuumOff := FALSE;
    StackVacuumOn := FALSE;
    StackWhiteLamp := FALSE;
    SFCInit := FALSE;
    SFCReset := FALSE;
    SliderMovedOut := FALSE;
    SliderNotMovedOut := FALSE;
    StartButtonStack := FALSE;
    StartCommandStack := FALSE;
    StartVar := FALSE;
    WorkpieceReady := FALSE;
    StackState := StackStates#StackSFCInit;
    interconnectStackStartCommand := FALSE;
END_IF;

IF NOT (SFCInit OR SFCPause) THEN
    CASE StackState OF
        StackStates#StackSFCInit:
            IF TRUE THEN
                StackState := StackStates#StartStack;
            END_IF;

        StackStates#StartStack:
            StackSlider := FALSE;
            StackGreenLamp := FALSE;
            StartVar := FALSE;
            StartCommandStack := FALSE;
            IF (StartButtonStack = TRUE) THEN
                interconnectStackStartCommand := TRUE;
            END_IF;
            IF interconnectStackStartCommand THEN
                StackState := StackStates#GreenLamp;
            END_IF;

        StackStates#GreenLamp:
            StackGreenLamp := TRUE;
            interconnectStackStartCommand := FALSE;
            IF TRUE THEN
                StackState := StackStates#StackInit;
            END_IF;

        StackStates#StackInit:
            StackSlider := TRUE;
            IF TRUE THEN
                StackState := StackStates#StackInit2;
            END_IF;

        StackStates#StackInit2:
            StackSlider := FALSE;
            IF TRUE THEN
                StackState := StackStates#Interstep;
            END_IF;

        StackStates#Interstep:
            StartCommandStack := TRUE;
            IF StartVar THEN
                StackState := StackStates#convey;
            END_IF;

        StackStates#convey:
            IF SliderNotMovedOut THEN
                StackSlider := TRUE;
            END_IF;
            IF CraneOnStack THEN
                StackState := StackStates#Step0;
            END_IF;

        StackStates#Step0:
            IF CraneDown THEN
                StackState := StackStates#SliderMoveBack;
            END_IF;

        StackStates#SliderMoveBack:
            IF ((SliderMovedOut = TRUE) AND (SliderNotMovedOut = FALSE)) THEN
                StackSlider := FALSE;
            END_IF;
            IF CraneUp THEN
                StackState := StackStates#Step1;
            END_IF;

        StackStates#Step1:
            IF CraneOnConveyor THEN
                StackState := StackStates#Interstep;
            END_IF;
    END_CASE;
END_IF;
END_FUNCTION_BLOCK

PROGRAM Main
VAR_INPUT
    SensorCraneDown : BOOL := FALSE;
    SensorCraneOnConveyor : BOOL := FALSE;
    SensorCraneOnStack : BOOL := FALSE;
    SensorCranePosition : BOOL := FALSE;
    SensorCranePositionStamp : BOOL := FALSE;
    SensorCraneSucked : BOOL := FALSE;
    SensorCraneUp : BOOL := FALSE;
    SensorStackCapacitiveSensor : BOOL := FALSE;
    SensorStackEmergencyStop : BOOL := FALSE;
    SensorStackOpticalSensor : BOOL := FALSE;
    SensorStackSwitchManuellAutomatic : BOOL := FALSE;
    SensorSliderMovedOut : BOOL := FALSE;
    SensorSliderNotMovedOut : BOOL := FALSE;
    SensorStartButtonStack : BOOL := FALSE;
    SensorWorkpieceReady : BOOL := FALSE;
END_VAR
VAR
    Crane : Crane;
    Stack : Stack;
END_VAR
VAR_OUTPUT
    ActuatorCraneLower : BOOL := FALSE;
    ActuatorCraneTurnClockwise : BOOL := FALSE;
    ActuatorCraneTurnCounterClockwise : BOOL := FALSE;
    ActuatorStackGreenLamp : BOOL := FALSE;
    ActuatorStackSlider : BOOL := FALSE;
    ActuatorStackVacuumOff : BOOL := FALSE;
    ActuatorStackVacuumOn : BOOL := FALSE;
    ActuatorStackWhiteLamp : BOOL := FALSE;
END_VAR
Stack.SliderNotMovedOut := SensorSliderNotMovedOut;
Stack.SliderMovedOut := SensorSliderMovedOut;
Stack.CraneOnStack := SensorCraneOnStack;
Stack.CraneDown := SensorCraneDown;
Stack.CraneUp := SensorCraneUp;
Stack.CraneOnConveyor := SensorCraneOnConveyor;
Stack.WorkpieceReady := SensorWorkpieceReady;
Stack.StartButtonStack := SensorStartButtonStack;
ActuatorStackSlider := Stack.StackSlider;
ActuatorStackGreenLamp := Stack.StackGreenLamp;
Crane.WorkpieceReady := SensorWorkpieceReady;
Crane.CraneUp := SensorCraneUp;
Crane.CraneOnConveyor := SensorCraneOnConveyor;
Crane.CraneDown := SensorCraneDown;
Crane.CraneSucked := SensorCraneSucked;
Crane.CraneOnStack := SensorCraneOnStack;
Crane.StackCapacitiveSensor := SensorStackCapacitiveSensor;
Crane.SliderMovedOut := SensorSliderMovedOut;
Crane.StartButtonStack := SensorStartButtonStack;
ActuatorCraneTurnCounterClockwise := Crane.CraneTurnCounterClockwise;
ActuatorCraneTurnClockwise := Crane.CraneTurnClockwise;
ActuatorCraneLower := Crane.CraneLower;
ActuatorStackVacuumOff := Crane.StackVacuumOff;
ActuatorStackVacuumOn := Crane.StackVacuumOn;
IF SensorStackEmergencyStop THEN
    Stack();
    Crane();
    IF ActuatorStackGreenLamp THEN
        IF (Crane.StartCommandCrane AND Stack.StartCommandStack) THEN
            Crane.StartVar := TRUE;
            Stack.StartVar := TRUE;
        END_IF;
    END_IF;
    Crane.SFCReset := FALSE;
    Stack.SFCReset := FALSE;
ELSIF NOT SensorStackEmergencyStop THEN
    ActuatorStackSlider := FALSE;
    ActuatorCraneLower := FALSE;
    ActuatorStackVacuumOn := FALSE;
    ActuatorStackVacuumOff := TRUE;
    ActuatorStackGreenLamp := FALSE;
    ActuatorCraneTurnCounterClockwise := FALSE;
    ActuatorCraneTurnClockwise := FALSE;
    Crane.SFCReset := TRUE;
    Stack.SFCReset := TRUE;
    Crane.StartVar := FALSE;
    Stack.StartVar := FALSE;
END_IF;
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