// sc2kfix modules/update_notifier.cpp: exactly what it says on the tin
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#include <stdio.h>

#include <sc2kfix.h>
#include "../resource.h"

#define UPDATENOTIFIER_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef UPDATENOTIFIER_DEBUG
#define UPDATENOTIFIER_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT updatenotifier_debug = UPDATENOTIFIER_DEBUG;

typedef HINTERNET (WINAPI *PFN_INTERNETOPENA)(LPCSTR lpszAgent, DWORD dwAccessType, LPCSTR lpszProxy, LPCSTR lpszProxyBypass, DWORD dwFlags);
typedef BOOL (WINAPI *PFN_INTERNETSETOPTIONA)(HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength);
typedef HINTERNET (WINAPI *PFN_INTERNETOPENURLA)(HINTERNET hInternet, LPCSTR lpszUrl, LPCSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext);
typedef BOOL (WINAPI *PFN_INTERNETREADFILE)(HINTERNET hFile, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead);
typedef BOOL (WINAPI *PFN_INTERNETCLOSEHANDLE)(HINTERNET hInternet);

static PFN_INTERNETOPENA pfnInternetOpenA = nullptr;
static PFN_INTERNETSETOPTIONA pfnInternetSetOptionA = nullptr;
static PFN_INTERNETOPENURLA pfnInternetOpenUrlA = nullptr;
static PFN_INTERNETREADFILE pfnInternetReadFile = nullptr;
static PFN_INTERNETCLOSEHANDLE pfnInternetCloseHandle = nullptr;

static HMODULE hWinInetInst = nullptr;

const char* szGitHubRepoReleases = "https://api.github.com/repos/sc2kfix/sc2kfix/releases?per_page=1";
const char* szGitHubAPIType[] = { "Accept: application/vnd.github+json", NULL };
char szLatestRelease[24] = { 0 };
BOOL bUpdateAvailable = FALSE;

DWORD WINAPI UpdaterThread(LPVOID lpParameter) {
	// Give the main thread time to finish DLL init then check for updates and return
	Sleep(200);
	bUpdateAvailable = UpdaterCheckForUpdates();
	return EXIT_SUCCESS;
}

BOOL UpdaterCheckForUpdates(void) {
	DWORD dwContext;

	if(!hWinInetInst)
		hWinInetInst = LoadLibrary("WININET.DLL");

	if(!hWinInetInst) // can't load WININET.DLL, returning FALSE
		return FALSE;

	pfnInternetOpenA = reinterpret_cast<PFN_INTERNETOPENA>(GetProcAddress(hWinInetInst, "InternetOpenA"));
	pfnInternetSetOptionA = reinterpret_cast<PFN_INTERNETSETOPTIONA>(GetProcAddress(hWinInetInst, "InternetSetOptionA"));
	pfnInternetOpenUrlA = reinterpret_cast<PFN_INTERNETOPENURLA>(GetProcAddress(hWinInetInst, "InternetOpenUrlA"));
	pfnInternetReadFile = reinterpret_cast<PFN_INTERNETREADFILE>(GetProcAddress(hWinInetInst, "InternetReadFile"));
	pfnInternetCloseHandle = reinterpret_cast<PFN_INTERNETCLOSEHANDLE>(GetProcAddress(hWinInetInst, "InternetCloseHandle"));

	if(!pfnInternetOpenA || !pfnInternetSetOptionA || !pfnInternetOpenUrlA || !pfnInternetReadFile || !pfnInternetCloseHandle) {// can't get required WININET API, returning FALSE
		return FALSE;
	}

	HINTERNET hInet = pfnInternetOpenA("sc2kfix Update Notifier", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, NULL);
	if (!hInet) {
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  Couldn't open hInet.\n");
		return FALSE;
	}

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetOpen()\n");

	DWORD dwTimeoutMilliseconds = 3000;
	pfnInternetSetOptionA(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeoutMilliseconds, sizeof(DWORD));

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetSetOption()\n");

#pragma warning(push)
#pragma warning(disable:6385)
	HINTERNET hHttpRequest = pfnInternetOpenUrlA(hInet, szGitHubRepoReleases, "X-GitHub-Api-Version: 2022-11-28\r\n", -1L, INTERNET_FLAG_SECURE, (DWORD_PTR)&dwContext);
#pragma warning(pop)
	if (!hHttpRequest) {
		if (GetLastError() == 12002)
			ConsoleLog(LOG_WARNING, "UPD:  Update notifier timed out after 3000 milliseconds.\n");
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  InternetOpenUrl failed, 0x%08X.\n", GetLastError());
		return FALSE;
	}

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetOpenUrl()\n");

	DWORD dwBytesGrabbed = 0;
	char* szBuffer = (char*)malloc(16384);
	if (!szBuffer) {
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  Couldn't allocate szBuffer.\n");
		return FALSE;
	}

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  malloc()\n");

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  Bytes to grab: %u\n", 16384);

	if (!pfnInternetReadFile(hHttpRequest, szBuffer, 16384, &dwBytesGrabbed)) {
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  InternetReadFile failed, 0x%08X.\n", GetLastError());
		return FALSE;
	}

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetReadFile()\n");

	pfnInternetCloseHandle(hHttpRequest);
	pfnInternetCloseHandle(hInet);

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetCloseHandle()\n");

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  Bytes received: %u\n", dwBytesGrabbed);

	szBuffer[16383] = '\0';

	char* szTagString = strstr(szBuffer, "\"tag_name\"");
	if (!szTagString) {
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  No release tag name found (1).\n");
		return FALSE;
	}

	char* szQuoteComma = strstr(szTagString, "\",");
	char szThrowaway[64] = { 0 };
	strncpy_s(szThrowaway, 64, szTagString, szQuoteComma - szTagString);

	int n = sscanf_s(szThrowaway, "\"tag_name\":\"%s\"", szLatestRelease, 23);
	if (!*szLatestRelease) {
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  No release tag name found (2) - scanned %i fields.\n", n);
		return FALSE;
	}

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  Latest release: %s\n", szLatestRelease);

	if (strcmp(szSC2KFixReleaseTag, szLatestRelease)) {
		ConsoleLog(LOG_INFO, "UPD:  New release available: %s (currently running %s)\n", szLatestRelease, szSC2KFixReleaseTag);
		return TRUE;
	}

	return FALSE;
}