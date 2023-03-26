#pragma once

PCWSTR WINAPI GetWindowName();

PCWSTR WINAPI GetIconName();

HINSTANCE WINAPI GetIconModule();

BOOL WINAPI BeginML(_In_ HWND hwnd, _Out_ void** pContext);

void WINAPI EndML(_In_ void* Context);