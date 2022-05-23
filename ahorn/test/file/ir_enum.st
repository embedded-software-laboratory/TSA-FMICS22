TYPE
    Magazin_states_t : (Green_Lamp , Init , Interstep , Magazin_Init , Magazin_Init_2 , Slider_Move_Back , Start_Magazin , Step0 , Step1 , convey);
END_TYPE

PROGRAM P
VAR
    State : Magazin_states_t := Green_Lamp;
    x : INT;
END_VAR
    CASE State OF
    Green_Lamp:
        x := 1;
    Init:
        x := x + 1;
    Interstep:
        x := 2;
    Magazin_Init,
    Magazin_Init_2,
    Slider_Move_Back:
        x := 3;
    Start_Magazin,
    Step0:
        x := 4;
    Step1:
        x := 5;
    convey:
        x := 6;
    END_CASE;
END_PROGRAM