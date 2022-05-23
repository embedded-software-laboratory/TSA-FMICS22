TYPE
    CraneStates : (CraneSFCInit, StartCrane, CraneInit, CraneInit2, Interstep, Interstep2, TimeDelay, TurnRight, 
                   InterstepCheckWorkpiece, StackStop, Step0, CraneLiftStack, CraneTurnLeftNOTMetallic,
                   CraneOnConveyorState, CraneLiftStackMetallic, CraneTurnLeftMetallic, CraneOnStampMetallic,
                   releaseWorkpiece, CraneGoUp, pause, Resetinterconnect, LowerCrane, SuckAtStamp, CraneLiftAtStamp,
                   CraneTurnLeftFromStamp, CraneOnConveyorMetallic, release, CraneLiftConveyor, WaitAtStamp, Step2,
                   CraneStop);
    StackStates : (StackSFCInit, StartStack, GreenLamp, StackInit, StackInit2, Interstep, convey, Step0, SliderMoveBack,
                   Step1);
    StampStates : (StampSFCInit, StartStamp, GreenLamp, StampInit1, StampInit2, StampInit3, StampInit4, Interstep,
                   Interstep2, Step0, StampSliderBack, StampProcess, Step1, StampSliderForward, Step2);
END_TYPE

FUNCTION_BLOCK Crane
VAR_INPUT
    CraneDown : BOOL := FALSE;
    CraneOnConveyor : BOOL := FALSE;
    CraneOnStack : BOOL := FALSE;
    CranePositionStamp : BOOL := FALSE;
    CraneSucked : BOOL := FALSE;
    CraneUp : BOOL := FALSE;
    StackCapacitiveSensor : BOOL := FALSE;
    SFCReset : BOOL := FALSE;
    SliderMovedOut : BOOL := FALSE;
    SliderNotMovedOut : BOOL := FALSE;
    StampLowered : BOOL := FALSE;
    StampSliderFilled : BOOL := FALSE;
    StartButtonStack : BOOL := FALSE;
    StartVar : BOOL := FALSE;
    WorkpieceReady : BOOL := FALSE;
END_VAR
VAR
    Duration1 : TIME := TIME#1500.0ms;
    SFCInit : BOOL := FALSE;
    SFCPause : BOOL := FALSE;
    CraneState : CraneStates := CraneStates#CraneSFCInit;
    Timer : TON;
    TimerDuration : TIME := TIME#50.0ms;
    Timerinterconnect : BOOL := FALSE;
    Timer1 : TON;
    interconnect : BOOL := FALSE;
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
    CranePositionStamp := change(CranePositionStamp, FALSE);
    CraneSucked := FALSE;
    CraneTurnClockwise := FALSE;
    CraneTurnCounterClockwise := FALSE;
    CraneUp := FALSE;
    Duration1 := change(Duration1, TIME#1500ms);
    StackCapacitiveSensor := FALSE;
    StackVacuumOff := FALSE;
    StackVacuumOn := FALSE;
    SFCInit := FALSE;
    SFCReset := FALSE;
    SliderMovedOut := FALSE;
    SliderNotMovedOut := FALSE;
    StampLowered := change(StampLowered, FALSE);
    StampSliderFilled := change(StampSliderFilled, FALSE);
    StartButtonStack := FALSE;
    StartCommandCrane := FALSE;
    StartVar := FALSE;
    Timer.ET := INT#0;
    Timer.IN := FALSE;
    Timer.PT := INT#0;
    Timer.Q := FALSE;
    TimerDuration := TIME#50ms;
    Timerinterconnect := FALSE;
    Timer1.ET := change(Timer1.ET, INT#0);
    Timer1.IN := change(Timer1.IN, FALSE);
    Timer1.PT := change(Timer1.PT, INT#0);
    Timer1.Q := change(Timer1.Q, FALSE);
    WorkpieceReady := FALSE;
    CraneState := CraneStates#CraneSFCInit;
    interconnect := change(interconnect, FALSE);
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
            ELSIF (NOT WorkpieceReady AND change(TRUE, StampSliderFilled)) THEN
                CraneState := change(CraneStates#CraneStop, CraneStates#WaitAtStamp);
            ELSIF change(false, (NOT WorkpieceReady AND NOT StampSliderFilled)) THEN
                CraneState := CraneStates#Step2;
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
            IF change(CraneOnConveyor, CranePositionStamp) THEN
                CraneState := change(CraneStates#CraneOnConveyorMetallic, CraneStates#CraneOnStampMetallic);
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

        change(CraneStates#CraneStop, CraneStates#Step2):
            IF TRUE THEN
                CraneState := CraneStates#StartCrane;
            END_IF;

        ELSE
            IF change(false, true) THEN
                CASE CraneState OF
                        CraneStates#CraneOnStampMetallic:
                            CraneTurnCounterClockwise := FALSE;
                            CraneTurnClockwise := FALSE;
                            CraneLower := TRUE;
                            IF CraneDown THEN
                                CraneState := CraneStates#releaseWorkpiece;
                            END_IF;

                        CraneStates#releaseWorkpiece:
                            StackVacuumOff := TRUE;
                            StackVacuumOn := FALSE;
                            IF TRUE THEN
                                CraneState := CraneStates#CraneGoUp;
                            END_IF;

                        CraneStates#CraneGoUp:
                            CraneLower := FALSE;
                            IF CraneUp THEN
                                CraneState := CraneStates#pause;
                            END_IF;

                        CraneStates#pause:
                            Timer1(IN := CraneUp, PT := Duration1);
                            interconnect := Timer1.Q;
                            IF interconnect THEN
                                CraneState := CraneStates#Resetinterconnect;
                            END_IF;

                        CraneStates#Resetinterconnect:
                            interconnect := FALSE;
                            Timer1(IN := FALSE, PT := TIME#1ms);
                            IF SliderMovedOut THEN
                                CraneState := CraneStates#LowerCrane;
                            END_IF;

                        CraneStates#LowerCrane:
                            CraneLower := TRUE;
                            IF CraneDown THEN
                                CraneState := CraneStates#SuckAtStamp;
                            END_IF;

                        CraneStates#SuckAtStamp:
                            StackVacuumOn := TRUE;
                            StackVacuumOff := FALSE;
                            IF CraneSucked THEN
                                CraneState := CraneStates#CraneLiftAtStamp;
                            END_IF;

                        CraneStates#CraneLiftAtStamp:
                            CraneLower := FALSE;
                            IF CraneUp THEN
                                CraneState := CraneStates#CraneTurnLeftFromStamp;
                            END_IF;

                        CraneStates#CraneTurnLeftFromStamp:
                            CraneTurnClockwise := TRUE;
                            CraneTurnCounterClockwise := FALSE;
                            IF CraneOnConveyor THEN
                                CraneState := CraneStates#CraneOnConveyorMetallic;
                            END_IF;

                        CraneStates#WaitAtStamp:
                            IF TRUE THEN
                                CraneState := CraneStates#pause;
                            END_IF;
                END_CASE;
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

FUNCTION_BLOCK Stamp
VAR_INPUT
    CraneDown : BOOL := FALSE;
    CraneOnConveyor : BOOL := FALSE;
    CranePositionStamp : BOOL := FALSE;
    CraneSucked : BOOL := FALSE;
    CraneUp : BOOL := FALSE;
    SFCReset : BOOL := FALSE;
    StampLowered : BOOL := FALSE;
    StampSliderFilled : BOOL := FALSE;
    StampSliderSensorMovedIn : BOOL := FALSE;
    StampSliderSensorMovedOut : BOOL := FALSE;
    StampStartButton : BOOL := FALSE;
    StampUp : BOOL := FALSE;
    StartVar : BOOL := FALSE;
END_VAR
VAR
    Duration : TIME := TIME#500.0ms;
    SFCInit : BOOL := FALSE;
    SFCPause : BOOL := FALSE;
    StampState : StampStates := StampStates#StampSFCInit;
    Timer : TON;
    interconnect : BOOL := FALSE;
    interconnectStampStartCommand : BOOL := FALSE;
END_VAR
VAR_OUTPUT
    StampGreenLamp : BOOL := FALSE;
    StampPressure : BOOL := FALSE;
    StampPusher : BOOL := FALSE;
    StampSliderMovedIn : BOOL := FALSE;
    StampSliderMovedOut : BOOL := FALSE;
    StampWhiteLamp : BOOL := FALSE;
    StartCommandStamp : BOOL := FALSE;
END_VAR

IF (SFCInit OR SFCReset) THEN
    CraneDown := FALSE;
    CraneOnConveyor := FALSE;
    CranePositionStamp := FALSE;
    CraneSucked := FALSE;
    CraneUp := FALSE;
    Duration := TIME#500ms;
    SFCInit := FALSE;
    SFCReset := FALSE;
    StampGreenLamp := FALSE;
    StampLowered := FALSE;
    StampPressure := FALSE;
    StampPusher := FALSE;
    StampSliderFilled := FALSE;
    StampSliderMovedIn := FALSE;
    StampSliderMovedOut := FALSE;
    StampSliderSensorMovedIn := FALSE;
    StampSliderSensorMovedOut := FALSE;
    StampStartButton := FALSE;
    StampUp := FALSE;
    StampWhiteLamp := FALSE;
    StartCommandStamp := FALSE;
    StartVar := FALSE;
    Timer.ET := INT#0;
    Timer.IN := FALSE;
    Timer.PT := INT#0;
    Timer.Q := FALSE;
    StampState := StampStates#StampSFCInit;
    interconnect := FALSE;
    interconnectStampStartCommand := FALSE;
END_IF;

IF NOT (SFCInit OR SFCPause) THEN
    CASE StampState OF
        StampStates#StampSFCInit:
            IF TRUE THEN
                StampState := StampStates#StartStamp;
            END_IF;

        StampStates#StartStamp:
            StampSliderMovedOut := FALSE;
            StampSliderMovedIn := FALSE;
            StampPusher := FALSE;
            StampGreenLamp := FALSE;
            StampPressure := FALSE;
            StartCommandStamp := FALSE;
            IF (StampStartButton = TRUE) THEN
                interconnectStampStartCommand := TRUE;
            END_IF;
            IF interconnectStampStartCommand THEN
                StampState := StampStates#GreenLamp;
            END_IF;

        StampStates#GreenLamp:
            StampGreenLamp := TRUE;
            interconnectStampStartCommand := FALSE;
            IF TRUE THEN
                StampState := StampStates#StampInit1;
            END_IF;

        StampStates#StampInit1:
            StampSliderMovedIn := TRUE;
            StampSliderMovedOut := FALSE;
            IF StampSliderSensorMovedIn THEN
                StampState := StampStates#StampInit2;
            END_IF;

        StampStates#StampInit2:
            StampSliderMovedIn := FALSE;
            StampSliderMovedOut := TRUE;
            IF StampSliderSensorMovedOut THEN
                StampState := StampStates#StampInit3;
            END_IF;

        StampStates#StampInit3:
            StampPusher := TRUE;
            IF StampLowered THEN
                StampState := StampStates#StampInit4;
            END_IF;

        StampStates#StampInit4:
            StampPusher := FALSE;
            IF StampUp THEN
                StampState := StampStates#Interstep;
            END_IF;

        StampStates#Interstep:
            StartCommandStamp := TRUE;
            IF StartVar THEN
                StampState := StampStates#Interstep2;
            END_IF;

        StampStates#Interstep2:
            IF StampSliderFilled THEN
                StampState := StampStates#Step0;
            END_IF;

        StampStates#Step0:
            IF CraneUp THEN
                StampState := StampStates#StampSliderBack;
            END_IF;

        StampStates#StampSliderBack:
            StampSliderMovedIn := TRUE;
            StampSliderMovedOut := FALSE;
            IF StampSliderSensorMovedIn THEN
                StampState := StampStates#StampProcess;
            END_IF;

        StampStates#StampProcess:
            StampSliderMovedIn := FALSE;
            StampPusher := TRUE;
            IF StampLowered THEN
                StampState := StampStates#Step1;
            END_IF;

        StampStates#Step1:
            Timer(IN := StampLowered, PT := TIME#0ms);
            interconnect := Timer.Q;
            IF interconnect THEN
                StampState := StampStates#StampSliderForward;
            END_IF;

        StampStates#StampSliderForward:
            StampPusher := FALSE;
            StampSliderMovedOut := TRUE;
            IF ((StampLowered = FALSE) AND (StampUp = TRUE)) THEN
                interconnect := FALSE;
            END_IF;
            Timer(IN := FALSE, PT := TIME#1ms);
            IF StampSliderMovedOut THEN
                StampState := StampStates#Step2;
            END_IF;

        StampStates#Step2:
            IF CraneOnConveyor THEN
                StampState := StampStates#Interstep;
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
    SensorStampEmergencyStop : BOOL := FALSE;
    SensorStampLowered : BOOL := FALSE;
    SensorStampSliderFilled : BOOL := FALSE;
    SensorStampSliderSensorMovedIn : BOOL := FALSE;
    SensorStampSliderSensorMovedOut : BOOL := FALSE;
    SensorStampStartButton : BOOL := FALSE;
    SensorStampSwitchManuellAutomatic : BOOL := FALSE;
    SensorStampUp : BOOL := FALSE;
    SensorStartButtonStack : BOOL := FALSE;
    SensorWorkpieceReady : BOOL := FALSE;
END_VAR
VAR
    Crane : Crane;
    Stack : Stack;
    Stamp : Stamp;
END_VAR
VAR_OUTPUT
    ActuatorAnalogCranePressure : INT := 0;
    ActuatorCraneLower : BOOL := FALSE;
    ActuatorCraneTurnClockwise : BOOL := FALSE;
    ActuatorCraneTurnCounterClockwise : BOOL := FALSE;
    ActuatorStackGreenLamp : BOOL := FALSE;
    ActuatorStackSlider : BOOL := FALSE;
    ActuatorStackVacuumOff : BOOL := FALSE;
    ActuatorStackVacuumOn : BOOL := FALSE;
    ActuatorStackWhiteLamp : BOOL := FALSE;
    ActuatorSomeActuator : BOOL := FALSE;
    ActuatorStampGreenLamp : BOOL := FALSE;
    ActuatorStampPusher : BOOL := FALSE;
    ActuatorStampSliderMovedIn : BOOL := FALSE;
    ActuatorStampSliderMovedOut : BOOL := FALSE;
    ActuatorStampWhiteLamp : BOOL := FALSE;
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
Crane.CraneUp := SensorCraneUp;
Crane.CraneOnConveyor := SensorCraneOnConveyor;
Crane.CraneDown := SensorCraneDown;
Crane.CraneSucked := SensorCraneSucked;
Crane.CraneOnStack := SensorCraneOnStack;
Crane.StackCapacitiveSensor := SensorStackCapacitiveSensor;
Crane.CranePositionStamp := change(Crane.CranePositionStamp, SensorCranePositionStamp);
Crane.SliderMovedOut := change(Crane.SliderMovedOut, SensorSliderMovedOut);
Crane.WorkpieceReady := SensorWorkpieceReady;
Crane.StampSliderFilled := change(Crane.StampSliderFilled, SensorStampSliderFilled);
Crane.SliderMovedOut := SensorSliderMovedOut;
Crane.StartButtonStack := SensorStartButtonStack;
ActuatorCraneTurnCounterClockwise := Crane.CraneTurnCounterClockwise;
ActuatorCraneTurnClockwise := Crane.CraneTurnClockwise;
ActuatorCraneLower := Crane.CraneLower;
ActuatorStackVacuumOff := Crane.StackVacuumOff;
ActuatorStackVacuumOn := Crane.StackVacuumOn;
Stamp.StampSliderSensorMovedOut := change(Stamp.StampSliderSensorMovedOut, SensorStampSliderSensorMovedOut);
Stamp.StampSliderSensorMovedIn := change(Stamp.StampSliderSensorMovedIn, SensorStampSliderSensorMovedIn);
Stamp.StampSliderFilled := change(Stamp.StampSliderFilled, SensorStampSliderFilled);
Stamp.StampUp := change(Stamp.StampUp, SensorStampUp);
Stamp.StampLowered := change(Stamp.StampLowered, SensorStampLowered);
Stamp.CraneDown := change(Stamp.CraneDown, SensorCraneDown);
Stamp.CraneUp := change(Stamp.CraneUp, SensorCraneUp);
Stamp.CraneSucked := change(Stamp.CraneSucked, SensorCraneSucked);
Stamp.CranePositionStamp := change(Stamp.CranePositionStamp, SensorCranePositionStamp);
Stamp.CraneOnConveyor := change(Stamp.CraneOnConveyor, SensorCraneOnConveyor);
Stamp.StampStartButton := change(Stamp.StampStartButton, SensorStampStartButton);
ActuatorStampSliderMovedOut := change(ActuatorStampSliderMovedOut, Stamp.StampSliderMovedOut);
ActuatorStampSliderMovedIn := change(ActuatorStampSliderMovedIn, Stamp.StampSliderMovedIn);
ActuatorStampWhiteLamp := change(ActuatorStampWhiteLamp, Stamp.StampWhiteLamp);
ActuatorStampPusher := change(ActuatorStampPusher, Stamp.StampPusher);
ActuatorStampGreenLamp := change(ActuatorStampGreenLamp, Stamp.StampGreenLamp);
ActuatorAnalogCranePressure := change(ActuatorAnalogCranePressure, 6000);
IF (SensorStampEmergencyStop AND change(true, SensorStackEmergencyStop)) THEN
    Stack();
    Crane();
    IF change(FALSE, TRUE) THEN
        Stamp();
    END_IF;
    IF (ActuatorStackGreenLamp AND change(true, ActuatorStampGreenLamp)) THEN
        IF ((change(TRUE, Stamp.StartCommandStamp) AND Crane.StartCommandCrane) AND Stack.StartCommandStack) THEN
            Crane.StartVar := TRUE;
            Stack.StartVar := TRUE;
            Stamp.StartVar := change(Stamp.StartVar, TRUE);
        END_IF;
    END_IF;
    Crane.SFCReset := FALSE;
    Stack.SFCReset := FALSE;
    Stamp.SFCReset := change(Stamp.SFCReset, FALSE);
ELSIF (NOT (change(FALSE, SensorStampEmergencyStop)) OR NOT SensorStackEmergencyStop) THEN
    ActuatorStackSlider := FALSE;
    ActuatorCraneLower := FALSE;
    ActuatorStackVacuumOn := FALSE;
    ActuatorStackVacuumOff := TRUE;
    ActuatorStackGreenLamp := FALSE;
    ActuatorCraneTurnCounterClockwise := FALSE;
    ActuatorCraneTurnClockwise := FALSE;
    ActuatorStampSliderMovedOut := change(ActuatorStampSliderMovedOut, TRUE);
    ActuatorStampSliderMovedIn := change(ActuatorStampSliderMovedIn, FALSE);
    ActuatorStampGreenLamp := change(ActuatorStampGreenLamp, FALSE);
    ActuatorStampPusher := change(ActuatorStampPusher, FALSE);
    ActuatorAnalogCranePressure := change(ActuatorAnalogCranePressure, 0);
    Crane.SFCReset := TRUE;
    Stack.SFCReset := TRUE;
    Stamp.SFCReset := change(Stamp.SFCReset, TRUE);
    Crane.StartVar := FALSE;
    Stack.StartVar := FALSE;
    Stamp.StartVar := change(Stamp.StartVar, FALSE);
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