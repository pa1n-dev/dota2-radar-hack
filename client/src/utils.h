#pragma once

namespace utils
{
	static DWORD get_process_id(const wchar_t* process_name)
	{
		DWORD process_id = 0;

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (snapshot == INVALID_HANDLE_VALUE)
			return process_id;

		PROCESSENTRY32W entry = {};
		entry.dwSize = sizeof(decltype(entry));

		if (Process32FirstW(snapshot, &entry) == TRUE)
		{
			if (_wcsicmp(process_name, entry.szExeFile) == NULL)
				process_id = entry.th32ProcessID;
			else
			{
				while (Process32NextW(snapshot, &entry) == TRUE)
				{
					if (_wcsicmp(process_name, entry.szExeFile) == 0)
					{
						process_id = entry.th32ProcessID;
						break;
					}
				}
			}
		}

		CloseHandle(snapshot);

		return process_id;
	}

	static std::uintptr_t get_module_base(const DWORD pid, const wchar_t* process_name)
	{
		std::uintptr_t module_base = 0;

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
		if (snapshot == INVALID_HANDLE_VALUE)
			return module_base;

		MODULEENTRY32W entry = {};
		entry.dwSize = sizeof(decltype(entry));

		if (Module32FirstW(snapshot, &entry) == TRUE)
		{
			if (wcsstr(process_name, entry.szModule) != nullptr)
				module_base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
			else
			{
				while (Module32NextW(snapshot, &entry) == TRUE)
				{
					if (wcsstr(process_name, entry.szModule) != nullptr)
					{
						module_base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
						break;
					}
				}
			}
		}

		CloseHandle(snapshot);

		return module_base;
	}

}