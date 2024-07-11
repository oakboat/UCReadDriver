#pragma once
#include <windows.h>  
#include <iostream>

class driver
{
public:
	~driver()
	{
		if (hDevice != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hDevice);
			hDevice = INVALID_HANDLE_VALUE;
		}
	}
	bool open_device()
	{
		hDevice = CreateFileA("\\\\.\\mstoresvc", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			std::cout << "Failed To Obtain Device Handle!\n";
			return false;
		}
		return true;
	}
	ULONG64 get_base_section_address(ULONG32 pid)
	{
		struct GET_SECTION_BASE_ADDRESS
		{
			ULONG32 pid;
			ULONG64 adderess;
		};

		GET_SECTION_BASE_ADDRESS request{};
		request.pid = pid;
		if (DeviceIoControl(hDevice, 0x1120E040, &request, sizeof(request), &request, sizeof(request), 0, 0))
		{
			return request.adderess;
		}
		return 0;
	}
	bool read_process_memory(ULONG32 pid, ULONG64 target_address, void* buffer, ULONG64 size)
	{
		struct READ_PROCESS_MEMORY
		{
			ULONG32 pid;
			void* buffer;
			ULONG64 target_address;
			ULONG64 size;
		};
		READ_PROCESS_MEMORY request{};
		request.pid = pid;
		request.target_address = target_address;
		request.buffer = buffer;
		request.size = size;
		if (DeviceIoControl(hDevice, 0x1120E040 + 8, &request, sizeof(request), &request, sizeof(request), 0, 0))
		{
			return true;
		}
		return false;
	}

	bool get_dwm_thread_id(ULONG32 pid, void* buffer)
	{
		struct GET_DWM_THREAD_ID
		{
			ULONG32 pid;
			void* buffer;
		};
		GET_DWM_THREAD_ID request{};
		request.pid = pid;
		request.buffer = buffer;
		if (DeviceIoControl(hDevice, 0x1120E040 + 8 + 4, &request, sizeof(request), &request, sizeof(request), 0, 0))
		{
			return true;
		}
		return false;
	}
	
	template<typename T>
	bool read_process_memory(ULONG32 pid, ULONG64 target_address, T* buffer)
	{
		return read_process_memory(pid, target_address, buffer, sizeof(T));
	}

	template<typename T>
	T read_process_memory(ULONG32 pid, ULONG64 target_address)
	{
		T buffer{};
		read_process_memory(pid, target_address, &buffer);
		return buffer;
	}

private:
	HANDLE hDevice = INVALID_HANDLE_VALUE;
};