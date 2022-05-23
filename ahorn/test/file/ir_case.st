PROGRAM P
VAR_INPUT
    x : INT;
END_VAR
VAR_OUTPUT
    code : WORD := 16#0000;
    result : WORD;
END_VAR
    (* Mismatch on purpose. Only first case should be reachable *)
    CASE code OF
    16#0000:
        code := 16#0001;
    16#0001:
        code := 16#0002;
    16#0002:
        code := 16#0003;
    16#0003:
        code := 16#0004;
    16#0004:
        code := 16#0000;
    END_CASE;

    result := code;
END_PROGRAM