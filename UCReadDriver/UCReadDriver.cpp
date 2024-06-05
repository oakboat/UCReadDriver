#include <iostream>
#include "driver_data.h"
#include "driver_loader.h"
#include "driver.h"

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

    std::cin.get();
    my_driver.~driver();
    my_driver_loader.unload_driver(system_driver_path, driver_name);
}
