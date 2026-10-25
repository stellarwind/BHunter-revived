#ifndef BH_SERIAL_OPTIONS_H
#define BH_SERIAL_OPTIONS_H
#include <windows.h>
/* Eight original 128-byte modem-name slots. Source context replaces the
   original process-global list and is passed through DirectPlay callback data. */
typedef struct BHModemNames { int count; char names[8][128]; } BHModemNames;
const char *__cdecl bh_serial_baud_label(int value);
int __cdecl bh_serial_baud_next(int value);
const char *__cdecl bh_serial_stop_label(int value);
int __cdecl bh_serial_stop_next(int value);
const char *__cdecl bh_serial_parity_label(int value);
int __cdecl bh_serial_parity_next(int value);
const char *__cdecl bh_serial_flow_label(int value);
int __cdecl bh_serial_flow_next(int value);
int __cdecl bh_modem_name_find(const BHModemNames *names,const char *name);
/* Exact four-argument stdcall callback ABI (original RET 0x10).
   As with original, size is unused: data is a double-NUL-terminated ANSI list.
   Valid original inputs have names <=127 bytes and a count in [0,8]. */
BOOL CALLBACK bh_modem_address_element(const GUID *type,DWORD size,const void *data,void *context);
#endif
