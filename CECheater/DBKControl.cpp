#include "DBKControl.h"
#include "IoCtlCode.h"
#include "MemLoadDriver.h"

#include <tlhelp32.h>
#include <Psapi.h>
#include <fstream>

static HANDLE g_DBKDevice = INVALID_HANDLE_VALUE;

// 判断文件是否存在
bool FileExists(const std::wstring& filePath)
{
	std::ifstream file(filePath);
	return file.good();
}

// 获取驱动地址
PVOID GetDriverAddress(LPCWSTR driverName)
{
	PVOID driverAddrList[1024];
	DWORD cbNeeded = 0;
	if (!EnumDeviceDrivers(driverAddrList, sizeof(driverAddrList), &cbNeeded))
	{
		LOG("EnumDeviceDrivers failed");
		return NULL;
	}

	PVOID pDriverAddr = NULL;
	for (int i = 0; i < (cbNeeded / sizeof(PVOID)); i++)
	{
		if (NULL == driverAddrList[i])
		{
			continue;
		}
		WCHAR curDriverName[MAX_PATH] = { 0 };
		if (GetDeviceDriverBaseNameW(driverAddrList[i], curDriverName, sizeof(curDriverName) / sizeof(WCHAR)))
		{
			if (0 == wcscmp(curDriverName, driverName))
			{
				pDriverAddr = driverAddrList[i];
				break;
			}
		}
	}

	return pDriverAddr;
}

// 加载DBK驱动
BOOL LoadDBKDriver()
{
	// 获取驱动文件全路径
	wchar_t driverFilePath[MAX_PATH];
	GetModuleFileName(NULL, driverFilePath, sizeof(driverFilePath) / sizeof(wchar_t));
	wchar_t* pos = wcsrchr(driverFilePath, L'\\');
	if (nullptr == pos)
	{
		LOG("wcsrchr failed");
		return false;
	}
	*(pos + 1) = L'\0';
	wcscat(driverFilePath, DBK_DRIVER_NAME);

	// 判断驱动文件是否存在
	if (!FileExists(driverFilePath))
	{
		LOG("FileExists failed");
		return false;
	}

	// 打开服务控制管理器
	SC_HANDLE hMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hMgr)
	{
		LOG("OpenSCManager failed");
		return false;
	}

	// 打开NT驱动程序服务
	SC_HANDLE hService = OpenService(hMgr, DBK_SERVICE_NAME, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		// 打不开就创建服务
		hService = CreateService(
			hMgr,
			DBK_SERVICE_NAME,		// 驱动程序的在注册表中的名字  
			DBK_SERVICE_NAME,		// 注册表驱动程序的 DisplayName 值  
			SERVICE_ALL_ACCESS,		// 加载驱动程序的访问权限  
			SERVICE_KERNEL_DRIVER,	// 表示加载的服务是驱动程序  
			SERVICE_DEMAND_START,	// 注册表驱动程序的 Start 值  
			SERVICE_ERROR_NORMAL,	// 注册表驱动程序的 ErrorControl 值  
			driverFilePath,			// 注册表驱动程序的 ImagePath 值  
			NULL,					// GroupOrder HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GroupOrderList
			NULL,
			NULL,
			NULL,
			NULL);
		if (NULL == hService)
		{
			LOG("CreateService failed");
			CloseServiceHandle(hMgr);
			return false;
		}
	}
	else
	{
		bool ret = false;
		ret = ChangeServiceConfig(
			hService,
			SERVICE_KERNEL_DRIVER,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			driverFilePath,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			DBK_SERVICE_NAME);
		if (!ret)
		{
			LOG("ChangeServiceConfig failed");
			CloseServiceHandle(hService);
			CloseServiceHandle(hMgr);
			return false;
		}
	}

	// 写相关注册表
	HKEY hKey;
	std::wstring subKey = Format(L"SYSTEM\\CurrentControlSet\\Services\\%ws", DBK_SERVICE_NAME);
	LSTATUS status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_WRITE, &hKey);
	if (ERROR_SUCCESS != status)
	{
		LOG("RegOpenKeyEx failed");
		CloseServiceHandle(hService);
		CloseServiceHandle(hMgr);
		return false;
	}
	std::wstring AValue = Format(L"\\Device\\%ws", DBK_SERVICE_NAME);
	RegSetValueEx(hKey, L"A", 0, REG_SZ, reinterpret_cast<const BYTE*>(AValue.data()), AValue.size() * sizeof(wchar_t));
	std::wstring BValue = Format(L"\\DosDevices\\%ws", DBK_SERVICE_NAME);
	RegSetValueEx(hKey, L"B", 0, REG_SZ, reinterpret_cast<const BYTE*>(BValue.data()), BValue.size() * sizeof(wchar_t));
	std::wstring CValue = Format(L"\\BaseNamedObjects\\%ws", DBK_PROCESS_EVENT_NAME);
	RegSetValueEx(hKey, L"C", 0, REG_SZ, reinterpret_cast<const BYTE*>(CValue.data()), CValue.size() * sizeof(wchar_t));
	std::wstring DValue = Format(L"\\BaseNamedObjects\\%ws", DBK_THREAD_EVENT_NAME);
	RegSetValueEx(hKey, L"D", 0, REG_SZ, reinterpret_cast<const BYTE*>(DValue.data()), DValue.size() * sizeof(wchar_t));

	// 开启服务
	if (!StartService(hService, NULL, NULL))
	{
		DWORD lastError = GetLastError();
		if (ERROR_SERVICE_ALREADY_RUNNING != lastError)
		{
			LOG("StartService failed, last error: %d", lastError);
			RegCloseKey(hKey);
			CloseServiceHandle(hService);
			CloseServiceHandle(hMgr);
			return false;
		}
	}

	RegCloseKey(hKey);
	CloseServiceHandle(hService);
	CloseServiceHandle(hMgr);
	return true;
}

// 卸载DBK驱动
BOOL UninitDBKDriver()
{
	// 关闭设备对象
	if (INVALID_HANDLE_VALUE != g_DBKDevice)
	{
		CloseHandle(g_DBKDevice);
	}

	// 打开服务控制管理器
	SC_HANDLE hMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hMgr)
	{
		LOG("OpenSCManager failed");
		return false;
	}

	// 打开驱动所对应的服务
	SC_HANDLE hService = OpenService(hMgr, DBK_SERVICE_NAME, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		LOG("OpenService failed");
		CloseServiceHandle(hMgr);
		return false;
	}

	// 停止驱动程序，如果停止失败，只有重新启动才能，再动态加载
	SERVICE_STATUS serviceStatus;
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus))
	{
		LOG("ControlService failed");
		CloseServiceHandle(hService);
		CloseServiceHandle(hMgr);
		return false;
	}

	//动态卸载驱动程序
	if (!DeleteService(hService))
	{
		LOG("DeleteService failed");
		CloseServiceHandle(hService);
		CloseServiceHandle(hMgr);
		return false;
	}

	//离开前关闭打开的句柄
	CloseServiceHandle(hService);
	CloseServiceHandle(hMgr);
	return true;
}

// 初始化DBK驱动
BOOL InitDBKDriver()
{
	// 打开设备对象
	std::wstring deviceName = Format(L"\\\\.\\%ws", DBK_SERVICE_NAME);
	HANDLE hDevice = CreateFile(
		deviceName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (INVALID_HANDLE_VALUE == hDevice)
	{
		LOG("CreateFile failed, last error: %d", GetLastError());
		return false;
	}
	g_DBKDevice = hDevice;

	// 这个步骤或许不做也行
	// InitializeDriver(0, 0);

	// 删除相关注册表
	HKEY hKey;
	std::wstring subKey = Format(L"SYSTEM\\CurrentControlSet\\Services\\%ws", DBK_SERVICE_NAME);
	LSTATUS status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_WRITE, &hKey);
	if (ERROR_SUCCESS != status)
	{
		LOG("RegOpenKeyEx failed");
		return false;
	}
	RegDeleteKey(hKey, L"A");
	RegDeleteKey(hKey, L"B");
	RegDeleteKey(hKey, L"C");
	RegDeleteKey(hKey, L"D");

	RegCloseKey(hKey);
	return true;
}

UINT64 DBK_AllocNonPagedMem(ULONG size)
{
#pragma pack(1)
	struct InputBuffer
	{
		ULONG size;
	};
#pragma pack()

	InputBuffer inputBuffer;
	inputBuffer.size = size;
	UINT64 allocAddress = 0LL;
	DWORD retSize;
	if (!DeviceIoControl(g_DBKDevice, IOCTL_CE_ALLOCATEMEM_NONPAGED, (LPVOID)&inputBuffer, sizeof(inputBuffer), &allocAddress, sizeof(allocAddress), &retSize, NULL))
	{
		LOG("DeviceIoControl IOCTL_CE_ALLOCATEMEM_NONPAGED failed");
		return 0;
	}
	return allocAddress;
}
bool DBK_FreeNonPagedMem(UINT64 allocAddress)
{
#pragma pack(1)
	struct InputBuffer
	{
		UINT64 address;
	};
#pragma pack()

	InputBuffer inputBuffer;
	inputBuffer.address = allocAddress;
	DWORD retSize;
	if (!DeviceIoControl(g_DBKDevice, IOCTL_CE_FREE_NONPAGED, (LPVOID)&inputBuffer, sizeof(inputBuffer), NULL, 0, &retSize, NULL))
	{
		LOG("DeviceIoControl IOCTL_CE_FREE_NONPAGED failed");
		return false;
	}
	return true;
}

UINT64 DBK_AllocProcessMem(ULONG pid, UINT64 baseAddress, UINT64 size, UINT64 allocationType, UINT64 protect)
{
#pragma pack(1)
	struct InputBuffer
	{
		UINT64 pid;
		UINT64 baseAddress;
		UINT64 size;
		UINT64 allocationType;
		UINT64 protect;
	};
#pragma pack()

	InputBuffer inputBuffer;
	inputBuffer.pid = pid;
	inputBuffer.baseAddress = baseAddress;
	inputBuffer.size = size;
	inputBuffer.allocationType = allocationType;
	inputBuffer.protect = protect;
	UINT64 allocAddress = 0LL;
	DWORD retSize;
	if (!DeviceIoControl(g_DBKDevice, IOCTL_CE_ALLOCATEMEM, &inputBuffer, sizeof(inputBuffer), &allocAddress, sizeof(allocAddress), &retSize, NULL))
	{
		LOG("DeviceIoControl IOCTL_CE_ALLOCATEMEM_NONPAGED failed");
		return 0;
	}
	return allocAddress;
}

bool DBK_MdlMapMem(MapMemInfo& mapMemInfo, UINT64 fromPid, UINT64 toPid, UINT64 address, DWORD size)
{
#pragma pack(1)
	struct InputBuffer
	{
		UINT64 fromPid;
		UINT64 toPid;
		UINT64 address;
		DWORD size;
	};
	struct OutputBuffer
	{
		UINT64 fromMDL;
		UINT64 address;
	};
#pragma pack()

	InputBuffer inputBuffer;
	inputBuffer.fromPid = fromPid;
	inputBuffer.toPid = toPid;
	inputBuffer.address = address;
	inputBuffer.size = size;
	OutputBuffer outputBuffer;
	DWORD retSize;
	if (!DeviceIoControl(g_DBKDevice, IOCTL_CE_MAP_MEMORY, &inputBuffer, sizeof(inputBuffer), &outputBuffer, sizeof(outputBuffer), &retSize, NULL))
	{
		LOG("DeviceIoControl IOCTL_CE_MAP_MEMORY failed");
		return false;
	}
	mapMemInfo.fromMDL = outputBuffer.fromMDL;
	mapMemInfo.address = outputBuffer.address;
	return true;
}
bool DBK_MdlUnMapMem(MapMemInfo mapMemInfo)
{
#pragma pack(1)
	struct InputBuffer
	{
		UINT64 fromMDL;
		UINT64 address;
	};
#pragma pack()

	InputBuffer inputBuffer;
	inputBuffer.fromMDL = mapMemInfo.fromMDL;
	inputBuffer.address = mapMemInfo.address;
	DWORD retSize;
	if (!DeviceIoControl(g_DBKDevice, IOCTL_CE_UNMAP_MEMORY, &inputBuffer, sizeof(inputBuffer), NULL, 0, &retSize, NULL))
	{
		LOG("DeviceIoControl IOCTL_CE_UNMAP_MEMORY failed");
		return false;
	}
	return true;
}

bool DBK_ReadProcessMem(UINT64 pid, UINT64 toAddr, UINT64 fromAddr, DWORD size, bool failToContinue)
{
#pragma pack(1)
	struct InputBuffer
	{
		UINT64 processid;
		UINT64 startaddress;
		WORD bytestoread;
	};
#pragma pack()

	UINT64 remaining = size;
	UINT64 offset = 0;
	do
	{
		UINT64 toRead = remaining;
		if (remaining > 4096)
		{
			toRead = 4096;
		}

		InputBuffer inputBuffer;
		inputBuffer.processid = pid;
		inputBuffer.startaddress = fromAddr + offset;
		inputBuffer.bytestoread = toRead;
		DWORD retSize;
		if (!DeviceIoControl(g_DBKDevice, IOCTL_CE_READMEMORY, (LPVOID)&inputBuffer, sizeof(inputBuffer), (LPVOID)(toAddr + offset), toRead, &retSize, NULL))
		{
			if (!failToContinue)
			{
				LOG("DeviceIoControl IOCTL_CE_READMEMORY failed");
				return false;
			}
		}

		remaining -= toRead;
		offset += toRead;
	} while (remaining > 0);

	return true;
}
bool DBK_WriteProcessMem(UINT64 pid, UINT64 targetAddr, UINT64 srcAddr, DWORD size)
{
#pragma pack(1)
	struct InputBuffer
	{
		UINT64 processid;
		UINT64 startaddress;
		WORD bytestowrite;
	};
#pragma pack()

	UINT64 remaining = size;
	UINT64 offset = 0;
	do
	{
		UINT64 toWrite = remaining;
		if (remaining > (512 - sizeof(InputBuffer)))
		{
			toWrite = 512 - sizeof(InputBuffer);
		}

		InputBuffer* pInputBuffer = (InputBuffer*)malloc(toWrite + sizeof(InputBuffer));
		if (NULL == pInputBuffer)
		{
			LOG("malloc failed");
			return false;
		}
		pInputBuffer->processid = pid;
		pInputBuffer->startaddress = targetAddr + offset;
		pInputBuffer->bytestowrite = toWrite;
		memcpy((PCHAR)pInputBuffer + sizeof(InputBuffer), (PCHAR)srcAddr + offset, toWrite);
		DWORD retSize;
		if (!DeviceIoControl(g_DBKDevice, IOCTL_CE_WRITEMEMORY, (LPVOID)pInputBuffer, (sizeof(InputBuffer) + toWrite), NULL, 0, &retSize, NULL))
		{
			LOG("DeviceIoControl IOCTL_CE_WRITEMEMORY failed");
			free(pInputBuffer);
			return false;
		}
		free(pInputBuffer);

		remaining -= toWrite;
		offset += toWrite;
	} while (remaining > 0);

	return true;
}

UINT64 DBK_GetKernelFuncAddress(const wchar_t* funcName)
{
#pragma pack(1)
	struct InputBuffer
	{
		UINT64 s;
	};
#pragma pack()

	InputBuffer inputBuffer;
	inputBuffer.s = (UINT64)funcName;
	UINT64 funcAddress = 0;
	DWORD retSize;
	if (!DeviceIoControl(g_DBKDevice, IOCTL_CE_GETPROCADDRESS, (LPVOID)&inputBuffer, sizeof(inputBuffer), &funcAddress, sizeof(funcAddress), &retSize, NULL))
	{
		LOG("DeviceIoControl IOCTL_CE_GETPROCADDRESS failed");
		return 0;
	}
	return funcAddress;
}

bool DBK_ExecuteCode(UINT64 address)
{
#pragma pack(1)
	struct InputBuffer
	{
		UINT64 address;
		UINT64 parameters;
	};
#pragma pack()

	InputBuffer inputBuffer;
	inputBuffer.address = address;
	inputBuffer.parameters = 0;
	DWORD retSize;
	if (!DeviceIoControl(g_DBKDevice, IOCTL_CE_EXECUTE_CODE, (LPVOID)&inputBuffer, sizeof(inputBuffer), NULL, 0, &retSize, NULL))
	{
		LOG("DeviceIoControl IOCTL_CE_EXECUTE_CODE failed");
		return false;
	}
	return true;
}
