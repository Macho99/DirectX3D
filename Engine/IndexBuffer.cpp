#include "pch.h"
#include "IndexBuffer.h"

IndexBuffer::IndexBuffer()
{

}

IndexBuffer::~IndexBuffer()
{

}

void IndexBuffer::Create(const vector<uint32>& indices, int indexCount)
{
	_stride = sizeof(uint32);
	_count = static_cast<uint32>(indices.size());
    if (indexCount > 0)
    {
        _count = std::min(static_cast<uint32>(indexCount), _count);
    }

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.ByteWidth = (uint32)(_stride * _count);

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = indices.data();

	DX_CREATE_BUFFER(&desc, &data, _indexBuffer);
}
