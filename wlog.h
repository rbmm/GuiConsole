#pragma once

enum { WM_FLUSH = WM_USER, WM_KEEP, WM_FPRINT };

void SetEditHandle(HWND hwnd, WPARAM wParam);

class WLog
{
	PVOID _BaseAddress;
	ULONG _RegionSize, _Ptr;

	PWSTR _buf()
	{
		return (PWSTR)((ULONG_PTR)_BaseAddress + _Ptr * sizeof(WCHAR));
	}

	ULONG _cch()
	{
		return _RegionSize / sizeof(WCHAR) - _Ptr;
	}

public:
	void Flush(HWND hwnd);

	void operator >> (HWND hwnd);

	ULONG Init(SIZE_T RegionSize);

	~WLog();

	WLog(WLog&&) = delete;
	WLog(WLog&) = delete;
	WLog(): _BaseAddress(0) { }

	operator PCWSTR()
	{
		return (PCWSTR)_BaseAddress;
	}

	WLog& operator ()(
		_In_reads_bytes_(cbBinary) CONST BYTE *pbBinary,
		_In_ DWORD cbBinary,
		_In_ DWORD dwFlags);

	WLog& operator ()(PCWSTR format, ...);
	WLog& operator << (PCWSTR str);

	WLog& operator[](HRESULT dwError);
};

class WNoBufLog
{
	HWND _M_hwnd;
public:
	WNoBufLog(HWND hwnd) : _M_hwnd(hwnd) {
	}

	WNoBufLog& operator ()(
		_In_reads_bytes_(cbBinary) CONST BYTE *pbBinary,
		_In_ DWORD cbBinary,
		_In_ DWORD dwFlags);

	WNoBufLog& operator ()(PCWSTR format, ...);
	WNoBufLog& operator << (PCWSTR str);

	WNoBufLog& operator[](HRESULT dwError);
};
