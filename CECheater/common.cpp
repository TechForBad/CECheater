#include "common.h"

#include <tlhelp32.h>
#include <process.h>

static char* str_Format(const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	int count = _vsnprintf(NULL, 0, format, argptr);
	va_end(argptr);

	va_start(argptr, format);
	char* buf = (char*)malloc((count + 1) * sizeof(char));
	if (NULL == buf)
	{
		return NULL;
	}
	memset(buf, 0, (count + 1) * sizeof(char));
	_vsnprintf(buf, count, format, argptr);
	va_end(argptr);

	return buf;
}

static wchar_t* str_Format(const wchar_t* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	int count = _vsnwprintf(NULL, 0, format, argptr);
	va_end(argptr);

	va_start(argptr, format);
	wchar_t* buf = (wchar_t*)malloc((count + 1) * sizeof(wchar_t));
	if (NULL == buf)
	{
		return NULL;
	}
	memset(buf, 0, (count + 1) * sizeof(wchar_t));
	_vsnwprintf(buf, count, format, argptr);
	va_end(argptr);

	return buf;
}

std::string Format(const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	int count = _vsnprintf(NULL, 0, format, argptr);
	va_end(argptr);

	va_start(argptr, format);
	char* buf = (char*)malloc(count * sizeof(char));
	if (NULL == buf)
	{
		return "";
	}
	_vsnprintf(buf, count, format, argptr);
	va_end(argptr);

	std::string str(buf, count);
	free(buf);
	return str;
}

std::wstring Format(const wchar_t* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	int count = _vsnwprintf(NULL, 0, format, argptr);
	va_end(argptr);

	va_start(argptr, format);
	wchar_t* buf = (wchar_t*)malloc(count * sizeof(wchar_t));
	if (NULL == buf)
	{
		return L"";
	}
	_vsnwprintf(buf, count, format, argptr);
	va_end(argptr);

	std::wstring str(buf, count);
	free(buf);
	return str;
}

std::wstring ConvertCharToWString(const char* charStr)
{
	std::wstring wstr;
	int len = strlen(charStr);

	int size = MultiByteToWideChar(CP_UTF8, 0, charStr, len, NULL, 0);
	if (size > 0)
	{
		wstr.resize(size);
		MultiByteToWideChar(CP_UTF8, 0, charStr, len, &wstr[0], size);
	}

	return wstr;
}

BOOL AdjustProcessTokenPrivilege()
{
	LUID luidTmp;
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		LOG("OpenProcessToken failed");
		return FALSE;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidTmp))
	{
		LOG("LookupPrivilegeValue failed");
		CloseHandle(hToken);
		return FALSE;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luidTmp;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
	{
		LOG("AdjustTokenPrivileges failed");
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);
	return TRUE;
}

bool GetCurrentModuleDirPath(WCHAR* dirPath)
{
	HMODULE hModule = NULL;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModuleDirPath, &hModule);
	GetModuleFileName(hModule, dirPath, MAX_PATH);
	wchar_t* pos = wcsrchr(dirPath, L'\\');
	if (nullptr == pos)
	{
		LOG("wcsrchr failed");
		return false;
	}
	*(pos + 1) = L'\0';
	return true;
}
