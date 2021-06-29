// NOTE(ljre): needed by 'platform_win32_input.c'
#define IID_IDirectInput8W _def_IID_IDirectInput8W
internal const GUID IID_IDirectInput8W = {0xBF798031,0x483A,0x4DA2,{0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00}};

// NOTE(ljre): needed by 'c_dfDIJoystick'
#define GUID_XAxis _def_GUID_XAxis
#define GUID_YAxis _def_GUID_YAxis
#define GUID_ZAxis _def_GUID_ZAxis
#define GUID_RxAxis _def_GUID_RxAxis
#define GUID_RyAxis _def_GUID_RyAxis
#define GUID_RzAxis _def_GUID_RzAxis
#define GUID_Slider _def_GUID_Slider
#define GUID_POV _def_GUID_POV
internal const GUID GUID_XAxis = {0xA36D02E0,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
internal const GUID GUID_YAxis = {0xA36D02E1,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
internal const GUID GUID_ZAxis = {0xA36D02E2,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
internal const GUID GUID_RxAxis = {0xA36D02F4,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
internal const GUID GUID_RyAxis = {0xA36D02F4,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
internal const GUID GUID_RzAxis = {0xA36D02E3,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
internal const GUID GUID_Slider = {0xA36D02E4,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
internal const GUID GUID_POV = {0xA36D02F2,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};

// NOTE(ljre): because CLANG
#ifndef DIDFT_OPTIONAL
#define DIDFT_OPTIONAL		0x80000000
#endif

// NOTE(ljre): Generated with https://github.com/elmindreda/c_dfDIJoystick2
//
//             clang is too dumb to compile it - GCC worked for me:
//                 gcc c_dfDIJoystick2.c -o test.exe -ldinput8 -lrpcrt4
//
internal DIOBJECTDATAFORMAT c_dfDIJoystick_data_format[] = {
	{ &GUID_XAxis,DIJOFS_X,DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,DIDOI_ASPECTPOSITION },
	{ &GUID_YAxis,DIJOFS_Y,DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,DIDOI_ASPECTPOSITION },
	{ &GUID_ZAxis,DIJOFS_Z,DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,DIDOI_ASPECTPOSITION },
	{ &GUID_RxAxis,DIJOFS_RX,DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,DIDOI_ASPECTPOSITION },
	{ &GUID_RyAxis,DIJOFS_RY,DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,DIDOI_ASPECTPOSITION },
	{ &GUID_RzAxis,DIJOFS_RZ,DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,DIDOI_ASPECTPOSITION },
	{ &GUID_Slider,DIJOFS_SLIDER(0),DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,DIDOI_ASPECTPOSITION },
	{ &GUID_Slider,DIJOFS_SLIDER(1),DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,DIDOI_ASPECTPOSITION },
	{ &GUID_POV,DIJOFS_POV(0),DIDFT_POV|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ &GUID_POV,DIJOFS_POV(1),DIDFT_POV|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ &GUID_POV,DIJOFS_POV(2),DIDFT_POV|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ &GUID_POV,DIJOFS_POV(3),DIDFT_POV|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(0),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(1),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(2),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(3),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(4),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(5),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(6),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(7),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(8),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(9),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(10),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(11),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(12),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(13),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(14),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(15),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(16),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(17),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(18),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(19),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(20),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(21),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(22),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(23),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(24),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(25),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(26),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(27),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(28),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(29),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(30),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
	{ NULL,DIJOFS_BUTTON(31),DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,0 },
};

#define c_dfDIJoystick _def_c_dfDIJoystick
internal const DIDATAFORMAT c_dfDIJoystick = {
    sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT),
    DIDFT_ABSAXIS, sizeof(DIJOYSTATE),
    sizeof(c_dfDIJoystick_data_format) / sizeof(DIOBJECTDATAFORMAT),
    c_dfDIJoystick_data_format,
};