#include <Windows.h>
#include <fstream>
#include <iostream>
#include <vector>

class driver_loader
{
public:
    bool is_running()
    {
        HANDLE h_device = CreateFileA("\\\\.\\mstoresvc", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h_device != INVALID_HANDLE_VALUE) {
            std::wcout << L"Driver already loaded." << std::endl;
            CloseHandle(h_device);
            return true;
        }
        return false;
    }

    bool write_driver_to_file(const std::wstring& driver_path, const std::vector<BYTE>& driver_data) {
        std::ofstream file(driver_path, std::ios::binary);
        if (!file.is_open()) {
            std::wcerr << L"Failed to open file: " << driver_path << std::endl;
            return false;
        }
        file.write(reinterpret_cast<const char*>(driver_data.data()), driver_data.size());
        if (!file.good()) {
            std::wcerr << L"Failed to write data to file: " << driver_path << std::endl;
            return false;
        }
        file.close();
        return true;
    }

    bool load_driver(const std::wstring& driver_path, const std::wstring& driver_name) {
        SC_HANDLE sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
        if (!sc_manager) {
            std::wcerr << L"OpenSCManager failed: " << GetLastError() << std::endl;
            return false;
        }

        SC_HANDLE sc_service = CreateService(
            sc_manager,
            driver_name.c_str(),
            driver_name.c_str(),
            SERVICE_START | DELETE | SERVICE_STOP,
            SERVICE_KERNEL_DRIVER,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_IGNORE,
            driver_path.c_str(),
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);

        if (!sc_service) {
            if (GetLastError() == ERROR_SERVICE_EXISTS) {
                sc_service = OpenService(sc_manager, driver_name.c_str(), SERVICE_START | DELETE | SERVICE_STOP);
                if (!sc_service) {
                    std::wcerr << L"OpenService failed: " << GetLastError() << std::endl;
                    CloseServiceHandle(sc_manager);
                    return false;
                }
            }
            else {
                std::wcerr << L"CreateService failed: " << GetLastError() << std::endl;
                CloseServiceHandle(sc_manager);
                return false;
            }
        }

        if (!StartService(sc_service, 0, NULL) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
            std::wcerr << L"StartService failed: " << GetLastError() << std::endl;
            CloseServiceHandle(sc_service);
            CloseServiceHandle(sc_manager);
            return false;
        }

        std::wcout << L"Driver loaded successfully." << std::endl;

        CloseServiceHandle(sc_service);
        CloseServiceHandle(sc_manager);
        return true;
    }

    bool unload_driver(const std::wstring& driver_name, const std::wstring& driver_path) {
        SC_HANDLE sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
        if (!sc_manager) {
            std::wcerr << L"OpenSCManager failed: " << GetLastError() << std::endl;
            return false;
        }

        SC_HANDLE sc_service = OpenService(sc_manager, driver_name.c_str(), SERVICE_STOP | DELETE);
        if (!sc_service) {
            std::wcerr << L"OpenService failed: " << GetLastError() << std::endl;
            CloseServiceHandle(sc_manager);
            return false;
        }

        SERVICE_STATUS service_status;
        if (!ControlService(sc_service, SERVICE_CONTROL_STOP, &service_status) && GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
            std::wcerr << L"ControlService failed: " << GetLastError() << std::endl;
            CloseServiceHandle(sc_service);
            CloseServiceHandle(sc_manager);
            return false;
        }

        if (!DeleteService(sc_service)) {
            std::wcerr << L"DeleteService failed: " << GetLastError() << std::endl;
            CloseServiceHandle(sc_service);
            CloseServiceHandle(sc_manager);
            return false;
        }

        std::wcout << L"Driver unloaded successfully." << std::endl;

        CloseServiceHandle(sc_service);
        CloseServiceHandle(sc_manager);

        if (!DeleteFile(driver_path.c_str())) {
            std::wcerr << L"Failed to delete driver file: " << driver_path << std::endl;
            return false;
        }

        std::wcout << L"Driver file deleted successfully." << std::endl;
        return true;
    }
};