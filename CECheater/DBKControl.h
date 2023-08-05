#pragma once
#include "common.h"

// 判断文件是否存在
bool FileExists(const std::wstring& filePath);

// 获取驱动地址
PVOID GetDriverAddress(LPCWSTR driverName);

// 加载DBK驱动
BOOL LoadDBKDriver();

// 卸载DBK驱动
BOOL UninitDBKDriver();

// 初始化DBK驱动
BOOL InitDBKDriver();

// 分配/释放 非分页内存
UINT64 DBK_AllocNonPagedMem(ULONG size);
bool DBK_FreeNonPagedMem(UINT64 allocAddress);

// 分配内存
UINT64 DBK_AllocProcessMem(ULONG pid, UINT64 baseAddress, UINT64 size, UINT64 allocationType, UINT64 protect);

// 映射内核态地址到用户态
struct MapMemInfo
{
	UINT64 fromMDL;
	UINT64 address;
};
bool DBK_MdlMapMem(MapMemInfo& mapMemInfo, UINT64 fromPid, UINT64 toPid, UINT64 address, DWORD size);
bool DBK_MdlUnMapMem(MapMemInfo mapMemInfo);

// 读/写虚拟地址
bool DBK_ReadProcessMem(UINT64 pid, UINT64 toAddr, UINT64 fromAddr, DWORD size, bool failToContinue = false);
bool DBK_WriteProcessMem(UINT64 pid, UINT64 targetAddr, UINT64 srcAddr, DWORD size);

UINT64 DBK_GetKernelFuncAddress(const wchar_t* funcName);

bool DBK_ExecuteCode(UINT64 address);
