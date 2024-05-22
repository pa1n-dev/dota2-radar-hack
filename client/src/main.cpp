#include <thread>
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

#include "dota2.h"
#include "driver.h"
#include "utils.h"

#include "../dependencies/easywsclient/easywsclient.hpp"
#include "../dependencies/nlohmann/json.hpp"

using namespace std;
using namespace nlohmann;

int main()
{
	WSADATA wsa_data = {};
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) 
	{
		cout << "[-] WSAStartup failed" << endl;
		return EXIT_FAILURE;
	}

	cout << "[+] WSA started" << endl;

	auto web_socket = easywsclient::WebSocket::from_url("ws://localhost:22800/");
	if (!web_socket)
	{
		cout << "[-] failed connect to web server" << endl;
		return EXIT_FAILURE;
	}

	cout << "[+] connected to web server" << endl;

	DWORD pid = utils::get_process_id(L"dota2.exe");
	if (!pid)
	{
		cout << "[-] failed to find process." << endl;
		return EXIT_FAILURE;
	}

	cout << "[+] dota2 pid: " << pid << endl;

	HANDLE driver = CreateFile(L"\\\\.\\TestDriver", GENERIC_READ, NULL, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (driver == INVALID_HANDLE_VALUE)
	{
		cout << "[-] failed to find driver." << endl;
		return EXIT_FAILURE;
	}

	cout << "[+] driver handle: " << driver << endl;

	if (!driver::attch_to_process(driver, pid))
	{
		cout << "[-] failed to attach to process." << endl;
		CloseHandle(driver);
		return EXIT_FAILURE;
	}

	cout << "[+] attach successfully." << endl;

	uintptr_t client = utils::get_module_base(pid, L"client.dll");
	if (!client)
	{
		cout << "[-] failed to find client module." << endl;
		CloseHandle(driver);
		return EXIT_FAILURE;
	}

	cout << "[+] base address: " << hex << client << endl;

	while(true)
	{
		uintptr_t entity_list = driver::read_memory<uintptr_t>(driver, client + dota2::offsets::entity_list);
		if (!entity_list)
			continue;

		cout << "[+] entity list address: " << hex << entity_list << endl;

		json data = {};

		for (int i = 0; i < 999; i++)
		{
			uintptr_t entity = driver::read_memory<uintptr_t>(driver, entity_list + (i * 0x8));

			if (!entity) 
				break;

			if ((entity & 0x7) != 0)
				break;

			if ((entity & 0xFF) != 0)
				break;

			uintptr_t networkable = driver::read_memory<uintptr_t>(driver, entity + dota2::offsets::networkable);
			if (!networkable)
				continue;

			uintptr_t client_class = driver::read_memory<uintptr_t>(driver, networkable + dota2::offsets::client_class);
			if (!client_class)
				continue;

			uintptr_t dogshit = driver::read_memory<uintptr_t>(driver, entity + 0x308);
			if (!dogshit)
				continue;

			const char* network_name = driver::read_memory<char*>(driver, client_class, 32);
			if (!network_name)
				continue;

			float x = driver::read_memory<float>(driver, dogshit + 0x10);
			float y = driver::read_memory<float>(driver, dogshit + 0x14);

			data["data"][i] = { {"network_name", network_name}, {"x", x}, {"y", y} };
		}

		web_socket->send(data.dump());
		web_socket->poll();

		this_thread::sleep_for(chrono::milliseconds(100));
	}

	CloseHandle(driver);

	return EXIT_SUCCESS;
}