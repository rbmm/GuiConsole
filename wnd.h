#pragma once

class __declspec(novtable) SimpWndCls 
{
	LONG _M_dwRef = 1;
	LONG _M_dwMessageCount;

	static LRESULT CALLBACK __WindowProc(
		HWND hwnd, 
		UINT uMsg, 
		WPARAM wParam, 
		LPARAM lParam 
		);

	static LRESULT CALLBACK _WindowProc(
		HWND hwnd, 
		UINT uMsg, 
		WPARAM wParam, 
		LPARAM lParam 
		);

	LRESULT wWindowProc(
		HWND hwnd, 
		UINT uMsg, 
		WPARAM wParam, 
		LPARAM lParam 
		);

protected:
	virtual ~SimpWndCls() = default;

	virtual LRESULT WindowProc(
		HWND hwnd, 
		UINT uMsg, 
		WPARAM wParam, 
		LPARAM lParam 
		);

	virtual void AfterLastMessage()
	{
	}

public:
	virtual ULONG STDMETHODCALLTYPE AddRef();

	virtual ULONG STDMETHODCALLTYPE Release();

	HWND Create(
		_In_ DWORD dwExStyle,
		_In_opt_ PCWSTR lpWindowName,
		_In_ DWORD dwStyle,
		_In_ int X,
		_In_ int Y,
		_In_ int nWidth,
		_In_ int nHeight,
		_In_opt_ HWND hWndParent,
		_In_opt_ HMENU hMenu,
		_In_opt_ HINSTANCE hInstance);

	inline static const WCHAR ClassName[] = L"{9F178670-8E1B-4ea1-9606-83DD97FDF359}";

	static ATOM Register();

	static void Unregister();
};