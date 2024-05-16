#pragma once
#include <stdint.h>
#include <Windows.h>
#include <iostream>

bool DataCompare(const uint8_t* OpCodes, const uint8_t* Mask, const char* StrMask)
{
    while (*StrMask)
    {
        if (*StrMask == 'x' && *OpCodes != *Mask)
            return false;

        ++StrMask;
        ++OpCodes;
        ++Mask;
    }

    return true;
}

uintptr_t FindPattern(uintptr_t StartAddress, uint32_t CodeLen, const char* Mask, const char* StrMask, unsigned short ignore = 0)
{
    uint16_t Ign = 0;
    uint32_t i = 0;

    while (Ign <= ignore)
    {
        if (IsBadReadPtr((void*)(StartAddress + i), 1))
        {
            std::cerr << "Access violation at: " << std::hex << (StartAddress + i) << std::endl;
            return 0;
        }

        if (DataCompare((uint8_t*)(StartAddress + i), (uint8_t*)Mask, StrMask))
        {
            ++Ign;
        }
        else
        {
            ++i;
        }

        if (i >= CodeLen)
            return 0;
    }

    return StartAddress + i - 1;
}