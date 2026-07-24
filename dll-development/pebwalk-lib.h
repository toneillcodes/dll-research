// pebwalk-lib.h
#pragma once
#include <windows.h>

__declspec(dllexport)
void WalkPEB(void);

__declspec(dllexport)
void DemoKernel32Exports(void);

PVOID GetPebAddressGeneric(HANDLE hProcess);
PVOID GetModuleBaseManual(PPEB pebObject, const char* targetModuleName);
PVOID GetModuleBaseManualGeneric(HANDLE hProcess, PVOID pebAddr, const char* targetModuleName);
BOOL ReadMemoryInternal(HANDLE hProcess, PVOID baseAddress, PVOID localBuffer, SIZE_T size);