#include "common.h"
#include "export.h"
#include "DBKControl.h"
#include "MemLoadDriver.h"

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
    if (!DBK_LoadMyDriver(DRIVER_TO_LOAD, DRIVER_NAME))
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
