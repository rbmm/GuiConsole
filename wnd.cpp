#include "stdafx.h"

_NT_BEGIN

#include "wnd.h"

ULONG STDMETHODCALLTYPE SimpWndCls::AddRef()
{
	return InterlockedIncrementNoFence(&_M_dwRef);
}

ULONG STDMETHODCALLTYPE SimpWndCls::Release()
{
	if (ULONG dwRef = InterlockedDecrement(&_M_dwRef))
	{
		return dwRef;
	}
	delete this;
	return 0;
}

HWND SimpWndCls::Create(
						_In_ DWORD dwExStyle, 
						_In_opt_ PCWSTR lpWindowName, 
						_In_ DWORD dwStyle, 
						_In_ int X, 
						_In_ int Y, 
						_In_ int nWidth, 
						_In_ int nHeight, 
						_In_opt_ HWND hWndParent, 
						_In_opt_ HMENU hMenu, 
						_In_opt_ HINSTANCE hInstance)
{
	return CreateWindowExW(dwExStyle, ClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, this);
}

LRESULT CALLBACK SimpWndCls::_WindowProc(
	HWND hwnd, 
	UINT uMsg, 
	WPARAM wParam, 
	LPARAM lParam 
	)
{
	return reinterpret_cast<SimpWndCls*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA))->wWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT SimpWndCls::wWindowProc(
	HWND hwnd, 
	UINT uMsg, 
	WPARAM wParam, 
	LPARAM lParam 
	)
{
	++_M_dwMessageCount;

	lParam = WindowProc(hwnd, uMsg, wParam, lParam);

	if (!--_M_dwMessageCount)
	{
		AfterLastMessage();
		Release();
	}

	return lParam;
}

LRESULT CALLBACK SimpWndCls::__WindowProc(
	HWND hwnd, 
	UINT uMsg, 
	WPARAM wParam, 
	LPARAM lParam )
{
	if (uMsg == WM_NCCREATE)
	{
		PVOID lpCreateParams = reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(lpCreateParams));
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LPARAM>(_WindowProc));
		reinterpret_cast<SimpWndCls*>(lpCreateParams)->AddRef();
		reinterpret_cast<SimpWndCls*>(lpCreateParams)->_M_dwMessageCount = 1 << 31;
		return reinterpret_cast<SimpWndCls*>(lpCreateParams)->WindowProc(hwnd, uMsg, wParam, lParam);
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT SimpWndCls::WindowProc(
							   HWND hwnd, 
							   UINT uMsg, 
							   WPARAM wParam, 
							   LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCDESTROY:
		_bittestandreset(&_M_dwMessageCount, 31);
		break;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

ATOM SimpWndCls::Register()
{
	const static WNDCLASSW wcls = { 0, __WindowProc, 0, 0, (HINSTANCE)&__ImageBase, 0, 0, 0, 0, ClassName };

	return RegisterClassW(&wcls);
}

void SimpWndCls::Unregister()
{
	if (!UnregisterClassW(ClassName, (HINSTANCE)&__ImageBase))
	{
		if (ERROR_CLASS_DOES_NOT_EXIST != GetLastError())
		{
			__debugbreak();
		}
	}
}

_NT_END