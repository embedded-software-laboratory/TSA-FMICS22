PROGRAM DiagnosticsConcept
USING PLCopenX;

  VAR_INPUT
	S1_S_EStopIn_1 : SAFEBOOL;
	S1_S_EStopIn_2 : SAFEBOOL;
	S2_S_ESPE_In : SAFEBOOL;
	S0_Reset : BOOL;
	S3_Drive_Reset : BOOL;
	AxisD_1 : INT;
	InputDevice1_active : BOOL;
	InputDevice2_active : BOOL;
	Internal_Acknowledge: BOOL;
  END_VAR
  VAR_OUTPUT
    Error_Equiv1 : BOOL;
	Diag_Equiv1 : WORD;
	Error_EStop1 : BOOL;
	Diag_EStop1 : WORD;
	Error_ESPE1 : BOOL;
	Diag_ESPE1 : WORD;
	Error_SafeStop1 : BOOL;
	Diag_SafeStop1 : WORD;
    S_Stopped : BOOL;
  END_VAR
  VAR
  	S_EStopOut : SAFEBOOL;
	SF_Equivalent_1 : SF_EquivalentX;
	SF_EmergencyStop_1 : SF_EmergencyStopX;
	SF_ESPE_1 : SF_ESPEX;
	SF_SafeStop1_1 : SF_SafeStop1X;
  END_VAR
  
  SF_Equivalent_1(InputDevice1_active, S1_S_EStopIn_1, S1_S_EStopIn_2, T#10ms);
  SF_EmergencyStop_1(SF_Equivalent_1.Ready, SF_Equivalent_1.S_EquivalentOut, FALSE, FALSE, S0_Reset);
  SF_ESPE_1(InputDevice2_active, S2_S_ESPE_In, FALSE, FALSE, S0_Reset);
  SF_SafeStop1_1(
    Activate:=SF_EmergencyStop_1.Ready AND SF_ESPE_1.Ready,
    S_StopIn:=S_EStopOut AND SF_ESPE_1.S_ESPE_Out,
    AxisID:=AxisD_1,
    MonitoringTime:=T#100ms,
    Reset:=S3_Drive_Reset,
    Acknowledge := Internal_Acknowledge);
  S_EStopOut := SF_EmergencyStop_1.S_EStopOut;
  Error_Equiv1 := SF_Equivalent_1.Error;
  Diag_Equiv1 := SF_Equivalent_1.DiagCode;
  Error_EStop1 := SF_EmergencyStop_1.Error;
  Diag_EStop1 := SF_EmergencyStop_1.DiagCode;
  Error_ESPE1 := SF_ESPE_1.Error;
  Diag_ESPE1 := SF_ESPE_1.DiagCode;
  Error_SafeStop1 := SF_SafeStop1_1.Error;
  Diag_SafeStop1 := SF_SafeStop1_1.DiagCode;
  S_Stopped := SF_SafeStop1_1.S_Stopped;
END_PROGRAM