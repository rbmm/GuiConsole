#include "stdafx.h"
#include <Cpl.h>

_NT_BEGIN

#include "console.h"

typedef struct CONSOLE_STATE_INFO {
    /* BEGIN V1 CONSOLE_STATE_INFO */
    COORD ScreenBufferSize;
    COORD WindowSize;
    INT WindowPosX;
    INT WindowPosY;
    COORD FontSize;
    UINT FontFamily;
    UINT FontWeight;
    WCHAR FaceName[LF_FACESIZE];
    UINT CursorSize;
    UINT FullScreen : 1;
    UINT QuickEdit : 1;
    UINT AutoPosition : 1;
    UINT InsertMode : 1;
    UINT HistoryNoDup : 1;
    UINT FullScreenSupported : 1;
    UINT UpdateValues : 1;
    UINT Defaults : 1;
    WORD ScreenAttributes;
    WORD PopupAttributes;
    UINT HistoryBufferSize;
    UINT NumberOfHistoryBuffers;
    COLORREF ColorTable[16];
    HWND hWnd;
    HICON hIcon;
    LPWSTR OriginalTitle;
    LPWSTR LinkTitle;

    /*
     * Starting code page
     */
    UINT CodePage;

    /* END V1 CONSOLE_STATE_INFO */

    /* BEGIN V2 CONSOLE_STATE_INFO */
    BOOL fIsV2Console;
    BOOL fWrapText;
    BOOL fFilterOnPaste;
    BOOL fCtrlKeyShortcutsDisabled;
    BOOL fLineSelection;
    BYTE bWindowTransparency;
    BOOL fWindowMaximized;

    unsigned int CursorType;
    COLORREF CursorColor;

    BOOL InterceptCopyPaste;

    COLORREF DefaultForeground;
    COLORREF DefaultBackground;
    BOOL TerminalScrolling;
    /* END V2 CONSOLE_STATE_INFO */

} *PCONSOLE_STATE_INFO;

NTSTATUS GetRegDWORD(_In_ HANDLE hKey, PCWSTR pszValueName, _Out_ PULONG Value)
{
	UNICODE_STRING ValueName;
	RtlInitUnicodeString(&ValueName, pszValueName);

	KEY_VALUE_PARTIAL_INFORMATION kvpi;

	NTSTATUS status = ZwQueryValueKey(hKey, &ValueName, KeyValuePartialInformation, &kvpi, sizeof(kvpi), &kvpi.TitleIndex);

	if (0 > status)
	{
		return status;
	}

	if (kvpi.Type != REG_DWORD)
	{
		return STATUS_OBJECT_TYPE_MISMATCH;
	}

	if (sizeof(ULONG) != kvpi.DataLength)
	{
		return STATUS_INFO_LENGTH_MISMATCH;
	}

	*Value = *(ULONG*)kvpi.Data;

	return STATUS_SUCCESS;
}

NTSTATUS GetRegSZ(_In_ HANDLE hKey, 
				  _In_ PCWSTR pszValueName, 
				  _Out_writes_bytes_(Length) PKEY_VALUE_PARTIAL_INFORMATION kvpi,
				  _In_ ULONG Length,
				  _Out_writes_bytes_(cbValue) PWSTR pszValue,
				  _In_ ULONG cbValue)
{
	UNICODE_STRING ValueName;
	RtlInitUnicodeString(&ValueName, pszValueName);

	NTSTATUS status = ZwQueryValueKey(hKey, &ValueName, KeyValuePartialInformation, kvpi, Length, &Length);

	if (0 > status)
	{
		return status;
	}

	if (kvpi->Type != REG_SZ)
	{
		return STATUS_OBJECT_TYPE_MISMATCH;
	}

	if ((Length = kvpi->DataLength) & (sizeof(WCHAR) - 1) ||
		*(WCHAR*)RtlOffsetToPointer(kvpi->Data, Length - sizeof(WCHAR)))
	{
		return STATUS_BAD_DATA;
	}

	if (Length > cbValue)
	{
		return STATUS_BUFFER_OVERFLOW;
	}

	memcpy(pszValue, kvpi->Data, Length);

	return STATUS_SUCCESS;
}

COLORREF GetColor(HKEY hKey, ULONG i)
{
	static const COLORREF _S_ColorTable[] = {
		0x00000000,
		0x00A50000,
		0x00549B5A,
		0x00808000,
		0x003755B1,
		0x008E17DF,
		0x00008080,
		0x00C0C0C0,
		0x00808080,
		0x00FF9600,
		0x0000FF00,
		0x00FFFF00,
		0x000000FF,
		0x00FF00FF,
		0x0000FFFF,
		0x00FFFFFF,
	};

	COLORREF color = _S_ColorTable[i & (_countof(_S_ColorTable) - 1)];
	
	WCHAR buf[32];
	if (0 < swprintf_s(buf, _countof(buf), L"ColorTable%02u", i))
	{
		GetRegDWORD(hKey, buf, &color);
	}

	return color;
}

void InitFontInfo(_Out_ PCONSOLE_STATE_INFO StateInfo, _In_ HFONT hFont)
{
	LOGFONTW lf;
	if (hFont && sizeof(lf) == GetObjectW(hFont, sizeof(lf), &lf))
	{
		wcscpy(StateInfo->FaceName, lf.lfFaceName);
		StateInfo->FontSize.X = (SHORT)lf.lfWidth;
		StateInfo->FontSize.Y = (SHORT)(0 > lf.lfHeight ? -lf.lfHeight : lf.lfHeight);
		StateInfo->FontWeight = lf.lfWeight;
	}
}

void InitFontDefaults(_Out_ PLOGFONTW lf)
{
	NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
	{
		lf->lfHeight = -ncm.iMenuHeight;
		lf->lfWeight = FW_NORMAL;
		lf->lfQuality = CLEARTYPE_QUALITY;
		lf->lfPitchAndFamily = FIXED_PITCH|FF_MODERN;
	}

	wcscpy(lf->lfFaceName, L"Courier New");
}

void ConsoleWnd::SetConsoleState(_In_ PLOGFONTW lf)
{
	if (_M_hbr)
	{
		DeleteObject(_M_hbr);
	}

	switch (_M_Background)
	{
	case 0:
		_M_hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
		break;
	case RGB(MAXUCHAR, MAXUCHAR, MAXUCHAR):
		_M_hbr = (HBRUSH)GetStockObject(WHITE_BRUSH);
		break;
	default:
		_M_hbr = CreateSolidBrush(_M_Background);
	}

	if (HFONT hFont = CreateFontIndirectW(lf))
	{
		if (_M_hFont)
		{
			DeleteObject(_M_hFont);
		}

		_M_hFont = hFont;
		SendMessageW(_M_hwndEdit, WM_SETFONT, (WPARAM)hFont, FALSE);
	}
}

void ConsoleWnd::LoadConsoleState(_Out_ PLOGFONTW lf)
{
	InitFontDefaults(lf);

	HKEY hKey;

	if (NOERROR == RegOpenKeyExW(HKEY_CURRENT_USER, L"Console", 0, KEY_READ, &hKey))
	{
		union {
			KEY_VALUE_PARTIAL_INFORMATION kvpi;
			UCHAR buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(LOGFONTW::lfFaceName)];
		};

		union {
			ULONG ScreenAttributes;
			COORD FontSize;
			ULONG FontWeight;
			ULONG FontFamily;
		};

		if (0 <= GetRegDWORD(hKey, L"ScreenColors", &ScreenAttributes) && ScreenAttributes < MAXUCHAR)
		{
			_M_ScreenAttributes = ScreenAttributes;
			_M_Foreground = GetColor(hKey, ScreenAttributes & 0xF);
			_M_Background = GetColor(hKey, ScreenAttributes >> 4);
		}

		GetRegSZ(hKey, L"FaceName", &kvpi, sizeof(buf), lf->lfFaceName, sizeof(lf->lfFaceName));

		if (0 <= GetRegDWORD(hKey, L"FontSize", &ScreenAttributes))
		{
			ULONG lfHeight = 0 > lf->lfHeight ? -lf->lfHeight : +lf->lfHeight;
			ULONG Y = 0 > FontSize.Y ? -FontSize.Y : +FontSize.Y;

			if ((ULONG)FontSize.X < Y && Y <= (lfHeight << 1) && Y >= (lfHeight >> 1) )
			{
				lf->lfHeight = FontSize.Y;
				lf->lfWidth = FontSize.X;
			}
		}

		if (0 <= GetRegDWORD(hKey, L"FontWeight", &FontWeight) && FontWeight <= FW_HEAVY)
		{
			lf->lfWeight = FontWeight;
		}

		if (0 <= GetRegDWORD(hKey, L"FontFamily", &FontFamily) && (FontFamily &= ~0xF) <= FF_DECORATIVE)
		{
			lf->lfPitchAndFamily = FIXED_PITCH|(BYTE)FontFamily;
		}

		NtClose(hKey);
	}
}

void ConsoleWnd::SetDefaults(HWND hwnd)
{
	if (HMODULE hmod = LoadLibraryW(L"console.dll"))
	{
		union {
			APPLET_PROC CPlApplet;
			PVOID __imp_CPlApplet;
		};

		if (__imp_CPlApplet = GetProcAddress(hmod, "CPlApplet"))
		{
			CONSOLE_STATE_INFO StateInfo{};

			StateInfo.hWnd = hwnd;
			StateInfo.hIcon = _M_hii[0];
			StateInfo.OriginalTitle = const_cast<PWSTR>(L"OriginalTitle");
			StateInfo.Defaults = TRUE;

			StateInfo.ScreenAttributes = (USHORT)_M_ScreenAttributes;
			InitFontInfo(&StateInfo, _M_hFont);

			CPlApplet(hwnd, CPL_INIT, 0, 0);
			CPlApplet(hwnd, CPL_DBLCLK, (LPARAM)&StateInfo, 0);
			CPlApplet(hwnd, CPL_EXIT, 0, 0);

			if (StateInfo.UpdateValues)
			{
				_M_ScreenAttributes = StateInfo.ScreenAttributes;
				_M_Background = StateInfo.ColorTable[StateInfo.ScreenAttributes >> 4];
				_M_Foreground = StateInfo.ColorTable[StateInfo.ScreenAttributes & 0xF];

				LOGFONTW lf {};
				wcscpy(lf.lfFaceName, StateInfo.FaceName);
				lf.lfPitchAndFamily = (BYTE)((StateInfo.FontFamily << 4) | FIXED_PITCH);
				lf.lfHeight = StateInfo.FontSize.Y;
				lf.lfWidth = StateInfo.FontSize.X;
				lf.lfWeight = StateInfo.FontWeight;
				lf.lfQuality = CLEARTYPE_QUALITY;

				SetConsoleState(&lf);

				InvalidateRect(_M_hwndEdit, 0, TRUE);
			}
		}

		FreeLibrary(hmod);
	}
}

_NT_END