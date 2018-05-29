#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "wininet")
#pragma comment(lib, "uxtheme")

#include <windows.h>
#include <shlwapi.h>
#include <wininet.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#include "resource.h"

#define TOKEN L"rIiQzotV02ZRFq6CaKOFzaXvRVBwbL4dtpvNb1YKZA5"

TCHAR szClassName[] = TEXT("Window");

int UrlEncode(LPCWSTR lpszSrc, LPWSTR lpszDst)
{
	DWORD iDst = 0;
	const DWORD dwTextLengthA = WideCharToMultiByte(CP_UTF8, 0, lpszSrc, -1, 0, 0, 0, 0);
	LPSTR szUTF8TextA = (LPSTR)GlobalAlloc(GMEM_FIXED, dwTextLengthA); // NULL を含んだ文字列バッファを確保
	if (szUTF8TextA)
	{
		if (WideCharToMultiByte(CP_UTF8, 0, lpszSrc, -1, szUTF8TextA, dwTextLengthA, 0, 0))
		{
			for (DWORD iSrc = 0; iSrc < dwTextLengthA && szUTF8TextA[iSrc] != '\0'; ++iSrc)
			{
				LPCSTR lpszUnreservedCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
				if (StrChrA(lpszUnreservedCharacters, szUTF8TextA[iSrc]))
				{
					if (lpszDst) lpszDst[iDst] = (WCHAR)szUTF8TextA[iSrc];
					++iDst;
				}
				else if (szUTF8TextA[iSrc] == ' ')
				{
					if (lpszDst) lpszDst[iDst] = L'+';
					++iDst;
				}
				else
				{
					if (lpszDst) wsprintfW(&lpszDst[iDst], L"%%%02X", szUTF8TextA[iSrc] & 0xFF);
					iDst += 3;
				}
			}
			if (lpszDst) lpszDst[iDst] = L'\0';
			++iDst;
		}
		GlobalFree(szUTF8TextA);
	}
	return iDst; // NULL 文字を含む
}

BOOL SendNotify(LPCWSTR lpszToken, LPCWSTR lpszMessage)
{
	HINTERNET hInternet = InternetOpenW(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_NO_COOKIES);
	if (hInternet == NULL)
	{
		return FALSE;
	}
	HINTERNET hSession = InternetConnectW(hInternet, L"notify-api.line.me", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (hSession == NULL)
	{
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	HINTERNET hRequest = HttpOpenRequestW(hSession, L"POST", L"/api/notify", NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
	if (hRequest == NULL)
	{
		InternetCloseHandle(hSession);
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	WCHAR szHeader[1024];
	lstrcpyW(szHeader, L"Authorization: Bearer ");
	lstrcatW(szHeader, lpszToken);
	lstrcatW(szHeader, L"\r\nContent-Type: application/x-www-form-urlencoded");
	DWORD dwUrlEncodedTextLength = UrlEncode(lpszMessage, 0);
	LPWSTR lpszUrlEncodedText = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * dwUrlEncodedTextLength);
	UrlEncode(lpszMessage, lpszUrlEncodedText);
	DWORD nLength = WideCharToMultiByte(CP_UTF8, 0, lpszUrlEncodedText, -1, 0, 0, 0, 0);
	LPCSTR lpszMessageAndEqual = "message=";
	LPSTR lpszSendDataA = (LPSTR)GlobalAlloc(0, nLength + lstrlenA(lpszMessageAndEqual));
	lstrcpyA(lpszSendDataA, lpszMessageAndEqual);
	WideCharToMultiByte(CP_UTF8, 0, lpszUrlEncodedText, -1, lpszSendDataA + lstrlenA(lpszSendDataA), nLength, 0, 0);
	GlobalFree(lpszUrlEncodedText);
	if (!HttpSendRequestW(hRequest, szHeader, lstrlenW(szHeader), lpszSendDataA, (DWORD)lstrlenA(lpszSendDataA)))
	{
		GlobalFree(lpszSendDataA);
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hSession);
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	GlobalFree(lpszSendDataA);
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hSession);
	InternetCloseHandle(hInternet);
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hButton;
	static HWND hEdit1;
	static HWND hEdit2;
	static HFONT hFont;
	static DOUBLE dControlHeight = 32.0;
	switch (msg)
	{
	case WM_CREATE:
		{
			HTHEME hTheme = OpenThemeData(hWnd, VSCLASS_AEROWIZARD);
			LOGFONT lf = { 0 };
			GetThemeFont(hTheme, NULL, AW_HEADERAREA, 0, TMT_FONT, &lf);
			hFont = CreateFontIndirectW(&lf);
			dControlHeight = lf.lfHeight * 1.8;
			if (dControlHeight < 0.0)
			{
				dControlHeight = -dControlHeight;
			}
			CloseThemeData(hTheme);
		}
		hButton = CreateWindowW(L"BUTTON", L"送信(F5)", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, 0);
		hEdit1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", TOKEN, WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit1, WM_SETFONT, (WPARAM)hFont, 0);
		hEdit2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", 0, WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit2, WM_SETFONT, (WPARAM)hFont, 0);
		SendMessage(hEdit2, WM_PASTE, 0, 0);
		SendMessage(hEdit2, EM_SETSEL, 0, -1);
		SetFocus(hEdit2);
		break;
	case WM_SIZE:
		MoveWindow(hButton, 10, 10, 256, (int)dControlHeight, TRUE);
		MoveWindow(hEdit1, 10, (int)(dControlHeight + 20), LOWORD(lParam) - 20, (int)dControlHeight, TRUE);
		MoveWindow(hEdit2, 10, (int)(dControlHeight * 2 + 30), LOWORD(lParam) - 20, HIWORD(lParam) - (int)(dControlHeight * 2 + 40), TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			WCHAR szToken[128];
			GetWindowTextW(hEdit1, szToken, _countof(szToken));
			DWORD dwMessageSize = GetWindowTextLengthW(hEdit2);
			LPWSTR lpszMessage = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR)*(dwMessageSize + 1));
			GetWindowTextW(hEdit2, lpszMessage, dwMessageSize + 1);
			BOOL bSucceed =SendNotify(szToken, lpszMessage);
			GlobalFree(lpszMessage);
			if (bSucceed)
			{
				SetWindowText(hEdit2, 0);
				SetFocus(hEdit2);
			}
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefDlgProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	{
		int n = 0;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &n);
		if (n == 2)
		{
			SendNotify(TOKEN, argv[1]);
			LocalFree(argv);
			return 0;
		}
		LocalFree(argv);
	}
	MSG msg;
	WNDCLASSW wndclass = {
		0,
		WndProc,
		0,
		DLGWINDOWEXTRA,
		hInstance,
		LoadIcon(hInstance,(LPCTSTR)IDI_ICON1),
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClassW(&wndclass);
	HWND hWnd = CreateWindowW(
		szClassName,
		L"Line Notify",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	ACCEL Accel[] = { { FVIRTKEY,VK_F5,IDOK } };
	HACCEL hAccel = CreateAcceleratorTable(Accel, 1);
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccel, &msg) && !IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DestroyAcceleratorTable(hAccel);
	return (int)msg.wParam;
}
