#include "common.h"
#include "export.h"
#include "DBKControl.h"
#include "MemLoadDriver.h"

static LoadType g_LoadType;
static WCHAR g_DriverFilePath[MAX_PATH] = { 0 };
static WCHAR g_DriverName[100] = L"\\FileSystem\\";

bool ParseCommandLine()
{
	int nArgs = 0;
	LPWSTR* argList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (nArgs < 3)
	{
		LOG("Number of command args is too few: %d", nArgs);
        return false;
	}

    // 判断驱动文件是否存在
	if (!std::filesystem::exists(argList[2]))
	{
		LOG("Parameter error, path not exist: %ws", argList[2]);
        return false;
	}
    LOG("Find driver file path: %ws", g_DriverFilePath);

    // 获取驱动文件绝对路径
	std::filesystem::path driverFilePath = std::filesystem::absolute(argList[2]);
	wcscpy(g_DriverFilePath, driverFilePath.c_str());

    // 获取驱动文件名
	std::wstring driverName = driverFilePath.stem();
	if (driverName.length() > 90)
	{
		LOG("Parameter error, file name is too long: %ws", driverName.c_str());
		return false;
	}
	wcscat(g_DriverName, driverName.c_str());

    // 获取加载类型
    if (0 == _wcsicmp(argList[1], L"-load_by_shellcode"))
    {
        g_LoadType = LoadByShellcode;
        LOG("Parameter error, load type: load by shellcode");
    }
    else if (0 == _wcsicmp(argList[1], L"-load_by_driver"))
    {
		g_LoadType = LoadByIoCreateDriver;
		LOG("load type: load by driver, driver name: %ws", g_DriverName);
    }
    else
    {
        LOG("Unknown load type: %ws", argList[1]);
        return false;
    }

    return true;
}

void Worker()
{
    // 提权
    if (!AdjustProcessTokenPrivilege())
    {
        LOG("AdjustProcessTokenPrivilege failed");
        return;
    }

    // 加载DBK驱动
    if (NULL == GetDriverAddress(DBK_DRIVER_NAME))
    {
        if (!LoadDBKDriver())
        {
            LOG("load DBKDriver failed");
            return;
        }
        if (NULL == GetDriverAddress(DBK_DRIVER_NAME))
        {
            LOG("GetDriverAddress failed");
            return;
        }
        LOG("load DBKDriver success");
    }
    else
    {
        LOG("DBKDriver Exists");
    }

    // 初始化DBK驱动
    if (!InitDBKDriver())
    {
        LOG("init DBKDriver failed");
        return;
    }
    LOG("init DBKDriver success");

    // 加载自定义驱动
    if (!DBK_LoadMyDriver(g_LoadType, g_DriverFilePath, g_DriverName))
    {
        LOG("load my driver failed");
        return;
    }
    LOG("test DBKDriver");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);

        __try
        {
            if (!ParseCommandLine())
            {
                LOG("ParseCommandLine failed");
                __leave;
            }

            Worker();
        }
        __finally
        {
            // 卸载DBK驱动
            UninitDBKDriver();

            // 直接结束进程
            ExitProcess(0);
        }

        break;
    }
    case DLL_PROCESS_DETACH:
    {
        break;
    }
    }
    return TRUE;
}
