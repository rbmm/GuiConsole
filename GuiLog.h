#pragma once

PCWSTR WINAPI GetWindowName();

PCWSTR WINAPI GetIconName();

HINSTANCE WINAPI GetIconModule();

BOOL WINAPI ep_work(_In_ HWND hwnd);