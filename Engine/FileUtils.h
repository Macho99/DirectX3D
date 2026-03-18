#pragma once

#include "fstream"

enum FileMode : uint8
{
	Write,
	Read,
};

class FileUtils
{
public:
	FileUtils();
	~FileUtils();

	void Open(wstring filePath, FileMode mode);

	template<typename T>
	void Write(const T& data)
	{
		DWORD numOfBytes = 0;
		assert(::WriteFile(_handle, &data, sizeof(T), (LPDWORD)&numOfBytes, nullptr));
	}

	template<>
	void Write<string>(const string& data)
	{
		return Write(data);
	}

	void Write(void* data, uint32 dataSize);
	void Write(const string& data);

	template<typename T>
	void Read(OUT T& data)
	{
		DWORD numOfBytes = 0;
		assert(::ReadFile(_handle, &data, sizeof(T), (LPDWORD)&numOfBytes, nullptr));
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
	
	static void SaveTextureToFile(ID3D11Texture2D* texture, const WCHAR* filename);
	static void SaveResourceToJson(const fs::path path, unique_ptr<ResourceBase>& target);
	static unique_ptr<ResourceBase> LoadResourceFromJson(const fs::path path);

	static fs::path SaveFileDialog(
		const wchar_t* title,
		const wchar_t* filter,
		const wchar_t* defaultExt = L"",
		const std::filesystem::path& initialDir = {});
private:
	HANDLE _handle = INVALID_HANDLE_VALUE;
};

