/* Instantiate the official render interface GUID declarations once. Keeping
   these scoped avoids DXGUID.LIB's aggregate object duplicating the existing
   official DirectPlay GUID definitions in bh_network_transport.c. */
#include <windows.h>
#include <initguid.h>
#include <ddraw.h>
#include <d3d.h>
