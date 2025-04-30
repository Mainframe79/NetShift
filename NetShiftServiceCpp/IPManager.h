#ifndef IP_MANAGER_H
#define IP_MANAGER_H

#include <windows.h>
#include <string>

DWORD SetStaticIP(const std::wstring& presetData);
DWORD ResetToDhcp(const std::wstring& adapterName);
bool IsAdapterActive(const std::wstring& adapterName);
DWORD SetAdapterToStaticIpViaRegistry(const std::wstring& adapterName, const std::wstring& ipAddress, const std::wstring& subnetMask, const std::wstring& gateway, const std::wstring& dns);
DWORD SetAdapterToDhcpViaRegistry(const std::wstring& adapterName);

#endif // IP_MANAGER_H