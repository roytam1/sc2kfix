// sc2kfix utility.cpp: utility functions to save me from reinventing the wheel
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>

#include <sc2kfix.h>
#include "resource.h"

#if defined(_MSC_VER) && _MSC_VER < 1900
#define thread_local
#endif

BOOL WINAPI MyPathRemoveFileSpecA(char* path)
{
	char* filespec = path;
	BOOL modified = FALSE;
	
	if (!path || !*path) { return FALSE; }
	if (*path == '\\') { filespec = ++path; }
	if (*path == '\\') { filespec = ++path; }

	while (*path)
	{
		if (*path == '\\')
		{
			filespec = path;
		}
		else if (*path == ':')
		{
			filespec = ++path;
			if (*path == '\\') { filespec++; }
		}
		path = CharNextA(path);
	}

	if (*filespec)
	{
		*filespec = '\0';
		modified = TRUE;
	}

	return modified;
}

LPSTR WINAPI MyPathFindExtensionA(const char* path)
{
	const char* p = path;
	const char* ext = NULL;
	while (*p)
	{
		if (*p == '.')
		{
			ext = p;
		}
		else if (*p == '\\' || *p == '/')
		{
			ext = NULL;
		}
		p = CharNextA(p);
	}
	if (ext)
	{
		return (LPSTR)ext;
	}
	return (LPSTR)p;
}

char* WINAPI MyPathFindFileNameA(const char* path)
{
	const char* p = path;
	const char* name = NULL;
	while (*p)
	{
		if (*p == '\\' || *p == '/')
		{
			name = p + 1;
		}
		p = CharNextA(p);
	}
	if (name)
	{
		return (char*)name;
	}
	return (char*)path;
}

void WINAPI MyPathRemoveExtensionA(char* path)
{
	char* ext = MyPathFindExtensionA(path);
	if (ext)
	{
		*ext = '\0';
	}
}

void WINAPI MyPathStripPathA(char *path)
{
	if (path)
	{
		char *filename = MyPathFindFileNameA(path);
		if (filename != path) {
			memcpy(path, filename, strlen(filename) + 1);
		}
    }
}

BOOL bFontsInitialized = FALSE;
HFONT hFontMSSansSerifRegular8;
HFONT hFontMSSansSerifBold8;
HFONT hFontMSSansSerifRegular10;
HFONT hFontMSSansSerifBold10;
HFONT hFontArialRegular10;
HFONT hFontArialBold10;
HFONT hSystemRegular12;

void InitializeFonts(void) {
	if (bFontsInitialized)
		return;

	HDC hDC = GetDC(0);
	int iDPI = GetDeviceCaps(hDC, LOGPIXELSY);
	hFontMSSansSerifRegular8 = CreateFont(-MulDiv(8, iDPI, 72), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	hFontMSSansSerifBold8 = CreateFont(-MulDiv(8, iDPI, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	hFontMSSansSerifRegular10 = CreateFont(-MulDiv(10, iDPI, 72), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	hFontMSSansSerifBold10 = CreateFont(-MulDiv(10, iDPI, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	hFontArialRegular10 = CreateFont(-MulDiv(10, iDPI, 72), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
	hFontArialBold10 = CreateFont(-MulDiv(10, iDPI, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
	hSystemRegular12 = CreateFont(-MulDiv(12, iDPI, 72), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "System");
	bFontsInitialized = TRUE;
}

void CenterDialogBox(HWND hwndDlg) {
	HWND hwndDesktop;
	RECT rcTemp, rcDlg, rcDesktop;

	hwndDesktop = GetDesktopWindow();
	GetWindowRect(hwndDesktop, &rcDesktop);
	GetWindowRect(hwndDesktop, &rcTemp);
	GetWindowRect(hwndDlg, &rcDlg);
	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
	OffsetRect(&rcTemp, -rcDesktop.left, -rcDesktop.top);
	OffsetRect(&rcTemp, -rcDlg.right, -rcDlg.bottom);
	SetWindowPos(hwndDlg, HWND_TOP, rcDesktop.left + (rcTemp.right / 2), rcDesktop.top + (rcTemp.bottom / 2), 0, 0, SWP_NOSIZE);
}

HWND CreateTooltip(HWND hDlg, HWND hControl, const char* szText) {
	if (!hDlg || !hControl || !szText)
		return NULL;

	HWND hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, 0, 0, 0, 0, hDlg, NULL, hSC2KFixModule, NULL);
	if (!hTooltip)
		return NULL;

	char* lpszText = _strdup(szText);
	if (!lpszText)
		return NULL;

	SendMessage(hTooltip, TTM_ACTIVATE, TRUE, 0);
	SendMessage(hTooltip, TTM_SETMAXTIPWIDTH, 0, 400);

	TOOLINFO tooltipInfo = { 0 };
	tooltipInfo.cbSize = sizeof(TOOLINFO);
	tooltipInfo.hwnd = hDlg;
	tooltipInfo.uId = (UINT_PTR)hControl;
	tooltipInfo.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	tooltipInfo.lpszText = lpszText;
	SendMessage(hTooltip, TTM_ADDTOOL, NULL, (LPARAM)&tooltipInfo);

	return hTooltip;
}

const char* HexPls(UINT uNumber, int width) {
	thread_local char szRet[16] = { 0 };
	sprintf_s(szRet, 16, "0x%0*X", width, uNumber);
	return szRet;
}

extern FILE* fdLog;

void ConsoleLog(int iLogLevel, const char* fmt, ...) {
	va_list args;
	int len;
	char* buf;
	const char* prefix;

	switch (iLogLevel) {
	case LOG_EMERGENCY:
		prefix = "[EMERG] ";
		break;
	case LOG_ALERT:
		prefix = "[ALERT] ";
		break;
	case LOG_CRITICAL:
		prefix = "[CRIT ] ";
		break;
	case LOG_ERROR:
		prefix = "[ERROR] ";
		break;
	case LOG_WARNING:
		prefix = "[WARN ] ";
		break;
	case LOG_NOTICE:
		prefix = "[NOTE ] ";
		break;
	case LOG_INFO:
		prefix = "[INFO ] ";
		break;
	case LOG_DEBUG:
		prefix = "[DEBUG] ";
		break;
	case LOG_NONE:
	default:
		prefix = "";
		break;
	}

	va_start(args, fmt);
	len = _vscprintf(fmt, args) + 1;
	buf = (char*)malloc(len);
	if (buf) {
		vsprintf_s(buf, len, fmt, args);

		if (fdLog) {
			fprintf(fdLog, "%s%s", prefix, buf);
			fflush(fdLog);
		}

		if (bConsoleEnabled)
			printf("%s%s", prefix, buf);

		free(buf);
	}

	va_end(args);
}

int GetTileID(int iTileX, int iTileY) {
	if (iTileX >= 0 && iTileX < 128 && iTileY >= 0 && iTileY < 128)
		return dwMapXBLD[iTileX]->iTileID[iTileY];
	else
		return -1;
}

const char* GetLowHighScale(BYTE bScale) {
	if (!bScale)
		return "None";
	if (bScale < 60)
		return "Low";
	if (bScale < 120)
		return "Medium";
	if (bScale < 180)
		return "High";
	return "Very High";
}

BOOL FileExists(const char* name) {
	FILE* fdTest;
	if (!fopen_s(&fdTest, name, "r")) {
		fclose(fdTest);
		return TRUE;
	}
	return FALSE;
}

BOOL WritePrivateProfileIntA(const char *section, const char *name, int value, const char *ini_name) {
	char szBuf[128 + 1];

	memset(szBuf, 0, sizeof(szBuf));

	sprintf_s(szBuf, sizeof(szBuf) - 1, "%d", value);
	return WritePrivateProfileStringA(section, name, szBuf, ini_name);
}

#if 1
typedef LSTATUS(WINAPI *MDBX_RegGetValueA)(HKEY hKey, LPCSTR lpSubKey,
                                           LPCSTR lpValue, DWORD dwFlags,
                                           LPDWORD pdwType, PVOID pvData,
                                           LPDWORD pcbData);
static MDBX_RegGetValueA mdbx_RegGetValueA = NULL;
static int mdbx_RegGetValueA_tried = 0;

static LSTATUS _RegGetValue(HKEY hKey, LPCSTR lpSubKey, LPCSTR lpValue, DWORD dwFlags, LPDWORD pdwType,
                                PVOID pvData, LPDWORD pcbData) {
  LSTATUS rc;
  if(!mdbx_RegGetValueA_tried) {
    mdbx_RegGetValueA = (MDBX_RegGetValueA)GetProcAddress(LoadLibraryA("advapi32.dll"), "RegGetValueA");
    mdbx_RegGetValueA_tried = 1;
  }

  if (!mdbx_RegGetValueA) {
    /* an old Windows 2000/XP */
    HKEY hSubKey;
    if (lpSubKey) {
        rc = RegOpenKeyA(hKey, lpSubKey, &hSubKey);
    } else { /* lpSubKey is null, so use hKey directly, don't close it */
        hSubKey = hKey;
        rc = ERROR_SUCCESS;
    }
    if (rc == ERROR_SUCCESS) {
      rc = RegQueryValueExA(hSubKey, lpValue, NULL, pdwType, (LPBYTE)pvData, pcbData);
      if (lpSubKey) RegCloseKey(hSubKey);
    }
    return rc;
  }

  rc = mdbx_RegGetValueA(hKey, lpSubKey, lpValue, dwFlags, pdwType, pvData,
                         pcbData);
  if (rc != ERROR_FILE_NOT_FOUND)
    return rc;

  rc = mdbx_RegGetValueA(hKey, lpSubKey, lpValue,
                         dwFlags | 0x00010000 /* RRF_SUBKEY_WOW6464KEY */,
                         pdwType, pvData, pcbData);
  if (rc != ERROR_FILE_NOT_FOUND)
    return rc;
  return mdbx_RegGetValueA(hKey, lpSubKey, lpValue,
                           dwFlags | 0x00020000 /* RRF_SUBKEY_WOW6432KEY */,
                           pdwType, pvData, pcbData);
}
#else
#define _RegGetValue RegGetValue
#endif

void MigrateRegStringValue(HKEY hKey, const char *lpSubKey, const char *lpValueName, char *szOutBuf, DWORD dwLen) {
	DWORD dwOutBufLen = dwLen;
	_RegGetValue(hKey, lpSubKey, lpValueName, RRF_RT_REG_SZ, NULL, szOutBuf, &dwOutBufLen);
}

void MigrateRegDWORDValue(HKEY hKey, const char *lpSubKey, const char *lpValueName, DWORD *dwOut, DWORD dwSize) {
	DWORD dwOutSize = dwSize;
	_RegGetValue(hKey, lpSubKey, lpValueName, RRF_RT_REG_DWORD, NULL, dwOut, &dwOutSize);
}

void MigrateRegBOOLValue(HKEY hKey, const char *lpSubKey, const char *lpValueName, BOOL *bOut) {
	DWORD dwOutSize = sizeof(BOOL);
	_RegGetValue(hKey, lpSubKey, lpValueName, RRF_RT_REG_DWORD, NULL, bOut, &dwOutSize);
}

// start of base64 code
/*
* Base64 encoding/decoding (RFC1341)
* Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
*
* This software may be distributed under the terms of the BSD license.
*/

static const unsigned char base64_encodetable[65] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char base64_decodetable[256] = {
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	62, 128, 128, 128, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
	61, 128, 128, 128, 0, 128, 128, 128, 0, 1, 2, 3, 4, 5,
	6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 128, 128, 128, 128, 128, 128, 26, 27,
	28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
	42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128
};

std::string Base64Encode(const unsigned char* pSrcData, size_t iSrcCount) {
	unsigned char* out, * pos;
	const unsigned char* end, * in;

	size_t olen;

	olen = 4 * ((iSrcCount + 2) / 3); /* 3-byte blocks to 4-byte */

	if (olen < iSrcCount)
		return std::string(); /* integer overflow */

	std::string outStr;
	outStr.resize(olen);
	out = (unsigned char*)&outStr[0];

	end = pSrcData + iSrcCount;
	in = pSrcData;
	pos = out;
	while (end - in >= 3) {
		*pos++ = base64_encodetable[in[0] >> 2];
		*pos++ = base64_encodetable[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_encodetable[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_encodetable[in[2] & 0x3f];
		in += 3;
	}

	if (end - in) {
		*pos++ = base64_encodetable[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_encodetable[(in[0] & 0x03) << 4];
			*pos++ = '=';
		}
		else {
			*pos++ = base64_encodetable[((in[0] & 0x03) << 4) |
				(in[1] >> 4)];
			*pos++ = base64_encodetable[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}

	return outStr;
}

size_t Base64Decode(BYTE* pBuffer, size_t iBufSize, const unsigned char* pSrcData, size_t iSrcCount) {
	unsigned char* pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	count = 0;
	for (i = 0; i < iSrcCount; i++) {
		if (base64_decodetable[pSrcData[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return 0;

	olen = count / 4 * 3;
	if (olen > iBufSize) {
		return 0;
	}
	pos = pBuffer;
	if (pBuffer == NULL) {
		return 0;
	}

	count = 0;
	for (i = 0; i < iSrcCount; i++) {
		tmp = base64_decodetable[pSrcData[i]];
		if (tmp == 0x80)
			continue;

		if (pSrcData[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					return 0;
				}
				break;
			}
		}
	}

	return pos - pBuffer;
}

// end of base64 code
