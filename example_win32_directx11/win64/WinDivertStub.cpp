#include "windivert.h"

HANDLE WinDivertOpen(const char* filter, WINDIVERT_LAYER layer, INT16 priority, UINT64 flags) {
    return INVALID_HANDLE_VALUE;
}

BOOL WinDivertRecv(HANDLE handle, VOID* pPacket, UINT packetLen, UINT* pRecvLen, WINDIVERT_ADDRESS* pAddr) {
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL WinDivertSend(HANDLE handle, const VOID* pPacket, UINT packetLen, UINT* pSendLen, const WINDIVERT_ADDRESS* pAddr) {
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL WinDivertClose(HANDLE handle) {
    return TRUE;
}
