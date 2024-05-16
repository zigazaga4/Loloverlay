#include <Windows.h>
#include <stdio.h>
#include "edgegdi.hpp"

BOOL BitBltHook(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop)
{
    auto CurrentPid = GetCurrentProcessId();
    auto CurrentTid = GetCurrentThreadId();

    char buf[256] = { 0 };
    sprintf_s(buf, sizeof(buf), "BitBltHook called: (PID: %i, TID: %i) (X: %i (%i), Y: %i (%i)) (X: %i, Y: %i) (%X, %X)\n",
        CurrentPid, CurrentTid, x, cx, y, cy, x1, y1, hdc, hdcSrc);
    OutputDebugStringA(buf);

    RECT rect = { x, y, x + cx, y + cy };
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);

    return edgegdi::BitBltOrig(hdc, x, y, cx, cy, hdcSrc, x1, y1, rop);
}

int main()
{
    SetConsoleTitleA("EDGEGDI.EXE");
    printf("Initializing hook...\n");

    if (!edgegdi::init() || !edgegdi::hook((uintptr_t)BitBltHook))
    {
        printf("Failed to hook BitBlt.\n");
        return -1;
    }

    printf("Hook installed. Running test BitBlt call...\n");
    BitBlt((HDC)0xB16B00B5, 0, 0, 0, 0, (HDC)0xCAFEBABE, 0, 0, 0);

    getchar();
    return 0;
}