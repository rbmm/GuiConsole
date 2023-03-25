#pragma once

#include "wnd.h"

class ConsoleWnd : public SimpWndCls
{
	HWND _M_hwndEdit = 0;
	HFONT _M_hFont = 0;
	HICON _M_hii[2]{};
	HBRUSH _M_hbr = 0;
	COLORREF _M_Background = 0;
	COLORREF _M_Foreground = RGB(0, MAXUCHAR, 0);
	DWORD _M_ScreenAttributes = 0x0A;
	BOOLEAN _M_bCanCloseFrame = TRUE;

	void SetConsoleState(_In_ PLOGFONTW lf);

	void LoadConsoleState(_Out_ PLOGFONTW lf);

	void OnDestroy();

	BOOL OnCreate(HWND hwndParent);

	BOOL OnWindowPosChanged(HWND hwnd, PWINDOWPOS pwp);

	void OnPaint(HWND hwnd);

	void SetDefaults(HWND hwnd);

	virtual LRESULT WindowProc(
		HWND hwnd, 
		UINT uMsg, 
		WPARAM wParam, 
		LPARAM lParam 
		);
};
