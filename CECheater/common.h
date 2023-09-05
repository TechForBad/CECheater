#pragma once

#include <windows.h>
#include <string>
#include <thread>
#include <assert.h>
#include <iostream>
#include <filesystem>

#pragma warning(disable: 4996)

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容

#define DRIVER_TO_LOAD L"MyDriver.sys"
#define DRIVER_NAME L"\\FileSystem\\MyDriver"

#define DBK_SERVICE_NAME L"RichStuff_Service_Name"
#define DBK_PROCESS_EVENT_NAME L"RichStuff_Process_Event_Name"
#define DBK_THREAD_EVENT_NAME L"RichStuff_Thread_Event_Name"
#ifdef _WIN64
#define CHEAT_ENGINE_FILE_NAME L"richstuff-x86_64.exe"
#define DBK_DRIVER_NAME L"richstuffk64.sys"
#else
#define CHEAT_ENGINE_FILE_NAME L"richstuff-i386.exe"
#define DBK_DRIVER_NAME L"richstuffk32.sys"
#endif

enum LoadType
{
	LoadByShellcode,            // 当作shellcode来加载驱动，会由当前进程直接运行驱动的入口点代码
	LoadByIoCreateDriver,       // 调用IoCreateDriver加载驱动，会创建驱动对象，并由系统进程运行驱动的入口点代码
};

std::string Format(const char* format, ...);
std::wstring Format(const wchar_t* format, ...);

#define LOG_TAG "CECheater"

#define __FUNC__ __func__
#define __FILENAME__ \
  (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)

template <typename... Args>
inline void log(
    const char* tag, const char* _file, int _line, const char* _fun,
    const char* fmt, Args... args)
{
    std::string log = Format(fmt, args...);
    std::string allLog = Format("[%s]-%s(%d)::%s %s", tag, _file, _line, _fun, log.c_str());
    OutputDebugStringA(allLog.c_str());
}
#define LOG(...) \
  log(LOG_TAG, __FILENAME__, __LINE__, __FUNC__, __VA_ARGS__)

template <typename... Args>
inline void wlog(
    const char* tag, const char* _file, int _line, const char* _fun,
    const wchar_t* fmt, Args... args)
{
    std::wstring log = Format(fmt, args...);
    std::wstring allLog = Format(L"[%s]-%s(%d)::%s %ws", tag, _file, _line, _fun, log.c_str());
    OutputDebugStringW(allLog.c_str());
}
#define WLOG(...) \
  wlog(LOG_TAG, __FILENAME__, __LINE__, __FUNC__, __VA_ARGS__)

std::wstring ConvertCharToWString(const char* charStr);

// 提权
BOOL AdjustProcessTokenPrivilege();

// 模块获取自身所处文件夹目录
bool GetCurrentModuleDirPath(WCHAR* dirPath);
