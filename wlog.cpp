#include "stdafx.h"

_NT_BEGIN

#include "wlog.h"

namespace {
	HMODULE ghnt;
}

void SetEditHandle(HWND hwnd, WPARAM wParam)
{
	HLOCAL hMem = (HLOCAL)SendMessage(hwnd, EM_GETHANDLE, 0, 0);

	SendMessage(hwnd, EM_SETHANDLE, wParam, 0);

	LocalFree(hMem);
}

void WLog::Flush(HWND hwnd)
{
	SetEditHandle(hwnd, (WPARAM)_BaseAddress);
	_BaseAddress = 0;
}

void WLog::operator >> (HWND hwnd)
{
	if (GetWindowThreadProcessId(hwnd, 0) == GetCurrentThreadId())
	{
		SendMessageW(hwnd, WM_FLUSH, (WPARAM)_BaseAddress, 0);
	}
	else
	{
		PostMessageW(hwnd, WM_FLUSH, (WPARAM)_BaseAddress, 0);
	}

	_BaseAddress = 0;
}

ULONG WLog::Init(SIZE_T RegionSize)
{
	if (PVOID BaseAddress = LocalAlloc(LMEM_FIXED, RegionSize))
	{
		_RegionSize = (ULONG)RegionSize, _Ptr = 0, _BaseAddress = BaseAddress;
		*(WCHAR*)BaseAddress = 0;
		return NOERROR;
	}
	return GetLastError();
}

WLog::~WLog()
{
	if (_BaseAddress)
	{
		LocalFree(_BaseAddress);
	}
}

WLog& WLog::operator ()(PCWSTR format, ...)
{
	va_list args;
	va_start(args, format);

	int len = _vsnwprintf_s(_buf(), _cch(), _TRUNCATE, format, args);

	if (0 < len)
	{
		_Ptr += len;
	}

	va_end(args);

	return *this;
}

WLog& WLog::operator << (PCWSTR str)
{
	if (!wcscpy_s(_buf(), _cch(), str))
	{
		_Ptr += (ULONG)wcslen(str);
	}
	return *this;
}

WLog& WLog::operator ()(_In_reads_bytes_(cbBinary) CONST BYTE *pbBinary,
						_In_ DWORD cbBinary,
						_In_ DWORD dwFlags)
{
	PWSTR psz = _buf();
	ULONG cch = _cch();
	if (CryptBinaryToStringW(pbBinary, cbBinary, dwFlags, psz, &cch))
	{
		_Ptr += cch;
	}
	return *this;
}

WLog& WLog::operator[](HRESULT dwError)
{
	LPCVOID lpSource = 0;
	ULONG dwFlags = FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS;

	if ((dwError & FACILITY_NT_BIT) || (0 > dwError && HRESULT_FACILITY(dwError) == FACILITY_NULL))
	{
		dwError &= ~FACILITY_NT_BIT;
__nt:
		dwFlags = FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_IGNORE_INSERTS;

		if (!ghnt && !(ghnt = GetModuleHandle(L"ntdll"))) return *this;
		lpSource = ghnt;
	}

	if (dwFlags = FormatMessageW(dwFlags, lpSource, dwError, 0, _buf(), _cch(), 0))
	{
		_Ptr += dwFlags;
	}
	else if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
	{
		goto __nt;
	}
	return *this;
}

WNoBufLog& WNoBufLog::operator()(PCWSTR format, ...)
{
	va_list args;
	va_start(args, format);

	int len = 0;
	PWSTR psz = 0;

	while (0 < (len = _vsnwprintf(psz, len, format, args)))
	{
		if (psz)
		{
			if (PostMessageW(_M_hwnd, WM_FPRINT, 0, (LPARAM)psz))
			{
				psz = 0;
			}

			break;
		}

		if (!(psz = (PWSTR)LocalAlloc(LMEM_FIXED, ++len * sizeof(WCHAR))))
		{
			break;
		}
	}

	if (psz)
	{
		LocalFree(psz);
	}

	return *this;
}

WNoBufLog& WNoBufLog::operator ()(
					   _In_reads_bytes_(cbBinary) CONST BYTE *pbBinary,
					   _In_ DWORD cbBinary,
					   _In_ DWORD dwFlags)
{
	PWSTR psz = 0;
	ULONG cch = 0;

	while (CryptBinaryToStringW(pbBinary, cbBinary, dwFlags, psz, &cch))
	{
		if (psz)
		{
			if (PostMessageW(_M_hwnd, WM_FPRINT, 0, (LPARAM)psz))
			{
				psz = 0;
			}

			break;
		}

		if (!(psz = (PWSTR)LocalAlloc(LMEM_FIXED, cch * sizeof(WCHAR))))
		{
			break;
		}
	}

	if (psz)
	{
		LocalFree(psz);
	}

	return *this;
}

WNoBufLog& WNoBufLog::operator[](HRESULT dwError)
{
	LPCVOID lpSource = 0;
	ULONG dwFlags = FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_ALLOCATE_BUFFER;

	if ((dwError & FACILITY_NT_BIT) || (0 > dwError && HRESULT_FACILITY(dwError) == FACILITY_NULL))
	{
		dwError &= ~FACILITY_NT_BIT;
__nt:
		dwFlags = FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_ALLOCATE_BUFFER;

		if (!ghnt && !(ghnt = GetModuleHandle(L"ntdll"))) return *this;
		lpSource = ghnt;
	}

	PWSTR psz;
	if (FormatMessageW(dwFlags, lpSource, dwError, 0, (PWSTR)&psz, 0, 0))
	{
		if (!PostMessageW(_M_hwnd, WM_FPRINT, 0, (LPARAM)psz))
		{
			LocalFree(psz);
		}
	}
	else if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
	{
		goto __nt;
	}
	return *this;
}

WNoBufLog& WNoBufLog::operator << (PCWSTR str)
{
	if (SIZE_T len = wcslen(str))
	{
		if (PWSTR psz = (PWSTR)LocalAlloc(LMEM_FIXED, ++len * sizeof(WCHAR)))
		{
			if (!PostMessageW(_M_hwnd, WM_FPRINT, 0, (LPARAM)wcscpy(psz, str)))
			{
				LocalFree(psz);
			}
		}
	}
	return *this;
}

_NT_END