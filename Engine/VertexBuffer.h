#pragma once

class VertexBuffer
{
public:
	VertexBuffer();
	~VertexBuffer();

	ComPtr<ID3D11Buffer> GetComPtr() { return _vertexBuffer; }
	uint32 GetStride() { return _stride; }
	uint32 GetOffset() { return _offset; }
	uint32 GetCount() { return _count; }
	uint32 GetSlot() { return _slot; }

	template<typename T>
	void Create(const vector<T>& vertices, string debugString, uint32 slot = 0, bool cpuWrite = false, bool gpuWrite = false)
	{
		_stride = sizeof(T);
		_count = static_cast<uint32>(vertices.size());

		_slot = slot;
		_cpuWrite = cpuWrite;
		_gpuWrite = gpuWrite;

		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.ByteWidth = (uint32)(_stride * _count);

		if (cpuWrite == false && gpuWrite == false)
		{
			desc.Usage = D3D11_USAGE_IMMUTABLE; // CPU Read, GPU Read
		}
		else if (cpuWrite == true && gpuWrite == false)
		{
			desc.Usage = D3D11_USAGE_DYNAMIC; // CPU Write, GPU Read
			desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
		}
		else if (cpuWrite == false && gpuWrite == true) // CPU Read, GPU Write
		{
			desc.Usage = D3D11_USAGE_DEFAULT;
		}
		else
		{
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		}

		D3D11_SUBRESOURCE_DATA data;
		ZeroMemory(&data, sizeof(data));
		data.pSysMem = vertices.data();

		HRESULT hr = DEVICE->CreateBuffer(&desc, &data, _vertexBuffer.GetAddressOf());
		CHECK(hr);
#ifdef _DEBUG
        const char* debugName = debugString.c_str();
		_vertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, debugString.length(), debugName);
#endif // _DEBUG
	}

	template<typename T>
	void CreateStreamOut(const int maxVertexSize)
	{
		_stride = sizeof(T);

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_DEFAULT;
		//vbd.ByteWidth = sizeof(Vertex::Particle) * 1;
		//vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		//
		// Create the ping-pong buffers for stream-out and drawing.
		//
		vbd.ByteWidth = sizeof(T) * maxVertexSize;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

		HRESULT hr = DEVICE->CreateBuffer(&vbd, 0, _vertexBuffer.GetAddressOf());
		CHECK(hr);
		_vertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("VertexBuffer(Stream)") - 1, "VertexBuffer(Stream)");
	}

	void PushData()
	{
		DC->IASetVertexBuffers(_slot, 1, _vertexBuffer.GetAddressOf(), &_stride, &_offset);
	}

private:
	ComPtr<ID3D11Buffer> _vertexBuffer;

	uint32 _stride = 0;
	uint32 _offset = 0;
	uint32 _count = 0;

	uint32 _slot = 0;
	bool _cpuWrite = false;
	bool _gpuWrite = false;
};
