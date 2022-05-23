PROGRAM cyclic
(* Moegliche Zustaende fuer den Automaten *)
VAR CONSTANT
	stateAfterBoot : BYTE := 0;
	stateInit : BYTE := 1;
	stateWaitForItem : BYTE := 2;
	stateWaitForCompressor1 : BYTE := 3;
	stateExtractItem1 : BYTE := 4;
	stateWaitForCompressor2 : BYTE := 5;
	stateExtractItem2 : BYTE := 6;
	stateRotateTable1 : BYTE := 7;
	stateWaitForCompressor3 : BYTE := 8;
	statePunchItem : BYTE := 9;	
	stateRotateTable2 : BYTE := 10;
	stateWaitForCompressor4 : BYTE := 11;
	stateMoveToConveyer1 : BYTE := 12;
	stateWaitForCompressor5 : BYTE := 13;
	stateMoveToConveyer2 : BYTE := 14;
	stateRunConveyer : BYTE := 15;
	stateMoveToRobot : BYTE := 16;
	stateTurnToItem : BYTE := 17;
	stateTakeItem : BYTE := 18;
	stateTurnToMagazin : BYTE := 19;
	stateError : BYTE := 20;
END_VAR
(* Vordefinierte Konstanten fuer den gesamten Ablauf *)
VAR CONSTANT
	pickPosTurn : BYTE := 111;
	pickPosVert : BYTE := 80;
	pickPosArm : BYTE := 28;
	pickPosClaw : BYTE := 19;
	dropPosTurn : BYTE := 71;
	dropPosArm : BYTE := 37;
	moveThreshold : BYTE := 36;
	compressorDelay : DINT := 50;
	extractionDelay : DINT := 50;
	conveyerDelay : DINT := 50;
	waitForItemThreshold : DINT := 100;
	waitForPuncherThreshold : DINT := 10;
END_VAR
(* Virtuelle Speicherstellen der Sensoren / Motoren *)
VAR_INPUT
	(* 3D-Roboter *)
	turnDestPos : BYTE;
	turnCurrPos : BYTE;
	vertDestPos  : BYTE;
	vertCurrPos : BYTE;
	armDestPos  : BYTE;
	armCurrPos   : BYTE;
	clawDestPos  : BYTE;
	clawCurrPos  : BYTE;
	(* Bearbeitungszentrum *)
	compressor : BYTE;
	tableMotPow  : BYTE;
	tableMotDir  : BYTE;
	magazinLed  : BYTE;
	magazinPhoto : BYTE;
	magazinPusherOn  : BYTE;
	magazinPusherOff : BYTE;
	conveyerMotPow  : BYTE;
	conveyerMotDir  : BYTE;
	conveyerLed  : BYTE;
	conveyerPhoto  : BYTE;
	conveyerButton : BYTE;
	conveyerPusherOn  : BYTE;
	conveyerPusherOff  : BYTE;
	puncherButton  : BYTE;
	puncher  : BYTE;
	
	flagDelay : DINT;
END_VAR

(* Verwendete Ein- und Ausgaenge *)
VAR_OUTPUT
	outState : BYTE;
	outError : BOOL := false;
	DiagCode : BYTE := 16#0000;
END_VAR

	(* Motoren einmalig zur Nullposition fahren *)
	IF DiagCode = stateAfterBoot THEN
		
		compressor := 0;
		
		tableMotPow := 0;
		tableMotDir := 0;

		magazinLed := 0;
		magazinPusherOn := 0;
		magazinPusherOff := 0;
		
		conveyerMotPow := 0;
		conveyerMotDir := 0;
		conveyerLed := 0;
		conveyerPusherOn := 0;
		conveyerPusherOff := 0;
		
		turnDestPos := 0;
		vertDestPos := 0;
		armDestPos := 0;
		clawDestPos := 0;
		
		IF turnCurrPos = 0 AND armCurrPos = 0 AND clawCurrPos = 0 THEN
			DiagCode := stateInit;
		END_IF
	
	(* Anlage vor dem Start initialisieren *)
	ELSIF DiagCode = stateInit THEN
		
		conveyerLed := 1;
		magazinLed := 1;
		
		turnDestPos := dropPosTurn;
		armDestPos := dropPosArm;
		
		IF turnCurrPos = dropPosTurn AND armCurrPos = dropPosArm AND vertCurrPos = 0 THEN
			IF clawCurrPos = 0 THEN
				DiagCode := stateWaitForItem;
			ELSE
				clawDestPos := 0;
			END_IF
		END_IF
	
	(* Warten, bis sich ein Werkstueck im Magazin befindet *)
	ELSIF DiagCode = stateWaitForItem THEN
	
		IF flagDelay = waitForItemThreshold THEN
			DiagCode := stateInit;
		ELSIF magazinPhoto = 1 THEN
			magazinLed := 0;
			DiagCode := stateWaitForCompressor1;
		END_IF
	
	(* Kompressor zum Ausfahren des Magazinschiebers vorbereiten *)
	ELSIF DiagCode = stateWaitForCompressor1 THEN
	
		compressor := 1;
		
		IF flagDelay >= compressorDelay THEN
			compressor := 0;
			DiagCode := stateExtractItem1;
		END_IF
	
	(* Werkstueck aus dem Magazin entnehmen - Teil 1 *)
	ELSIF DiagCode = stateExtractItem1 THEN

		magazinPusherOn := 1;
		
		IF flagDelay >= extractionDelay THEN
			magazinPusherOn := 0;
			DiagCode := stateWaitForCompressor2;
		END_IF
		
	(* Kompressor zum Einfahren des Magazinschiebers vorbereiten *)
	ELSIF DiagCode = stateWaitForCompressor2 THEN
	
		compressor := 1;
		
		IF flagDelay >= compressorDelay THEN
			compressor := 0;
			DiagCode := stateExtractItem2;
		END_IF
	
	(* Werkstueck aus dem Magazin entnehmen - Teil 2 *)
	ELSIF DiagCode = stateExtractItem2 THEN
	
		magazinPusherOff := 1;
		
		IF flagDelay >= extractionDelay THEN
			magazinPusherOff := 0;
			DiagCode := stateRotateTable1;
		END_IF
	
	(* Drehtisch aktivieren und Stanze-Schalter ueberwachen *)
	ELSIF DiagCode = stateRotateTable1 THEN
	
		tableMotPow := 200;
		(* something went wrong: we skip punching *)
		IF flagDelay > waitForPuncherThreshold THEN
			tableMotPow := 0;
			DiagCode := stateRotateTable2;
		ELSIF puncherButton = 0 THEN
			tableMotPow := 0;
			DiagCode := stateWaitForCompressor3;
		END_IF
		
	(* Kompressor zum Stanzen vorbereiten *)
	ELSIF DiagCode = stateWaitForCompressor3 THEN
	
		compressor := 1;
		
		IF flagDelay >= compressorDelay THEN
			compressor := 0;
			DiagCode := statePunchItem;
		END_IF
	
	(* Werkstueck stanzen *)
	ELSIF DiagCode = statePunchItem THEN
		
		puncher := 1;
		
		IF flagDelay >= extractionDelay THEN
			puncher := 0;
			DiagCode := stateRotateTable2;
		END_IF
	
	(* Drehtisch aktivieren und Transportband-Schalter ueberwachen *)
	ELSIF DiagCode = stateRotateTable2 THEN
	
		tableMotPow := 200;
	
		IF conveyerButton = 0 THEN
			tableMotPow := 0;
			DiagCode := stateWaitForCompressor4;
		END_IF	
	(* Kompressor zum Einfahren des Transportschiebers vorbereiten *)
	ELSIF DiagCode = stateWaitForCompressor4 THEN
	
		compressor := 1;
		
		IF flagDelay >= compressorDelay THEN
			compressor := 0;
			DiagCode := stateMoveToConveyer1;
		END_IF
	
	(* Werkstueck auf das Fliessband befoerdern - Teil 1 *)
	ELSIF DiagCode = stateMoveToConveyer1 THEN
	
		conveyerPusherOn := 1;
		
		IF flagDelay >= extractionDelay THEN
			conveyerPusherOn := 0;
			DiagCode := stateWaitForCompressor5;
		END_IF
	
	(* Kompressor zum Ausfahren des Transportschiebers vorbereiten *)
	ELSIF DiagCode = stateWaitForCompressor5 THEN
	
		compressor := 1;
		
		IF flagDelay >= compressorDelay THEN
			compressor := 0;
			DiagCode := stateMoveToConveyer2;
		END_IF
	
	(* Werkstueck auf das Fliessband befoerdern - Teil 2 *)
	ELSIF DiagCode = stateMoveToConveyer2 THEN
	
		conveyerPusherOff := 1;
		
		IF flagDelay >= extractionDelay THEN
			conveyerPusherOff := 0;
			DiagCode := stateRunConveyer;
		END_IF
	
	(* Fliessband aktivieren und auf das Werkstueck warten *)
	ELSIF DiagCode = stateRunConveyer THEN
		
		conveyerMotPow := 200;
		(*Something went wrong, we again try to put the item on the tape *)
		IF flagDelay = waitForItemThreshold THEN
			DiagCode := stateMoveToConveyer1;
		ELSIF conveyerPhoto = 1 THEN
			conveyerLed := 0;
			DiagCode := stateMoveToRobot;
		END_IF
	
	(* Drehen, Greifarm und Greifer ausstrecken *)
	ELSIF DiagCode = stateMoveToRobot THEN
		
		IF flagDelay > conveyerDelay THEN
			conveyerMotPow := 0;
			DiagCode := stateTurnToItem;
		END_IF
	
	(* Drehen, Greifarm und Greifer ausstrecken *)
	ELSIF DiagCode = stateTurnToItem THEN

		turnDestPos := pickPosTurn;
		vertDestPos := pickPosVert;
		armDestPos := pickPosArm;
		
		IF turnCurrPos = pickPosTurn AND vertCurrPos = pickPosVert AND armCurrPos = pickPosArm THEN
			DiagCode := stateTakeItem;
		END_IF
		
	(* Werkstueck vom Fliessband greifen *)
	ELSIF DiagCode = stateTakeItem THEN
	
		clawDestPos := pickPosClaw;
		
		IF clawCurrPos = pickPosClaw THEN
			DiagCode := stateTurnToMagazin;
		END_IF
	
	(* Das Werkstueck zum Magazin drehen *)
	ELSIF DiagCode = stateTurnToMagazin THEN
	
		vertDestPos := 0;
		
		IF vertCurrPos < moveThreshold THEN
			DiagCode := stateInit;
		END_IF
	
	(* Fehlerzustand - alles ausschalten *)
	ELSE
	
		outError := true;
		
		compressor := 0;
		puncher := 0;
		
		tableMotPow := 0;
		tableMotDir := 2;
		
		conveyerMotPow := 0;
		conveyerMotDir := 2;
	
	(* Ende des Zustandsautomaten *)
	END_IF

	(* Ausgabe vorbereiten *)
	outState := DiagCode;
	
END_PROGRAM