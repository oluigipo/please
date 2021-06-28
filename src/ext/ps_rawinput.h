//
// https://gist.github.com/mmozeiko/c136c1cfce9fe4267f3c8f7b90f8e4d4
//

#pragma once

#include <windows.h>
#include <hidsdi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>

// PlayStation DualShock4 and DualSense controller input through Windows raw input

#define PS_BUTTON_S       0x000010
#define PS_BUTTON_X       0x000020
#define PS_BUTTON_O       0x000040
#define PS_BUTTON_T       0x000080
#define PS_BUTTON_L1      0x000100
#define PS_BUTTON_R1      0x000200
#define PS_BUTTON_L2      0x000400
#define PS_BUTTON_R2      0x000800
#define PS_BUTTON_SHARE   0x001000
#define PS_BUTTON_OPTIONS 0x002000
#define PS_BUTTON_L3      0x004000
#define PS_BUTTON_R3      0x008000
#define PS_BUTTON_PS      0x010000
#define PS_BUTTON_TPAD    0x020000 // click on touchpad
#define PS_BUTTON_MUTE    0x040000 // only on DualSense

#define PS_DPAD_UP    0x01000000
#define PS_DPAD_DOWN  0x02000000
#define PS_DPAD_RIGHT 0x04000000
#define PS_DPAD_LEFT  0x08000000

typedef struct {
    bool active;
    uint8_t id;
    uint16_t x; // [0..1920)
    uint16_t y; // [0..944) for DualShock4, [0..1080) for DualSense
} ps_touch;

typedef struct {
    bool wired;
    bool dualsense;
    uint64_t serial;    // only bytes 0..5
    uint64_t timestamp; // microseconds
	
    int8_t left[2];
    int8_t right[2];
    uint8_t l2;
    uint8_t r2;
    uint32_t buttons;
	
    int16_t gyro[3];
    int16_t accel[3];
	
    bool charging;
    uint8_t battery; // 0..100 %
	
    ps_touch touch[2];
} ps_state;

typedef struct {
    uint8_t left_rumble;   // 0 = off, 255 = max
    uint8_t right_rumble;  // 0 = off, 255 = max
    uint8_t color[3];      // { R, G, B }
	
    // items below are DualSense only
    bool mic_led;
    uint8_t player_led;   // 0=off, 1..5=on
} ps_output;

// initialize raw input
static bool ps_init(HWND window);

// call on WM_INPUT_DEVICE_CHANGE message
// returns > 0 = index for connected gamepad
// returns < 0 = -index for disconnected gamepad
// otherwise returns 0 (api error, or not a DS4/DS controller)
static int ps_update(WPARAM wparam, LPARAM lparam);

// call on WM_INPUT message
// returns > 0 = index which gamepad was updated
// otherwise returns 0 if this message is not for DS4/DS controller
static int ps_input(LPARAM lparam);

// gets current coontroller state, call any time after ps_input
// returned pointer is stable, does not change
static const ps_state* ps_get(int index);

// sets vibration/leds/color
// do not call this very often (like on each input packet)
// calling this once a frame or something like every 10msec is OK
static void ps_set(int index, const ps_output* out);

// IMPLEMENTATION

// feel free to set this define to any number you want
#ifndef PS_MAX_DEVICES
#define PS_MAX_DEVICES 16
#endif

typedef struct {
    HANDLE raw;
    HANDLE file;
    bool connected;
    uint32_t tsc_prev;
    uint64_t tsc;
    ps_state state;
} ps_handle;

// [0] is never used
static ps_handle ps_handles[PS_MAX_DEVICES + 1];

bool ps_init(HWND window)
{
    ZeroMemory(ps_handles, sizeof(ps_handles));
	
    RAWINPUTDEVICE rid;
    rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid.usUsage = HID_USAGE_GENERIC_GAMEPAD;
    rid.dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    rid.hwndTarget = window;
	
    return !!RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

int ps_update(WPARAM wparam, LPARAM lparam)
{
    HANDLE raw = (HANDLE)lparam;
	
    if (wparam == GIDC_ARRIVAL)
    {
        ps_handle* handle = NULL;
        for (int i = 1; i <= PS_MAX_DEVICES; i++)
        {
            if (ps_handles[i].raw == NULL)
            {
                handle = ps_handles + i;
                break;
            }
        }
        if (handle == NULL)
        {
            // too many devices connected, ignore this one
            return 0;
        }
		
        ZeroMemory(handle, sizeof(*handle));
		
        RID_DEVICE_INFO info = { 0 };
        UINT info_size = sizeof(info);
        if (GetRawInputDeviceInfoW(raw, RIDI_DEVICEINFO, &info, &info_size) < 0)
        {
            // can not get info about device
            return 0;
        }
		
        uint32_t vid = info.hid.dwVendorId;
        uint32_t pid = info.hid.dwProductId;
        if (vid == 0x054c && pid == 0x05c4)
        {
            // DualShock4
        }
        else if (vid == 0x054c && pid == 0x0ce6)
        {
            // DualSense
        }
        else
        {
            // not supported
            return 0;
        }
		
        WCHAR name[MAX_PATH];
        UINT name_size = sizeof(name);
        if (GetRawInputDeviceInfoW(raw, RIDI_DEVICENAME, &name, &name_size) > 0)
        {
            handle->file = CreateFileW(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (handle->file != INVALID_HANDLE_VALUE)
            {
                handle->state.dualsense = pid == 0x0ce6;
				
                if (handle->state.dualsense)
                {
                    BYTE s[64];
                    s[0] = 9;
                    if (HidD_GetFeature(handle->file, s, sizeof(s))) // it also enables 0x31 reports for BT connection
                    {
                        for (int i = 0; i < 6; i++)
                        {
                            handle->state.serial |= (uint64_t)s[i + 1] << (i * 8);
                        }
                    }
                }
                else
                {
                    WCHAR ws[64];
                    if (HidD_GetSerialNumberString(handle->file, ws, sizeof(ws))) // this works for BT connection
                    {
                        for (int i = 0; i < 12; i++)
                        {
                            char hex[2] = { (char)ws[i], 0 };
                            handle->state.serial |= strtoll(hex, NULL, 16) << ((11 - i) * 4);
                        }
                    }
                    else
                    {
                        BYTE s[16];
                        s[0] = 18;
                        if (HidD_GetFeature(handle->file, s, sizeof(s))) // this works for USB connection
                        {
                            for (int i = 0; i < 6; i++)
                            {
                                handle->state.serial |= (uint64_t)s[i + 1] << (i * 8);
                            }
                        }
                    }
                }
				
                int index = (int)(handle - ps_handles);
				
                if (handle->state.serial != 0)
                {
                    // check that device with same serial is not already connected
                    // this will happen if you do BT connection with controller that
                    // is also connected to same PC with USB
                    for (int other = 1; other <= PS_MAX_DEVICES; other++)
                    {
                        if (other != index && ps_handles[other].raw != NULL)
                        {
                            if (ps_handles[other].state.serial == handle->state.serial)
                            {
                                // same device found;
                                CloseHandle(handle->file);
                                return 0;
                            }
                        }
                    }
                }
				
                handle->raw = raw;
                return index;
            }
        }
    }
    else if (wparam == GIDC_REMOVAL)
    {
        ps_handle* handle = NULL;
        for (int i = 1; i <= PS_MAX_DEVICES; i++)
        {
            if (ps_handles[i].raw == raw)
            {
                handle = ps_handles + i;
                break;
            }
        }
        if (handle != NULL)
        {
            CloseHandle(handle->file);
            handle->raw = NULL;
            return -(int)(handle - ps_handles);
        }
    }
	
    return 0;
}

int ps_input(LPARAM lparam)
{
    HRAWINPUT input = (HRAWINPUT)lparam;
	
    UINT input_size;
    if (GetRawInputData(input, RID_INPUT, NULL, &input_size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
    {
        return 0;
    }
	
    RAWINPUT* input_data = (RAWINPUT*)_alloca(input_size);
    if (GetRawInputData(input, RID_INPUT, input_data, &input_size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
    {
        return 0;
    }
	
    ps_handle* handle = NULL;
    for (int i = 1; i <= PS_MAX_DEVICES; i++)
    {
        if (ps_handles[i].raw == input_data->header.hDevice)
        {
            handle = ps_handles + i;
            break;
        }
    }
    if (handle == NULL)
    {
        return 0;
    }
    ps_state* state = &handle->state;
	
    const BYTE* data = input_data->data.hid.bRawData;
    DWORD size = input_data->data.hid.dwSizeHid;
	
    if (size < 64)
    {
        return 0;
    }
	
    if (data[0] == 0x01) // DualShock4 or DualSense wired
    {
        state->wired = true;
        data += 1;
        size -= 1;
		
    }
    else if (data[0] == 0x11) // DualShock4 BT
    {
        state->wired = false;
        data += 3;
        size -= 3;
    }
    else if (data[0] == 0x31) // DualSense BT
    {
        state->wired = false;
        data += 2;
        size -= 2;
    }
    else
    {
        // unknown format
        return 0;
    }
	
    state->left[0] = data[0] - 128;
    state->left[1] = data[1] - 128;
    state->right[0] = data[2] - 128;
    state->right[1] = data[3] - 128;
	
    uint8_t battery;
	
    bool dualsense = state->dualsense;
    if (dualsense)
    {
        state->l2 = data[4];
        state->r2 = data[5];
		
        const BYTE* buttons = data + 7;
        state->buttons = buttons[0] | (buttons[1] << 8) | ((buttons[2] & 0xf) << 16);
		
        const BYTE* gyro = data + 15;
        state->gyro[0] = gyro[0] | (gyro[1] << 8);
        state->gyro[1] = gyro[2] | (gyro[3] << 8);
        state->gyro[2] = gyro[4] | (gyro[5] << 8);
		
        const BYTE* accel = data + 21;
        state->accel[0] = accel[0] | (accel[1] << 8);
        state->accel[1] = accel[2] | (accel[3] << 8);
        state->accel[2] = accel[4] | (accel[5] << 8);
		
        battery = data[52];
		
        uint32_t tsc = data[27] | (data[28] << 8) | (data[29] << 16) | (data[30] << 24);
        if (handle->tsc == 0)
        {
            handle->tsc = tsc;
        }
        else
        {
            uint64_t delta;
            if (handle->tsc_prev > tsc)
            {
                delta = (1ULL<<32) + tsc - handle->tsc_prev;
            }
            else
            {
                delta = tsc - handle->tsc_prev;
            }
            handle->tsc += delta;
        }
        state->timestamp = handle->tsc / 3;
        handle->tsc_prev = tsc;
    }
    else
    {
        const BYTE* buttons = data + 4;
        state->buttons = buttons[0] | (buttons[1] << 8) | ((buttons[2] & 0x3) << 16);
		
        state->l2 = data[7];
        state->r2 = data[8];
		
        const BYTE* gyro = data + 12;
        state->gyro[0] = gyro[0] | (gyro[1] << 8);
        state->gyro[1] = gyro[2] | (gyro[3] << 8);
        state->gyro[2] = gyro[4] | (gyro[5] << 8);
		
        const BYTE* accel = data + 18;
        state->accel[0] = accel[0] | (accel[1] << 8);
        state->accel[1] = accel[2] | (accel[3] << 8);
        state->accel[2] = accel[4] | (accel[5] << 8);
		
        battery = data[29];
		
        uint32_t tsc = data[9] | (data[10] << 8);
        if (handle->tsc == 0)
        {
            handle->tsc = tsc;
        }
        else
        {
            uint32_t delta;
            if (handle->tsc_prev > tsc)
            {
                delta = (1ULL<<16) + tsc - handle->tsc_prev;
            }
            else
            {
                delta = tsc - handle->tsc_prev;
            }
            handle->tsc += delta;
        }
        state->timestamp = (handle->tsc * 1000) / 188;
        handle->tsc_prev = tsc;
    }
	
    state->charging = ((battery >> 4) & 1) != 0;
    battery = battery & 0xf;
    state->battery = (battery > 9) ? 100 : (uint8_t)(battery * 10 + 5);
	
    static const uint32_t dpad_values[] = {
        PS_DPAD_UP,
        PS_DPAD_UP | PS_DPAD_RIGHT,
        PS_DPAD_RIGHT,
        PS_DPAD_RIGHT | PS_DPAD_DOWN,
        PS_DPAD_DOWN,
        PS_DPAD_DOWN | PS_DPAD_LEFT,
        PS_DPAD_LEFT,
        PS_DPAD_LEFT | PS_DPAD_UP,
    };
    uint32_t dpad = state->buttons & 0xf;
    if (dpad < 8)
    {
        state->buttons |= dpad_values[dpad];
    }
	
    const BYTE* touch_data = data + (dualsense ? 31 : 33);
    for (int n = 0; n < 2; n++)
    {
        ps_touch* t = state->touch + n;
        t->active = (touch_data[1] & 0x80) == 0;
        t->id = touch_data[1] & 0x7f;
        t->x = touch_data[2] | ((touch_data[3] & 0xf) << 8);
        t->y = (touch_data[3] >> 4) | (touch_data[4] << 4);
        touch_data += dualsense ? 4 : 5;
    }
	
    return (int)(handle - ps_handles);
}

const ps_state* ps_get(int index)
{
    return ps_handles[index].raw ? &ps_handles[index].state : NULL;
}

// faster way would be to use lookup table
static DWORD ps_crc32(const BYTE* data, DWORD size, DWORD initial)
{
    DWORD r = ~initial;
    for (DWORD i=0; i<size; i++)
    {
        r ^= *data++;
        for (int i = 0; i < 8; i++)
        {
            r = (r >> 1) ^ (r & 1 ? 0xedb88320 : 0);
        }
    }
    return ~r;
}

void ps_set(int index, const ps_output* out)
{
    if (ps_handles[index].raw == NULL)
    {
        return;
    }
	
    ps_handle* handle = ps_handles + index;
	
    uint8_t data[78] = { 0 };
    uint32_t size;
	
    if (handle->state.dualsense)
    {
        int offset;
        if (handle->state.wired)
        {
            data[0] = 0x02;
            offset = 1;
            size = 48;
        }
        else
        {
            data[0] = 0x31;
            data[1] = 0x02;
            offset = 2;
            size = 74;
        }
		
        data[offset + 0] = 0x03;
        data[offset + 1] = 0x15;
        data[offset + 2] = out->right_rumble;
        data[offset + 3] = out->left_rumble;
        data[offset + 8] = (out->mic_led ? 1 : 0);
        data[offset + 38] = 2;
        data[offset + 41] = 2;
        data[offset + 44] = out->color[0];
        data[offset + 45] = out->color[1];
        data[offset + 46] = out->color[2];
        if (out->player_led >= 1 && out->player_led <= 5)
        {
            data[offset + 43] = 1 << (out->player_led - 1);
        }
        data[offset + 43] |= 0x20;
    }
    else
    {
        int offset;
        if (handle->state.wired)
        {
            data[0] = 0x05;
            data[1] = 0x0f;
            offset = 4;
            size = 32;
        }
        else
        {
            data[0] = 0x11;
            data[1] = 0xc4;
            data[3] = 0x03;
            offset = 6;
            size = 74;
        }
		
        data[offset + 0] = out->right_rumble;
        data[offset + 1] = out->left_rumble;
        data[offset + 2] = out->color[0];
        data[offset + 3] = out->color[1];
        data[offset + 4] = out->color[2];
    }
	
    if (!handle->state.wired)
    {
        BYTE header = 0xa2;
        DWORD crc = ps_crc32(&header, 1, 0);
        crc = ps_crc32(data, size, crc);
        CopyMemory(data + size, &crc, sizeof(crc));
        size += sizeof(crc);
    }
	
    // TODO: this can be turned into nonblocking/async write with OVERLAPPED
    DWORD written;
    WriteFile(handle->file, data, size, &written, NULL);
}
