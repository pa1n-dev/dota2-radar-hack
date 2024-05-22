#include <ntifs.h>

extern "C"
{
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFuntion);
	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress, PEPROCESS TargetProcess, PVOID TargetAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode, PSIZE_T ReturnSize);
}

void debug_print(PCSTR text)
{
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, text));
}

namespace driver
{
	namespace codes
	{
		//used to setup driver
		constexpr ULONG attach	= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		
		//used to read memory
		constexpr ULONG read	= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		
		//used to write memory
		constexpr ULONG write	= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}

	struct request
	{
		HANDLE process_id;

		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T return_size;
	};

	NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp)
	{
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}

	NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp)
	{
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}

	NTSTATUS memory(PEPROCESS source_process, PVOID source_address, PEPROCESS target_process, PVOID target_address, SIZE_T buffer_size, KPROCESSOR_MODE previous_mode, PSIZE_T return_size)
	{
		if (source_process == nullptr || target_process == nullptr)
		{
			debug_print("[-] process is nullptr\n");
			return STATUS_INVALID_PARAMETER;
		}

		return MmCopyVirtualMemory(source_process, source_address, target_process, target_address, buffer_size, previous_mode, return_size);
	}

	NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp)
	{
		UNREFERENCED_PARAMETER(device_object);

		debug_print("[+] device control called.\n");

		NTSTATUS status = STATUS_UNSUCCESSFUL;

		PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp);

		auto r = reinterpret_cast<request*>(irp->AssociatedIrp.SystemBuffer);

		if (stack_irp == nullptr || r == nullptr)
		{
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}

		static PEPROCESS target_process = nullptr;

		const ULONG control_code = stack_irp->Parameters.DeviceIoControl.IoControlCode;

		switch (control_code)
		{
		case codes::attach:
		{
			status = PsLookupProcessByProcessId(r->process_id, &target_process);
			break;
		}
		case codes::read:
		{
			status = memory(target_process, r->target, PsGetCurrentProcess(), r->buffer, r->size, KernelMode, &r->return_size);
			break;
		}
		case codes::write:
		{
			status = memory(PsGetCurrentProcess(), r->buffer, target_process, r->target, r->size, KernelMode, &r->return_size);
			break;
		}
		default:
		{
			debug_print("[-] unsupported control code\n");
			break;
		}
		}

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = sizeof(request);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}
}

NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	UNREFERENCED_PARAMETER(registry_path);

	UNICODE_STRING device_name = {};
	RtlInitUnicodeString(&device_name, L"\\Device\\TestDriver");

	PDEVICE_OBJECT device_object = nullptr;
	
	NTSTATUS status = IoCreateDevice(driver_object, NULL, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
	if (status != STATUS_SUCCESS)
	{
		debug_print("[-] failed to create driver device.\n");
		return status;
	}

	debug_print("[+] driver device successfully created.\n");

	UNICODE_STRING symbolic_link = {};
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\TestDriver");

	status = IoCreateSymbolicLink(&symbolic_link, &device_name);
	if (status != STATUS_SUCCESS)
	{
		debug_print("[-] failed to create driver symbolic link\n");
		return status;
	}

	debug_print("[+] driver symbolic link successfully created.\n");

	SetFlag(device_object->Flags, DO_BUFFERED_IO);

	driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;

	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);

	debug_print("[+] driver initialized successfully.\n");

	return status;
}

NTSTATUS driver_entry()
{
	debug_print("[+] driver_entry called.\n");

	UNICODE_STRING driver_name = {};
	RtlInitUnicodeString(&driver_name, L"\\Driver\\TestDriver");

	return IoCreateDriver(&driver_name, &driver_main);
}