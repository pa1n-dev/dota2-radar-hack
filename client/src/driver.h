#pragma once

namespace driver
{
	namespace codes
	{
		//used to setup driver
		constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		//used to read memory
		constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		//used to write memory
		constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}

	struct request
	{
		HANDLE process_id;

		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T return_size;
	};

	bool attch_to_process(HANDLE driver_handle, const DWORD pid)
	{
		request r;
		r.process_id = reinterpret_cast<HANDLE>(pid);

		return DeviceIoControl(driver_handle, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
	}

	template <class T>
	T read_memory(HANDLE driver_handle, const std::uintptr_t addr, size_t size = sizeof(T))
	{
		T temp = {};

		request r;
		r.target = reinterpret_cast<HANDLE>(addr);
		r.buffer = &temp;
		r.size = size;

		if (!DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr))
			return 0;

		return temp;
	}

	template <>
	char* read_memory<char*>(HANDLE driver_handle, const std::uintptr_t addr, size_t size)
	{
		char* buffer = new char[size + 1];
		memset(buffer, 0, size + 1);

		request r;
		r.target = reinterpret_cast<HANDLE>(addr);
		r.buffer = buffer;
		r.size = size;

		if (!DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr))
			return 0;

		return buffer;
	}

	/*template <class T>
	void write_memory(HANDLE driver_handle, const std::uintptr_t addr, const T& value)
	{
		request r;
		r.target = reinterpret_cast<PVOID>(addr);
		r.buffer = (PVOID)&value;
		r.size = sizeof(T);

		DeviceIoControl(driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
	}*/
}