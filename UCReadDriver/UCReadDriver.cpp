#include <iostream>
#include "driver_data.h"
#include "driver_loader.h"
#include "driver.h"
#include <TlHelp32.h>

DWORD GetProcessIDByName(const std::wstring& processName) {
    HANDLE hProcessSnap;
    PROCESSENTRY32W pe32;
    DWORD processID = 0;

    // Take a snapshot of all processes in the system
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::wcerr << L"CreateToolhelp32Snapshot failed!" << std::endl;
        return 0;
    }

    // Fill in the size of the structure before using it
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32FirstW(hProcessSnap, &pe32)) {
        std::wcerr << L"Process32FirstW failed!" << std::endl;
        CloseHandle(hProcessSnap);          // Clean the snapshot object
        return 0;
    }

    // Now walk the snapshot of processes
    do {
        if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
            processID = pe32.th32ProcessID;
            break;
        }
    } while (Process32NextW(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return processID;
}


int main()
{
    driver_loader my_driver_loader;
    driver my_driver;
    std::wstring driver_name = L"UCReadDriver";
    std::wstring system_driver_path = L"C:\\Windows\\System32\\drivers\\" + driver_name + L".sys";
    std::vector<BYTE> driver_data_vector(driver_data, driver_data + sizeof(driver_data));
    DWORD pid = GetCurrentProcessId();
    int test_value = 0x666;

    if (!my_driver_loader.is_running())
    {
        if (!my_driver_loader.write_driver_to_file(system_driver_path, driver_data_vector))
        {
            return 1;
        }
        if (!my_driver_loader.load_driver(system_driver_path, driver_name))
        {
            return 1;
        }
    }
    
    if (!my_driver.open_device())
    {
        return 1;
    }

    std::cout << std::hex;
    std::cout << "Base Address: " << (PVOID)GetModuleHandle(nullptr) << "\n";
    std::cout << "Base Address: " << my_driver.get_base_section_address(pid) << "\n";
    std::cout << "Read value: " << my_driver.read_process_memory<int>(pid, (ULONG64) & test_value) << "\n";

    pid = GetProcessIDByName(L"notepad.exe");
    std::cout << "Base Address: " << my_driver.get_base_section_address(pid) << "\n";
    std::cout << "Read value: " << my_driver.read_process_memory<int>(pid, my_driver.get_base_section_address(pid)) << "\n";

    uintptr_t thread_id = 0;
    my_driver.get_dwm_thread_id(pid, &thread_id);
    while (thread_id == 0 && !(GetAsyncKeyState(VK_F1) & 0x8000))
    {
        Sleep(100);
    }
    std::cout << thread_id << "\n";
    my_driver.get_dwm_thread_id(0, 0);

    my_driver.~driver();
    my_driver_loader.unload_driver(system_driver_path, driver_name);
}
