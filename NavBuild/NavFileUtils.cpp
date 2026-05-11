#include "pch.h"
#include "NavFileUtils.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <windows.h>
#include <commdlg.h>

using namespace Microsoft::WRL;

NavFileUtils::NavFileUtils()
{

}

NavFileUtils::~NavFileUtils()
{
	if (_handle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(_handle);
		_handle = INVALID_HANDLE_VALUE;
	}
}


void NavFileUtils::Open(wstring filePath, NavFileMode mode)
{
	if (mode == NavFileMode::Write)
	{
		_handle = ::CreateFile(
			filePath.c_str(),
			GENERIC_WRITE,
			0,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
	}
	else
	{
		_handle = ::CreateFile
		(
			filePath.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
	}

	assert(_handle != INVALID_HANDLE_VALUE);
}


void NavFileUtils::Write(const void* data, uint32 dataSize) const
{
    if (dataSize == 0)
        return;

	uint32 numOfBytes = 0;
	bool result = ::WriteFile(_handle, data, dataSize, reinterpret_cast<LPDWORD>(&numOfBytes), nullptr);
    assert(result);
}

void NavFileUtils::Write(const string& data)
{
	uint32 size = (uint32)data.size();
	Write(size);

	if (data.size() == 0)
		return;

	Write((void*)data.data(), size);
}

void NavFileUtils::Read(void** data, uint32 dataSize)
{
    if (dataSize == 0)
        return;

	uint32 numOfBytes = 0;
	bool result = ::ReadFile(_handle, *data, dataSize, reinterpret_cast<LPDWORD>(&numOfBytes), nullptr);
    assert(result);
}

void NavFileUtils::Read(OUT string& data)
{
	uint32 size = Read<uint32>();

	if (size == 0)
		return;

	char* temp = new char[size + 1];
	temp[size] = 0;
	Read((void**)&temp, size);
	data = temp;
	delete[] temp;
}