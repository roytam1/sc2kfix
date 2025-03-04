// sc2kfix statsu_dialog.cpp: recreation of the DOS/Mac version status dialog
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <intrin.h>
#include <string>
#include <regex>

#include <sc2kfix.h>
#include "../resource.h"

HWND hStatusDialog = NULL;
HANDLE hWeatherBitmaps[13];
static HCURSOR hDefaultCursor = NULL;
static HWND hwndDesktop;
static RECT rcTemp, rcDlg, rcDesktop;

static COLORREF crToolColor = RGB(0, 0, 0);
static COLORREF crStatusColor = RGB(0, 0, 0);
static HBRUSH hBrushBkg = NULL;

extern "C" int __stdcall Hook_402793(int iStatic, char* szText, int iMaybeAlways1, COLORREF crColor) {
	__asm {
		push ecx
	}
	if (hStatusDialog) {
		if (iStatic == 0) {
			char szCurrentText[200];
			GetDlgItemText(hStatusDialog, IDC_STATIC_SELECTEDTOOL, szCurrentText, 200);
			if (crColor != crToolColor || strcmp(szText, szCurrentText)) {
				SetDlgItemText(hStatusDialog, IDC_STATIC_SELECTEDTOOL, szText);
				crToolColor = crColor;
				InvalidateRect(GetDlgItem(hStatusDialog, IDC_STATIC_SELECTEDTOOL), NULL, TRUE);
			}
		} else if (iStatic == 1) {
			char szCurrentText[200];
			GetDlgItemText(hStatusDialog, IDC_STATIC_STATUSSTRING, szCurrentText, 200);

			// XXX - this is incredibly ugly
			std::string strText = szText;
			strText = std::regex_replace(strText, std::regex("&"), "&&");
			if (crColor != crStatusColor || strcmp(strText.c_str(), szCurrentText)) {
				SetDlgItemText(hStatusDialog, IDC_STATIC_STATUSSTRING, strText.c_str());
				crStatusColor = crColor;
				InvalidateRect(GetDlgItem(hStatusDialog, IDC_STATIC_STATUSSTRING), NULL, TRUE);
			}
		} else if (iStatic == 2) {
			if (crStatusColor == RGB(255, 0, 0))
				SendMessage(GetDlgItem(hStatusDialog, IDC_BUTTON_WEATHERICON), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hWeatherBitmaps[12]);
			else
				SendMessage(GetDlgItem(hStatusDialog, IDC_BUTTON_WEATHERICON), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hWeatherBitmaps[bWeatherTrend]);
			InvalidateRect(GetDlgItem(hStatusDialog, IDC_BUTTON_WEATHERICON), NULL, TRUE);
		} else
			InvalidateRect(hStatusDialog, NULL, FALSE);
	}

	__asm {
		pop ecx
		push crColor
		push iMaybeAlways1
		push szText
		push iStatic
		mov edi, 0x40BD50
		call edi
	}
}

extern "C" int __stdcall Hook_4021A8(int iShow) {
	__asm {
		push ecx
	}

	int iActualShow = iShow;

	if (bSettingsUseStatusDialog)
		iActualShow = 0;

	if (hStatusDialog)
		ShowWindow(hStatusDialog, iShow ? 5 : 0);
	else if (bSettingsUseStatusDialog)
		ShowStatusDialog();

	__asm {
		pop ecx
		push [iActualShow]
		mov edi, 0x40C3E0
		call edi
	}
}

extern "C" int __stdcall Hook_40103C(int iShow) {
	__asm {
		push ecx
	}

	if (hStatusDialog)
		ShowWindow(hStatusDialog, iShow ? 5 : 0);

	__asm {
		pop ecx
		push iShow
		mov edi, 0x40B9E0
		call edi
	}
}

BOOL CALLBACK StatusDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwndDlg, IDC_STATIC_SELECTEDTOOL), WM_SETFONT, (WPARAM)hFontMSSansSerifBold10, TRUE);
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_SELECTEDTOOL))
			SetTextColor((HDC)wParam, crToolColor);
		else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_STATUSSTRING))
			SetTextColor((HDC)wParam, crStatusColor);
		
		SetBkColor((HDC)wParam, RGB(240, 240, 240));
		if (!hBrushBkg)
			hBrushBkg = CreateSolidBrush(RGB(240, 240, 240));
		return (LONG)hBrushBkg;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
			case IDC_BUTTON_WEATHERICON:
				if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED) {
					HWND phWnd = GetParent(hwndDlg);
					if (phWnd) {
						HWND dhWnd = GetDlgItem(phWnd, 111);
						if (dhWnd) {
							SendMessage(GetDlgItem(dhWnd, 120), BM_CLICK, 0, 0);
						}
					}
					return FALSE;
				}
				break;
		}
		break;

	case WM_LBUTTONDOWN:
		// With the following you'll be able to click the client area
		// of the dialogue and drag it around.
		// The only detail is that the pointer reverts to the one set
		// by Windows not the one that's currently active in the game.
		SendMessage(hwndDlg, WM_SYSCOMMAND, (SC_MOVE | HTCAPTION), 0);
		return FALSE;
	}
	return FALSE;
}

HWND ShowStatusDialog(void) {
	DWORD* CWndMainWindow = (DWORD*)*(DWORD*)0x4C702C;	// god this is awful

	if (hStatusDialog)
		return hStatusDialog;

	hStatusDialog = CreateDialogParam(hSC2KFixModule, MAKEINTRESOURCE(IDD_SIMULATIONSTATUS), (HWND)(CWndMainWindow[7]), StatusDialogProc, NULL);
	if (!hStatusDialog) {
		ConsoleLog(LOG_ERROR, "CORE: Couldn't create status dialog: 0x%08X\n", GetLastError());
		return NULL;
	}
	hwndDesktop = GetDesktopWindow();
	GetWindowRect(hwndDesktop, &rcDesktop);
	SetWindowPos(hStatusDialog, HWND_TOP, rcDesktop.left + (rcDesktop.right - rcDesktop.left) / 8 + 128, rcDesktop.top + (rcDesktop.bottom - rcDesktop.top) / 8, 0, 0, SWP_NOSIZE);
	ShowWindow(hStatusDialog, SW_HIDE);
	return hStatusDialog;
}
