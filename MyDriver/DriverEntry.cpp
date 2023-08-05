#include <ntddk.h>

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(registry_path);
	PAGED_CODE();

	KdPrint(("Enter DriverEntry"));

	KdPrint(("Leave DriverEntry"));

	return STATUS_SUCCESS;
}
