#pragma once
#include "DBKControl.h"

// 加载自己的未签名驱动
bool DBK_LoadMyDriver(LoadType loadType, const wchar_t* driverFilePath, const wchar_t* driverName);
