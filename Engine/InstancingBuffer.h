#pragma once

class VertexBuffer;
struct InstancingData
{
	Matrix world;
};

#define MAX_MESH_INSTANCE 500

class InstancingBuffer
{
public:
	InstancingBuffer();
	~InstancingBuffer();

public:
	void ClearData();
	void AddData(InstancingData& data);
	void PushData();

public:
	uint32 GetCount() { return static_cast<uint32>(_data.size()); }
	shared_ptr<VertexBuffer> GetBuffer() { return _instanceBuffer; }

private:
	void CreateBuffer(uint32 maxCount = MAX_MESH_INSTANCE);

private:
	//uint64 _instanceId = 0;
	uint32 _maxCount = 0;
	shared_ptr<VertexBuffer> _instanceBuffer;
	vector<InstancingData> _data;
};

