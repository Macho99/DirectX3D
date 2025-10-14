#include "pch.h"
#include "FileUtils.h"
#include <d3d11.h>
#include <wrl/client.h>
using namespace Microsoft::WRL;

FileUtils::FileUtils()
{

}

FileUtils::~FileUtils()
{
	if (_handle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(_handle);
		_handle = INVALID_HANDLE_VALUE;
	}
}


void FileUtils::Open(wstring filePath, FileMode mode)
{
	if (mode == FileMode::Write)
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


void FileUtils::Write(void* data, uint32 dataSize)
{
	uint32 numOfBytes = 0;
	assert(::WriteFile(_handle, data, dataSize, reinterpret_cast<LPDWORD>(&numOfBytes), nullptr));
}

void FileUtils::Write(const string& data)
{
	uint32 size = (uint32)data.size();
	Write(size);

	if (data.size() == 0)
		return;

	Write((void*)data.data(), size);
}

void FileUtils::Read(void** data, uint32 dataSize)
{
	uint32 numOfBytes = 0;
	assert(::ReadFile(_handle, *data, dataSize, reinterpret_cast<LPDWORD>(&numOfBytes), nullptr));
}

void FileUtils::Read(OUT string& data)
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

void FileUtils::SaveTextureToFile(ID3D11Texture2D* texture, const WCHAR* filename)
{
	// 텍스처 설명 가져오기
	D3D11_TEXTURE2D_DESC desc;
	texture->GetDesc(&desc);

	// CPU에서 읽을 수 있는 스테이징 텍스처 생성
	D3D11_TEXTURE2D_DESC stagingDesc = desc;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> stagingTexture;
	HR(DEVICE->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture));

	// 원본 텍스처를 스테이징 텍스처로 복사
	DC->CopyResource(stagingTexture.Get(), texture);

	// DirectXTex의 ScratchImage로 캡처
	ScratchImage image;
	HR(CaptureTexture(DEVICE.Get(), DC.Get(), texture, image));

	// PNG 파일로 저장
	HR(DirectX::SaveToWICFile(*image.GetImages(), WIC_FLAGS_NONE,
		GetWICCodec(WIC_CODEC_PNG), filename));
}
