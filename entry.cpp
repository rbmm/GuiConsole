#include "stdafx.h"
_NT_BEGIN
#include "../inc/initterm.h"
#include "console.h"
#include "wlog.h"
#include "GuiLog.h"

void ConsoleWnd::OnDestroy()
{
	if (_M_hbr)
	{
		DeleteObject(_M_hbr);
	}

	if (_M_hFont)
	{
		DeleteObject(_M_hFont);
	}

	ULONG i = _countof(_M_hii);
	do 
	{
		if (HICON hi = _M_hii[--i])
		{
			DestroyIcon(hi);
		}
	} while (i);
}

BOOL ConsoleWnd::OnCreate(HWND hwndParent)
{
	RECT rc;
	GetClientRect(hwndParent, &rc);

	if (HWND hwnd = CreateWindowExW(0, WC_EDIT, 0, WS_CHILD|WS_HSCROLL|WS_VSCROLL|ES_MULTILINE|WS_VISIBLE,
		0, 0, rc.right, rc.bottom, hwndParent, 0, 0, 0))
	{
		_M_hwndEdit = hwnd;

		static const int 
			X_index[] = { SM_CXSMICON, SM_CXICON }, 
			Y_index[] = { SM_CYSMICON, SM_CYICON },
			icon_type[] = { ICON_SMALL, ICON_BIG};

		ULONG i = _countof(icon_type) - 1;

		HICON hi;
		do 
		{
			if (0 <= LoadIconWithScaleDown(GetIconModule(), GetIconName(), 
				GetSystemMetrics(X_index[i]), GetSystemMetrics(Y_index[i]), &hi))
			{
				_M_hii[i] = hi;
				SendMessage(hwndParent, WM_SETICON, icon_type[i], (LPARAM)hi);
			}
		} while (i--);

		ULONG n = 8;
		SendMessage(hwnd, EM_SETTABSTOPS, 1, (LPARAM)&n);
		SendMessage(hwnd, EM_LIMITTEXT, 0, 0);

		if (HMENU hmenu = GetSystemMenu(hwndParent, FALSE))
		{
			AppendMenu(hmenu, MF_STRING, SC_DEFAULT, L"Properties");
		}

		LOGFONTW lf {};
		LoadConsoleState(&lf);
		SetConsoleState(&lf);

		return TRUE;
	}

	return FALSE;
}

BOOL ConsoleWnd::OnWindowPosChanged(HWND hwnd, PWINDOWPOS pwp)
{
	if (!(pwp->flags & SWP_NOSIZE))
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		MoveWindow(_M_hwndEdit, 0, 0, rc.right, rc.bottom, FALSE);
	}

	return TRUE;
}

void ConsoleWnd::OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	if (BeginPaint(hwnd, &ps))
	{
		EndPaint(hwnd, &ps);
	}
}

LRESULT ConsoleWnd::WindowProc(
							   HWND hwnd, 
							   UINT uMsg, 
							   WPARAM wParam, 
							   LPARAM lParam 
							   )
{
	switch (uMsg)
	{
	case WM_NCDESTROY:
		PostQuitMessage(0);
		break;

	case WM_DESTROY:
		OnDestroy();
		break;

	case WM_CREATE:
		if (!OnCreate(hwnd))
		{
			return -1;
		}
		break;

	case WM_WINDOWPOSCHANGED:
		if (OnWindowPosChanged(hwnd, reinterpret_cast<PWINDOWPOS>(lParam)))
		{
			return 0;
		}
		break;

	case WM_ERASEBKGND:
		return TRUE;

	case WM_PAINT:
		OnPaint(hwnd);
		return 0;

	case WM_CTLCOLOREDIT:
		if ((HWND)lParam == _M_hwndEdit)
		{
			SetTextColor((HDC)wParam, _M_Foreground);
			SetBkColor((HDC)wParam, _M_Background);
			SetBkMode((HDC)wParam, OPAQUE);
			return (LPARAM)_M_hbr;
		}
		break;

	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_DEFAULT:
			SetDefaults(hwnd);
			break;
		}
		break;

	case WM_CLOSE:
		if (_M_bCanCloseFrame)
		{
			DestroyWindow(hwnd);
		}
		return 0;

	case WM_KEEP:
		_M_bCanCloseFrame = (wParam == 0);
		return 0;

	case WM_FLUSH:
		SetEditHandle(_M_hwndEdit, wParam);
		return 0;

	case WM_FPRINT:
		SendMessageW(_M_hwndEdit, EM_SETSEL, (WPARAM)-1, 0);
		SendMessageW(_M_hwndEdit, EM_REPLACESEL, 0, lParam);
		LocalFree((PWSTR)lParam);
		return 0;
	}

	return __super::WindowProc(hwnd, uMsg, wParam, lParam);
}

void CALLBACK ep(void*)
{
	initterm();
	if (SimpWndCls::Register())
	{
		if (ConsoleWnd* wnd = new ConsoleWnd)
		{
			HWND hwnd = wnd->Create(0, GetWindowName(), WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, 
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0);
			
			wnd->Release();

			if (hwnd)
			{
				if (ep_work(hwnd))
				{
					ShowWindow(hwnd, SW_SHOW);
					MSG msg;
					while (0 < GetMessageW(&msg, 0, 0, 0))
					{
						TranslateMessage(&msg);
						DispatchMessageW(&msg);
					}
				}
				else
				{
					DestroyWindow(hwnd);
				}
			}
		}
		SimpWndCls::Unregister();
	}
	destroyterm();
	ExitProcess(0);
}

_NT_END