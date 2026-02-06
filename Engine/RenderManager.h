#pragma once
#include "InstancingBuffer.h"

class GameObject;
enum class RenderTech;

class RenderManager
{
	DECLARE_SINGLE(RenderManager);

public:
	void OnDestroy();

	void Render(vector<GameObject*>& gameObjects, RenderTech renderTech);

private:
	void ClearData();
	void RenderMeshRenderer(vector<GameObject*>& gameObjects);
	void RenderModelRenderer(vector<GameObject*>& gameObjects);
	void RenderAnimRenderer(vector<GameObject*>& gameObjects);

private:
	void AddData(InstanceID instanceId, InstancingData& data);

private:
	map<InstanceID, shared_ptr<InstancingBuffer>> _buffers;
	RenderTech _renderTech;
};

