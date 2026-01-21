#include "pch.h"
#include "RenderManager.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "ModelRenderer.h"
#include "ModelAnimator.h"
#include "ParticleSystem.h"
#include "Billboard.h"
#include "SnowBillboard.h"
#include "Material.h"
#include "TessTerrain.h"
#include "GrassRenderer.h"

void RenderManager::OnDestroy()
{
    _buffers.clear();
}

void RenderManager::Render(vector<shared_ptr<GameObject>>& gameObjects, RenderTech renderTech)
{
	ClearData();
	_renderTech = renderTech;
	RenderMeshRenderer(gameObjects);
	RenderModelRenderer(gameObjects);
	RenderAnimRenderer(gameObjects);

	for (shared_ptr<GameObject>& gameObject : gameObjects)
	{
		shared_ptr<ParticleSystem> particle = gameObject->GetFixedComponent<ParticleSystem>(ComponentType::ParticleSystem);
		if (particle != nullptr)
			particle->Render(_renderTech);

		shared_ptr<Billboard> billboard = gameObject->GetFixedComponent<Billboard>(ComponentType::Billboard);
		if (billboard != nullptr)
			billboard->Render(_renderTech);

		shared_ptr<SnowBillboard> snowBillboard = gameObject->GetFixedComponent<SnowBillboard>(ComponentType::SnowBillboard);
		if (snowBillboard != nullptr)
			snowBillboard->Render(_renderTech);

        shared_ptr<TessTerrain> tessTerrain = gameObject->GetFixedComponent<TessTerrain>(ComponentType::TessTerrain);
        if (tessTerrain != nullptr)
            tessTerrain->Render(_renderTech);

        shared_ptr<GrassRenderer> grassRenderer = gameObject->GetFixedComponent<GrassRenderer>(ComponentType::GrassRenderer);
        if (grassRenderer != nullptr)
            grassRenderer->Render(_renderTech);
	}
}

void RenderManager::ClearData()
{
	for (auto& pair : _buffers)
	{
		pair.second->ClearData();
	}
}

void RenderManager::RenderMeshRenderer(vector<shared_ptr<GameObject>>& gameObjects)
{
	map<InstanceID, vector<shared_ptr<GameObject>>> cache;

	// 분류 단계
	for (shared_ptr<GameObject>& gameObject : gameObjects)
	{
		if (gameObject->GetMeshRenderer() == nullptr)
			continue;

		const InstanceID instanceId = gameObject->GetMeshRenderer()->GetInstanceID();
		cache[instanceId].push_back(gameObject);
	}

	for (auto& pair : cache)
	{
		const vector<shared_ptr<GameObject>>& vec = pair.second;
		
		/*if (vec.size() == 1)
		{

		}
		else*/
		{
			const InstanceID instanceId = pair.first;

			for (int32 i = 0; i < vec.size(); i++)
			{
				const shared_ptr<GameObject>& gameObject = vec[i];
				InstancingData data;
				data.world = gameObject->GetTransform()->GetWorldMatrix();

				AddData(instanceId, data);
			}

			shared_ptr<InstancingBuffer>& buffer = _buffers[instanceId];
			vec[0]->GetMeshRenderer()->RenderInstancing(buffer, _renderTech);
		}
	}
}

void RenderManager::RenderModelRenderer(vector<shared_ptr<GameObject>>& gameObjects)
{
	map<InstanceID, vector<shared_ptr<GameObject>>> cache;

	// 분류 단계
	for (shared_ptr<GameObject>& gameObject : gameObjects)
	{
		if (gameObject->GetModelRenderer() == nullptr)
			continue;

		const InstanceID instanceId = gameObject->GetModelRenderer()->GetInstanceID();
		cache[instanceId].push_back(gameObject);
	}

	for (auto& pair : cache)
	{
		const vector<shared_ptr<GameObject>>& vec = pair.second;

		/*if (vec.size() == 1)
		{

		}
		else*/
		{
			const InstanceID instanceId = pair.first;

			for (int32 i = 0; i < vec.size(); i++)
			{
				const shared_ptr<GameObject>& gameObject = vec[i];
				InstancingData data;
				data.world = gameObject->GetTransform()->GetWorldMatrix();

				AddData(instanceId, data);
			}

			shared_ptr<InstancingBuffer>& buffer = _buffers[instanceId];
			vec[0]->GetModelRenderer()->RenderInstancing(buffer, _renderTech);
		}
	}
}

void RenderManager::RenderAnimRenderer(vector<shared_ptr<GameObject>>& gameObjects)
{
	map<InstanceID, vector<shared_ptr<GameObject>>> cache;

	// 분류 단계
	for (shared_ptr<GameObject>& gameObject : gameObjects)
	{
		if (gameObject->GetModelAnimator() == nullptr)
			continue;

		const InstanceID instanceId = gameObject->GetModelAnimator()->GetInstanceID();
		cache[instanceId].push_back(gameObject);
	}

	for (auto& pair : cache)
	{
		shared_ptr<InstancedTweenDesc> tweenDesc = make_shared<InstancedTweenDesc>();
		const vector<shared_ptr<GameObject>>& vec = pair.second;

		/* TODO:
		if (vec.size() == 1)
		{

		}
		else*/
		{
			const InstanceID instanceId = pair.first;

			for (int32 i = 0; i < vec.size(); i++)
			{
				const shared_ptr<GameObject>& gameObject = vec[i];
				InstancingData data;
				data.world = gameObject->GetTransform()->GetWorldMatrix();

				AddData(instanceId, data);

				gameObject->GetModelAnimator()->UpdateTweenData();
				TweenDesc& desc = gameObject->GetModelAnimator()->GetTweenDesc();
				tweenDesc->tweens[i] = desc;
			}

			vec[0]->GetModelAnimator()->GetMaterial()->GetShader()->PushTweenData(*tweenDesc.get());
			shared_ptr<InstancingBuffer>& buffer = _buffers[instanceId];
			vec[0]->GetModelAnimator()->RenderInstancing(buffer, _renderTech);
		}
	}
}

void RenderManager::AddData(InstanceID instanceId, InstancingData& data)
{
	if (_buffers.find(instanceId) == _buffers.end())
	{
		_buffers[instanceId] = make_shared<InstancingBuffer>();
	}

	_buffers[instanceId]->AddData(data);
}
