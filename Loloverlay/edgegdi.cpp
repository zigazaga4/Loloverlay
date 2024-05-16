#include "edgegdi.hpp"
#include "pattern.hpp"
#include <stdio.h>

uintptr_t edgegdi::g_pEdgeGdi = 0;
uintptr_t edgegdi::own_table[25] = { 0 };

//
// Original functions
edgegdi::BitBlt_t edgegdi::BitBltOrig = nullptr;
edgegdi::CreateDIBSection_t edgegdi::CreateDIBSectionOrig = nullptr;
edgegdi::CreateFontW_t edgegdi::CreateFontWOrig = nullptr;
edgegdi::CreateFontIndirectW_t edgegdi::CreateFontIndirectWOrig = nullptr;
edgegdi::CreateCompatibleDC_t edgegdi::CreateCompatibleDCOrig = nullptr;
edgegdi::CreateDCW_t edgegdi::CreateDCWOrig = nullptr;
edgegdi::CreateICW_t edgegdi::CreateICWOrig = nullptr;
edgegdi::InternalDeleteDC_t edgegdi::InternalDeleteDCOrig = nullptr;
edgegdi::InternalDeleteObject_t edgegdi::InternalDeleteObjectOrig = nullptr;
edgegdi::GetObjectType_t edgegdi::GetObjectTypeOrig = nullptr;
edgegdi::GetObjectW_t edgegdi::GetObjectWOrig = nullptr;
edgegdi::GetStockObject_t edgegdi::GetStockObjectOrig = nullptr;
edgegdi::GetTextMetricsW_t edgegdi::GetTextMetricsWOrig = nullptr;
edgegdi::GetTextExtentPoint32W_t edgegdi::GetTextExtentPoint32WOrig = nullptr;
edgegdi::SelectClipRgn_t edgegdi::SelectClipRgnOrig = nullptr;
edgegdi::SelectObject_t edgegdi::SelectObjectOrig = nullptr;
edgegdi::SetMapMode_t edgegdi::SetMapModeOrig = nullptr;
edgegdi::D3DKMTOpenAdapterFromHdc_t edgegdi::D3DKMTOpenAdapterFromHdcOrig = nullptr;
edgegdi::ExtCreateRegion_t edgegdi::ExtCreateRegionOrig = nullptr;
edgegdi::GdiTrackHCreate_t edgegdi::GdiTrackHCreate = nullptr;


int NotImplemented()
{
    return 1;
}

bool edgegdi::init()
{
    auto gdi32 = (uintptr_t)GetModuleHandleA("gdi32.dll");
    if (!gdi32)
    {
        printf("Failed to get handle for gdi32.dll\n");
        return false;
    }

    g_pEdgeGdi = FindPattern(gdi32, 0xFF000, "\x48\x89\x05\x00\x00\x00\x00\xE9\x00\x00\x00\x00", "xxx????x????", 0);
    if (g_pEdgeGdi)
    {
        uint32_t offset = *(uint32_t*)(g_pEdgeGdi + 3);
        g_pEdgeGdi = g_pEdgeGdi + 7 + offset;
    }

    if (!g_pEdgeGdi)
    {
        printf("Failed to find pattern in gdi32.dll\n");
        return false;
    }

    printf("g_pEdgeGdi: 0x%p\n", (void*)g_pEdgeGdi);
    return g_pEdgeGdi != NULL;
}

bool edgegdi::hook(uintptr_t BitBltHook)
{
    auto gdi32 = GetModuleHandleA("gdi32.dll");
    auto gdi32full = GetModuleHandleA("gdi32full.dll");
    auto win32u = GetModuleHandleA("win32u.dll");

    if (!gdi32full || !gdi32 || !win32u)
    {
        printf("Failed to get necessary module handles\n");
        return false;
    }

    BitBltOrig = (BitBlt_t)GetProcAddress(gdi32full, "BitBlt");
    if (!BitBltOrig)
    {
        printf("Failed to get address of BitBlt\n");
        return false;
    }

    own_table[0] = (uintptr_t)BitBltHook;

    DWORD OldProtect;
    if (!VirtualProtect((LPVOID)g_pEdgeGdi, sizeof(uintptr_t), PAGE_READWRITE, &OldProtect))
    {
        printf("Failed to change memory protection\n");
        return false;
    }

    *(uintptr_t**)g_pEdgeGdi = own_table;
    if (!VirtualProtect((LPVOID)g_pEdgeGdi, sizeof(uintptr_t), OldProtect, &OldProtect))
    {
        printf("Failed to restore memory protection\n");
        return false;
    }

    return true;
}