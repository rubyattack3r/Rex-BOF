#ifndef REX_STANDALONE_IMPORTS_H
#define REX_STANDALONE_IMPORTS_H

#define ADVAPI32$ChangeServiceConfigW ChangeServiceConfigW
#define ADVAPI32$CloseServiceHandle CloseServiceHandle
#define ADVAPI32$CreateServiceW CreateServiceW
#define ADVAPI32$ImpersonateLoggedOnUser ImpersonateLoggedOnUser
#define ADVAPI32$LogonUserW LogonUserW
#define ADVAPI32$OpenSCManagerW OpenSCManagerW
#define ADVAPI32$OpenServiceW OpenServiceW
#define ADVAPI32$QueryServiceConfigW QueryServiceConfigW
#define ADVAPI32$QueryServiceStatusEx QueryServiceStatusEx
#define ADVAPI32$RegCloseKey RegCloseKey
#define ADVAPI32$RegConnectRegistryW RegConnectRegistryW
#define ADVAPI32$RegCreateKeyExW RegCreateKeyExW
#define ADVAPI32$RegOpenKeyExW RegOpenKeyExW
#define ADVAPI32$RegSetValueExW RegSetValueExW
#define ADVAPI32$RevertToSelf RevertToSelf
#define ADVAPI32$StartServiceW StartServiceW

#define KERNEL32$CloseHandle CloseHandle
#define KERNEL32$CreateEventW CreateEventW
#define KERNEL32$CreateThread CreateThread
#define KERNEL32$GetLastError GetLastError
#define KERNEL32$GetProcessHeap GetProcessHeap
#define KERNEL32$HeapAlloc HeapAlloc
#define KERNEL32$HeapFree HeapFree
#define KERNEL32$SetEvent SetEvent
#define KERNEL32$WaitForMultipleObjects WaitForMultipleObjects
#define KERNEL32$WaitForSingleObject WaitForSingleObject
#define KERNEL32$WideCharToMultiByte WideCharToMultiByte

#define OLE32$CoCancelCall CoCancelCall
#define OLE32$CoCreateInstance CoCreateInstance
#define OLE32$CoCreateInstanceEx CoCreateInstanceEx
#define OLE32$CoDisableCallCancellation CoDisableCallCancellation
#define OLE32$CoEnableCallCancellation CoEnableCallCancellation
#define OLE32$CoInitializeEx CoInitializeEx
#define OLE32$CoInitializeSecurity CoInitializeSecurity
#define OLE32$CoSetProxyBlanket CoSetProxyBlanket
#define OLE32$CoUninitialize CoUninitialize

#define OLEAUT32$SysAllocString SysAllocString
#define OLEAUT32$SysFreeString SysFreeString
#define OLEAUT32$VariantClear VariantClear

#define WSMSVC$WSManCloseCommand WSManCloseCommand
#define WSMSVC$WSManCloseOperation WSManCloseOperation
#define WSMSVC$WSManCloseSession WSManCloseSession
#define WSMSVC$WSManCloseShell WSManCloseShell
#define WSMSVC$WSManCreateSession WSManCreateSession
#define WSMSVC$WSManCreateShell WSManCreateShell
#define WSMSVC$WSManDeinitialize WSManDeinitialize
#define WSMSVC$WSManInitialize WSManInitialize
#define WSMSVC$WSManReceiveShellOutput WSManReceiveShellOutput
#define WSMSVC$WSManRunShellCommand WSManRunShellCommand

#endif
