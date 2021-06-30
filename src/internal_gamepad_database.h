#ifndef INTERNAL_GAMEPAD_DATABASE_H
#define INTERNAL_GAMEPAD_DATABASE_H

typedef uint8 GamepadObject;
enum GamepadObject
{
	GamepadObject_Null = 0,
	
	GamepadObject_Button_A,
	GamepadObject_Button_B,
	GamepadObject_Button_X,
	GamepadObject_Button_Y,
	GamepadObject_Button_Left,
	GamepadObject_Button_Right,
	GamepadObject_Button_Up,
	GamepadObject_Button_Down,
	GamepadObject_Button_LeftShoulder, // LB
	GamepadObject_Button_RightShoulder, // RB
	GamepadObject_Button_LeftStick, // LS
	GamepadObject_Button_RightStick, // RS
	GamepadObject_Button_Start,
	GamepadObject_Button_Back,
	
	GamepadObject_Pressure_LeftTrigger, // LT
	GamepadObject_Pressure_RightTrigger, // RT
	
	GamepadObject_Axis_LeftX,
	GamepadObject_Axis_LeftY,
	GamepadObject_Axis_RightX,
	GamepadObject_Axis_RightY,
	
	GamepadObject_Count,
};

struct GamepadMappings
{
	uint64 guid[2];
	
	GamepadObject buttons[32];
	GamepadObject axes[16];
	GamepadObject povs[8][4];
} typedef GamepadMappings;

internal GamepadMappings global_gamepadmap_default = {
	.buttons = {
		GamepadObject_Button_Y,
		GamepadObject_Button_B,
		GamepadObject_Button_A,
		GamepadObject_Button_X,
		GamepadObject_Button_LeftShoulder,
		GamepadObject_Button_RightShoulder,
		GamepadObject_Pressure_LeftTrigger,
		GamepadObject_Pressure_RightTrigger,
		GamepadObject_Button_Back,
		GamepadObject_Button_Start,
		GamepadObject_Button_LeftStick,
		GamepadObject_Button_RightStick,
		0
	},
	.axes = {
		GamepadObject_Axis_LeftY,
		GamepadObject_Axis_LeftX,
		GamepadObject_Axis_RightY,
		GamepadObject_Axis_RightX,
		0
	},
	.povs = {
		{
			GamepadObject_Button_Up,
			GamepadObject_Button_Right,
			GamepadObject_Button_Down,
			GamepadObject_Button_Left,
		},
		{ 0 },
	},
};

#endif //INTERNAL_GAMEPAD_DATABASE_H
