#ifndef INTERNAL_GAMEPAD_MAP_H
#define INTERNAL_GAMEPAD_MAP_H

typedef uint16 GamepadObject;
enum GamepadObject
{
	GamepadObject_Null = 0,
	
	GamepadObject_Button_A = 1,
	GamepadObject_Button_B = 2,
	GamepadObject_Button_X = 3,
	GamepadObject_Button_Y = 4,
	GamepadObject_Button_Left = 5,
	GamepadObject_Button_Right = 6,
	GamepadObject_Button_Up = 7,
	GamepadObject_Button_Down = 8,
	GamepadObject_Button_LeftShoulder = 9, // LB
	GamepadObject_Button_RightShoulder = 10, // RB
	GamepadObject_Button_LeftStick = 11, // LS
	GamepadObject_Button_RightStick = 12, // RS
	GamepadObject_Button_Start = 13,
	GamepadObject_Button_Back = 14,
	
	GamepadObject_Pressure_LeftTrigger = 15, // LT
	GamepadObject_Pressure_RightTrigger = 16, // RT
	
	GamepadObject_Axis_LeftX = 17,
	GamepadObject_Axis_LeftY = 18,
	GamepadObject_Axis_RightX = 19,
	GamepadObject_Axis_RightY = 20,
	
	// The lower bits of this enum is one of those entries specified above.
	// The higher bits are used to store extra information about the object.
	//
	// Higher Bits: 7654_3210
	// 7 = If the axis should be inverted
	// 6 = If the input axis should be limited to 0..MAX
	// 5 = If the input axis should be limited to MIN..0
	// 4 = If the output axis should be limited to 0..MAX
	// 3 = If the output axis should be limited to MIN..0
	//
	// Bits 6 & 5 should never be set at the same time
	// Bits 4 & 3 should never be set at the same time
	
	GamepadObject_Count = 21,
};

struct GamepadMappings
{
	char guid[32];
	
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

#include "internal_gamepad_map_database.inc"

#endif //INTERNAL_GAMEPAD_MAP_H
