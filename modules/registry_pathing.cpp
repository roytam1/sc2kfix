// sc2kfix registry_pathing.cpp: internal registry and pathing handling
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define MISCHOOK_DEBUG_REGISTRY 32768
#define MISCHOOK_DEBUG_PATHING 65536

#define REG_KEY_BASE 0x80000040UL

enum redirected_keys_t {
	enMaxisKey,
	enSC2KKey,
	enPathsKey,
	enWindowsKey,
	enVersionKey,
	enOptionsKey,
	enLocalizeKey,
	enRegistrationKey,
	enSCURKKey,

	enCountKey
};

static int iRegPathHookMode = -1; // -1 - Unknown, 0 - SC2K1996, 1 - SC2K1995, 2 - SCURK1996 (Special Edition)

static BOOL IsRegKey(HKEY hKey, redirected_keys_t rkVal) {
	if (rkVal < enMaxisKey || 
		rkVal >= enCountKey) {
		return FALSE;
	}

	if (hKey == (HKEY)(REG_KEY_BASE + (rkVal))) {
		return TRUE;
	}

	return FALSE;
}

static BOOL IsFakeRegKey(unsigned long ulKey) {
	if ((ulKey) >= (REG_KEY_BASE + enMaxisKey) &&
		(ulKey) < (REG_KEY_BASE + enCountKey)) {
		return TRUE;
	}

	return FALSE;
}

static BOOL RegLookup(const char *lpSubKey, unsigned long *ulKey) {
	BOOL ret;

	ret = FALSE;
	if (strcmp(lpSubKey, "Maxis") == 0) {
		*ulKey = enMaxisKey;
		ret = TRUE;
	}
	else if (strcmp(lpSubKey, "SimCity 2000") == 0) {
		*ulKey = enSC2KKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Paths") == 0) {
		*ulKey = enPathsKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Windows") == 0) {
		*ulKey = enWindowsKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Version") == 0) {
		*ulKey = enVersionKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Options") == 0) {
		*ulKey = enOptionsKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Localize") == 0) {
		*ulKey = enLocalizeKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Registration") == 0) {
		*ulKey = enRegistrationKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "SCURK") == 0) {
		*ulKey = enSCURKKey;
		ret = TRUE;
	}
	return ret;
}

static void GetOutString(const char *sString, LPBYTE lpData, LPDWORD lpcbData) {
	if (lpData == NULL) {
		*lpcbData = strlen(sString) + 1;
	}
	else {
		memcpy(lpData, sString, *lpcbData);
	}
}

static void GetOutDWORD(DWORD dwValue, LPBYTE lpData, LPDWORD lpcbData) {
	if (lpData == NULL) {
		*lpcbData = sizeof(DWORD);
	}
	else {
		memcpy(lpData, &dwValue, sizeof(DWORD));
	}
}

static const char *GetSetMoviesPath() {
	static char szTargetPath[MAX_PATH];

	sprintf_s(szTargetPath, MAX_PATH, "%s\\Movies", szGamePath);
	return szTargetPath;
}

static const char AdjustMoviePathDrive() {
	// Let's get the drive letter from one of two paths:
	// a) Movies path if the setting to use local movies is enabled
	// b) See Below **
	if (bSettingsUseLocalMovies) {
		const char *temp = GetSetMoviesPath();
		if (!temp && !isalpha(temp[0]))
			return 'A';
		return temp[0];
	}

#if STORE_DRIVE_LETTER
	// ** Stored drive letter that'll point towards
	// the drive that contains the 'Goodies' directory
	// which will then contain 'DATA' at its root.
	// The purpose of this is so you can manually
	// target the 'right' drive that would normally
	// contain your SimCity 2000 CD.
	return szSettingsMovieDriveLetter[0];
#else
	return szGamePath[0];
#endif
}

// Reference and inspiration for this comes from the separate
// 'simcity-noinstall' project.
const char *AdjustSource(char *buf, const char *path) {
	static char def_data_path[] = "A:\\DATA\\";

	def_data_path[0] = AdjustMoviePathDrive();

	int plen = strlen(path);
	int flen = strlen(def_data_path);
	if (plen <= flen || _strnicmp(def_data_path, path, flen) != 0) {
		return path;
	}

	char temp[MAX_PATH + 1];
	const char *ptemp = GetSetMoviesPath();
	if (!ptemp) {
		return path;
	}

	memset(temp, 0, sizeof(temp));

	strcpy_s(temp, MAX_PATH, path + (flen - 1));

	strcpy_s(buf, MAX_PATH, ptemp);
	strcat_s(buf, MAX_PATH, temp);

	if (mischook_debug & MISCHOOK_DEBUG_PATHING)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> Source Adjustment - %s -> %s\n", _ReturnAddress(), path, buf);

	return buf;
}

static void GamePathAdjust(const char *szBasePath, const char *target, LPBYTE lpData, LPDWORD lpcbData) {
	char szTarget[MAX_PATH];

	sprintf_s(szTarget, MAX_PATH, "%s\\%s", szBasePath, target);
	ConsoleLog(LOG_DEBUG, "[%s]\n", szTarget);
	GetOutString(szTarget, lpData, lpcbData);
}

static void GetIniOutString(const char *section, const char *key, const char *sValue, LPBYTE lpData, LPDWORD lpcbData) {
	const char *ini_file = GetIniPath();

	char szBuf[64];
	GetPrivateProfileStringA(section, key, sValue, szBuf, sizeof(szBuf) - 1, ini_file);
	GetOutString(szBuf, lpData, lpcbData);
}

static void GetIniOutDWORD(const char *section, const char *key, DWORD dvVal, LPBYTE lpData, LPDWORD lpcbData) {
	const char *ini_file = GetIniPath();

	GetOutDWORD(GetPrivateProfileIntA(section, key, dvVal, ini_file), lpData, lpcbData);
}

extern "C" LSTATUS __stdcall Hook_RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData) {
	if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegSetValueExA(0x%08x, %s, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", _ReturnAddress(), hKey, lpValueName,
			Reserved, dwType, *lpData, cbData);

	return RegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
}

extern "C" LSTATUS __stdcall Hook_RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
	

	if (IsRegKey(hKey, enPathsKey)) {
		char szTargetPath[MAX_PATH];

		strcpy_s(szTargetPath, MAX_PATH, szGamePath);
		if (_stricmp(lpValueName, "Goodies") == 0) {
			if (bSettingsUseLocalMovies) {
				if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
					ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> Query Adjustment - %s -> %s\n", _ReturnAddress(), lpValueName, "MOVIES");
				GamePathAdjust(szTargetPath, "Movies", lpData, lpcbData);
			}
			else {
				szTargetPath[0] = AdjustMoviePathDrive();
				GamePathAdjust(szTargetPath, "Goodies", lpData, lpcbData);
			}
		}
		else if (_stricmp(lpValueName, "Cities") == 0 ||
			_stricmp(lpValueName, "SaveGame") == 0) {
			GamePathAdjust(szTargetPath, "Cities", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Data") == 0) {
			GamePathAdjust(szTargetPath, "Data", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Graphics") == 0) {
			GamePathAdjust(szTargetPath, "Bitmaps", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Home") == 0) {
			ConsoleLog(LOG_DEBUG, "[%s]\n", szTargetPath);
			GetOutString(szTargetPath, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Music") == 0) {
			GamePathAdjust(szTargetPath, "Sounds", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Scenarios") == 0) {
			GamePathAdjust(szTargetPath, "Scenario", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "TileSets") == 0) {
			GamePathAdjust(szTargetPath, "ScurkArt", lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enVersionKey)) {
		if (_stricmp(lpValueName, "SimCity 2000") == 0) {
			GetIniOutDWORD("Version", lpValueName, 256, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "SCURK") == 0) {
			GetIniOutDWORD("Version", lpValueName, 256, lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enWindowsKey)) {
		if (_stricmp(lpValueName, "Display") == 0) {
			GetIniOutString("Windows", lpValueName, "8 1", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Color Check") == 0) {
			GetIniOutDWORD("Windows", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Last Color Depth") == 0) {
			GetIniOutDWORD("Windows", lpValueName, 20, lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enRegistrationKey)) {
		if (_stricmp(lpValueName, "Mayor Name") == 0) {
			GetIniOutString("Registration", lpValueName, szSettingsMayorName, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Company Name") == 0) {
			GetIniOutString("Registration", lpValueName, szSettingsCompanyName, lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enLocalizeKey)) {
		if (_stricmp(lpValueName, "Language") == 0) {
			GetIniOutString("Localize", lpValueName, "USA", lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enSCURKKey)) {
		if (_stricmp(lpValueName, "CycleColors") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 1, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "GridHeight") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 2, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "GridWidth") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 2, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "ShowClipRegion") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "ShowDrawGrid") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "SnapToGrid") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Sound") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 1, lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegQueryValueExA(0x%08x, %s, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", _ReturnAddress(), hKey, lpValueName,
			lpReserved, *lpType, lpData, lpcbData);

	return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

extern "C" LSTATUS __stdcall Hook_RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired,
	const LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition) {

	unsigned long ulKey;

	ulKey = 0;
	if (RegLookup(lpSubKey, &ulKey)) {
		*phkResult = (HKEY)(REG_KEY_BASE + ulKey);
		return ERROR_SUCCESS;
	}

	if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegCreateKeyExA(0x%08x, %s, ...)\n", _ReturnAddress(), hKey, lpSubKey);

	return RegCreateKeyExA(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
}

extern "C" LSTATUS __stdcall Hook_RegCloseKey(HKEY hKey) {

	if (IsFakeRegKey((unsigned long)hKey)) {
		return ERROR_SUCCESS;
	}

	if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegCloseKey(0x%08x)\n", _ReturnAddress(), hKey);

	return RegCloseKey(hKey);
}

extern "C" HANDLE __stdcall Hook_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
	if (mischook_debug & MISCHOOK_DEBUG_PATHING)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> CreateFileA(%s, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", _ReturnAddress(), lpFileName,
			dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	if (bSettingsUseLocalMovies) {
		if (iRegPathHookMode == 0) {
			if ((DWORD)_ReturnAddress() == 0x4A8A90 ||
				(DWORD)_ReturnAddress() == 0x48A810) {
				char buf[MAX_PATH + 1];

				memset(buf, 0, sizeof(buf));

				HANDLE hFileHandle = CreateFileA(AdjustSource(buf, lpFileName), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
				if (mischook_debug & MISCHOOK_DEBUG_PATHING)
					ConsoleLog(LOG_DEBUG, "MISC: (Modification): 0x%08X -> CreateFileA(%s, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X) (0x%08x)\n", _ReturnAddress(), lpFileName,
						dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile, hFileHandle);
				return hFileHandle;
			}
		}
	}
	return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

extern "C" HANDLE __stdcall Hook_FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
	if (mischook_debug & MISCHOOK_DEBUG_PATHING)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> FindFirstFileA(%s, 0x%08X)\n", _ReturnAddress(), lpFileName, lpFindFileData);
	if (bSettingsUseLocalMovies) {
		if (iRegPathHookMode == 0) {
			if ((DWORD)_ReturnAddress() == 0x4A8A90 ||
				(DWORD)_ReturnAddress() == 0x48A810) {
				char buf[MAX_PATH + 1];

				memset(buf, 0, sizeof(buf));

				HANDLE hFileHandle = FindFirstFileA(AdjustSource(buf, lpFileName), lpFindFileData);
				if (mischook_debug & MISCHOOK_DEBUG_PATHING)
					ConsoleLog(LOG_DEBUG, "MISC: (Modification): 0x%08X -> FindFirstFileA(%s, 0x%08X) (0x%08x)\n", _ReturnAddress(), buf, lpFindFileData, hFileHandle);
				return hFileHandle;
			}
		}
	}
	return FindFirstFileA(lpFileName, lpFindFileData);
}

void InstallRegistryPathingHooks_SC2K1996(void) {
	iRegPathHookMode = 0;

	// Install RegSetValueExA hook
	*(DWORD*)(0X4EF7F8) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4EF800) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4EF80C) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4EF810) = (DWORD)Hook_RegCloseKey;

	// Install CreateFileA hook
	*(DWORD*)(0x4EFADC) = (DWORD)Hook_CreateFileA;

	// Install FindFirstFileA hook
	*(DWORD*)(0x4EFB8C) = (DWORD)Hook_FindFirstFileA;
}

void InstallRegistryPathingHooks_SC2K1995(void) {
	iRegPathHookMode = 1;

	// Install RegSetValueExA hook
	*(DWORD*)(0X4EE7A8) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4EE7A4) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4EE7A0) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4EE7AC) = (DWORD)Hook_RegCloseKey;
}

void InstallRegistryPathingHooks_SCURK1996(void) {
	iRegPathHookMode = 2;

	// Install RegSetValueExA hook
	*(DWORD*)(0X4B05F0) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4B05E4) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4B05E8) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4B05EC) = (DWORD)Hook_RegCloseKey;
}
