#pragma once

#include "fstream"

enum NavFileMode : uint8
{
	Write,
	Read,
};

class NavFileUtils
{
public:
	NavFileUtils();
	~NavFileUtils();

	void Open(wstring filePath, NavFileMode mode);

	template<typename T>
	void Write(const T& data)
	{
		DWORD numOfBytes = 0;
		bool result = ::WriteFile(_handle, &data, sizeof(T), (LPDWORD)&numOfBytes, nullptr);
        assert(result);
	}

	template<>
	void Write<string>(const string& data)
	{
		return Write(data);
	}

	void Write(const void* data, uint32 dataSize) const;
	void Write(const string& data);

	template<typename T>
	void Read(OUT T& data)
	{
		DWORD numOfBytes = 0;
		bool result = ::ReadFile(_handle, &data, sizeof(T), (LPDWORD)&numOfBytes, nullptr);
        assert(result);
	}

	template<typename T>
	T Read()
	{
		T data;
		Read(data);
		return data;
	}

	void Read(void** data, uint32 dataSize);
	void Read(OUT string& data);

private:
	HANDLE _handle = INVALID_HANDLE_VALUE;
};

