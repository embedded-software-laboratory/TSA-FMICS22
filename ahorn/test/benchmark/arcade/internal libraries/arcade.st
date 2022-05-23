NAMESPACE Arcade

FUNCTION Error
(* This function is handled internally *);
END_FUNCTION

(* Do NOT change ANY name; check hack in PlcIRGenericInstructionVisitor.visit(Call) first! *)
{@CHECK PRECONDITION Condition}
{@CHECK POSTCONDITION Condition}
FUNCTION Assert
VAR_INPUT
  Condition: BOOL;
END_VAR
  IF NOT Condition THEN
    Error();
  END_IF;
END_FUNCTION

{@UNCHECKED POSTCONDITION Condition}
FUNCTION Assume
VAR_INPUT
  Condition: BOOL;
END_VAR
END_FUNCTION

{@DIALECT Codesys}
(* Used internally for implicit pointer casts *)
FUNCTION POINTER_TO_DWORD : DWORD
VAR_INPUT
	pIn : POINTER TO BYTE;
END_VAR

POINTER_TO_DWORD := unknown_dword();

END_FUNCTION

(* Used internally for implicit pointer casts *)
FUNCTION DWORD_TO_POINTER : POINTER TO BYTE
VAR_INPUT
	d : DWORD;
END_VAR

DWORD_TO_POINTER := unknown_pointer();

END_FUNCTION

END_NAMESPACE (* Arcade *)
