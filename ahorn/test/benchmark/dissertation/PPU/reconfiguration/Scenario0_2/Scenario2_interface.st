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
END_PROGRAM