#pragma once
#include "InstancingBuffer.h"

class GameObject;
enum class RenderTech;

class RenderManager
{
	DECLARE_SINGLE(RenderManager);

public:
	void Render(vector<shared_ptr<GameObject>>& gameObjects, RenderTech renderTech);

private:
	void ClearData();
	void RenderMeshRenderer(vector<shared_ptr<GameObject>>& gameObjects);
	void RenderModelRenderer(vector<shared_ptr<GameObject>>& gameObjects);
	void RenderAnimRenderer(vector<shared_ptr<GameObject>>& gameObjects);

private:
	void AddData(InstanceID instanceId, InstancingData& data);

private:
	map<InstanceID, shared_ptr<InstancingBuffer>> _buffers;
	RenderTech _renderTech;
};

