#ifndef GUID_UTILS_H
#define GUID_UTILS_H

// All the code below was borrowed and modified from GLFW and is being redistributed under the following license:
// Original link: https://github.com/glfw/glfw/blob/6876cf8d7e0e70dc3e4d7b0224d08312c9f78099/LICENSE.md
// 
//========================================================================
// GLFW 3.4 Win32 - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2019 Camilla Löwy <elmindreda@glfw.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//

// Original: https://github.com/glfw/glfw/blob/6876cf8d7e0e70dc3e4d7b0224d08312c9f78099/src/win32_joystick.c#L193
static bool
IsXInputDevice(const GUID* guid)
{
    RAWINPUTDEVICELIST ridl[512];
    UINT count = ArrayLength(ridl);
    
    count = GetRawInputDeviceList(ridl, &count, sizeof(RAWINPUTDEVICELIST));
    if (count == (UINT)-1)
        return false;
    
    for (UINT i = 0; i < count; i++)
    {
        RID_DEVICE_INFO rdi = { .cbSize = sizeof(rdi), };
        char name[256] = { 0 };
        UINT rdi_size = sizeof rdi;
        UINT name_size = sizeof name;
        
        if (RIM_TYPEHID != ridl[i].dwType)
            continue;
        if ((UINT)-1 == GetRawInputDeviceInfoA(ridl[i].hDevice, RIDI_DEVICEINFO, &rdi, &rdi_size))
            continue;
        if (guid->Data1 != MAKELONG(rdi.hid.dwVendorId, rdi.hid.dwProductId))
            continue;
        
        if ((UINT)-1 == GetRawInputDeviceInfoA(ridl[i].hDevice, RIDI_DEVICENAME, name, &name_size))
            break;
        
        name[sizeof(name) - 1] = '\0';
        if (MemoryStrstr(name, "IG_"))
            return true;
    }
    
    return false;
}

// Original: https://github.com/glfw/glfw/blob/6876cf8d7e0e70dc3e4d7b0224d08312c9f78099/src/win32_joystick.c#L452
static bool
ConvertGuidToSDLGuid(const DIDEVICEINSTANCEW* instance, char* guid_str, uintsize guid_str_size)
{
    if (MemoryCompare(&instance->guidProduct.Data4[2], "PIDVID", 6) == 0)
    {
        StringPrintfBuffer(guid_str, guid_str_size, "03000000%02x%02x0000%02x%02x000000000000",
		(uint8) instance->guidProduct.Data1,
		(uint8) (instance->guidProduct.Data1 >> 8),
		(uint8) (instance->guidProduct.Data1 >> 16),
		(uint8) (instance->guidProduct.Data1 >> 24));
    }
    else
    {
        char name[256];
		
		// Generate a joystick GUID that matches the SDL 2.0.5+ one
		if (!WideCharToMultiByte(CP_UTF8, 0, instance->tszInstanceName, -1, name, sizeof(name), NULL, NULL))
			return false;
		
		StringPrintfBuffer(guid_str, guid_str_size, "05000000%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x00",
			name[0], name[1], name[2], name[3],
			name[4], name[5], name[6], name[7],
			name[8], name[9], name[10]);
    }
    
    return true;
}

#endif //GUID_UTILS_H
