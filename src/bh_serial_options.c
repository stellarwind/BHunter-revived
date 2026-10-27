#include "bh_serial_options.h"
#include <string.h>
static const int baud_values[12] = {1200, 2400, 4800, 9600, 14400, 19200, 38400, 56000, 57600, 115200, 128000, 256000};
static const char *const baud_labels[12] = {"1200 bits/sec",  "2400 bits/sec",   "4800 bits/sec",   "9600 bits/sec",
                                            "14400 bits/sec", "19200 bits/sec",  "38400 bits/sec",  "56000 bits/sec",
                                            "57600 bits/sec", "115200 bits/sec", "128000 bits/sec", "256000 bits/sec"};
/*004759c0: exact supported values; unsupported input yields NULL. */
const char *__cdecl bh_serial_baud_label(int value)
{
    int i;
    for (i = 0; i < 12; ++i)
        if (value == baud_values[i])
            return baud_labels[i];
    return NULL;
}
/*00475a80: unsupported values reset to9600, not the first rate. */
int __cdecl bh_serial_baud_next(int value)
{
    int i;
    for (i = 0; i < 12; ++i)
        if (value == baud_values[i])
            return baud_values[(i + 1) % 12];
    return 9600;
}
/*00475b40/00475b60: DirectPlay stop constants are0=one and2=two. */
const char *__cdecl bh_serial_stop_label(int value)
{
    if (value == 0)
        return "One";
    if (value == 2)
        return "Two";
    return NULL;
}
int __cdecl bh_serial_stop_next(int value)
{
    return value == 0 ? 2 : 0;
}
/*00475b70/00475bb0: parity cycle uses original DirectPlay values. */
const char *__cdecl bh_serial_parity_label(int value)
{
    static const char *const labels[4] = {"None", "Odd", "Even", "Mark"};
    return value >= 0 && value < 4 ? labels[value] : NULL;
}
int __cdecl bh_serial_parity_next(int value)
{
    return value >= 0 && value < 3 ? value + 1 : 0;
}
/*00475bf0/00475c40: all original flow names, including combined RTS/DTR. */
const char *__cdecl bh_serial_flow_label(int value)
{
    static const char *const labels[5] = {"None", "Software Xon/Xoff", "Hardware RTS", "Hardware DTR",
                                          "Hardware RTS/DTR"};
    return value >= 0 && value < 5 ? labels[value] : NULL;
}
int __cdecl bh_serial_flow_next(int value)
{
    return value >= 1 && value <= 4 ? (value + 1) % 5 : 1;
}
/*00475c80: original MSVCRT case-insensitive ANSI comparison. */
int __cdecl bh_modem_name_find(const BHModemNames *names, const char *name)
{
    int i;
    if (!names || !name || names->count < 0 || names->count > 8)
        return -1;
    for (i = 0; i < names->count; ++i)
        if (_stricmp(name, names->names[i]) == 0)
            return i;
    return -1;
}
/*00475cd0: DPAID_Modem bytes pinned from original0048a1c8. This is
   address-element enumeration, not service-provider/session enumeration. */
BOOL CALLBACK bh_modem_address_element(const GUID *type, DWORD size, const void *data, void *context)
{
    static const unsigned char modem_id[16] = {0x00, 0xc2, 0xdc, 0xf6, 0xfe, 0xa2, 0xd0, 0x11,
                                               0x9c, 0x4f, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e};
    BHModemNames *names = (BHModemNames *)context;
    const char *name = (const char *)data;
    int length;
    (void)size;
    if (!type || memcmp(type, modem_id, 16) != 0)
        return TRUE;
    if (!names || !name || names->count < 0 || names->count > 8)
        return TRUE;
    while ((length = lstrlenA(name)) != 0 && names->count < 8)
    {
        /* Added malformed-input guard: original strcpy would overrun a slot. */
        if (length >= 128)
            return TRUE;
        memcpy(names->names[names->count], name, length + 1);
        ++names->count;
        name += length + 1;
    }
    return TRUE;
}
