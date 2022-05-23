TYPE
    Magazin_states_t : (Green_Lamp , Init , Interstep , Magazin_Init , Magazin_Init_2 , Slider_Move_Back , Start_Magazin , Step0 , Step1 , convey);
END_TYPE

PROGRAM P
VAR
    x : INT;
    State : Magazin_states_t;
END_VAR
State := Magazin_states_t#Interstep;
CASE State OF
    Magazin_states_t#Green_Lamp:
        x := 0;

    Magazin_states_t#Init:
        x := 1;

    Magazin_states_t#Interstep:
        x := 2;

    Magazin_Init:
        x := 3;

    Magazin_Init_2:
        x := 4;

    Slider_Move_Back:
        x := 5;

    Start_Magazin:
        x := 6;

    Step0:
        x := 7;

    Step1:
        x := 8;

    convey:
        x := 9;
END_CASE;
END_PROGRAM