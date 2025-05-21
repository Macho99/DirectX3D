#include "pch.h"
#include "28. ParticleDemo.h"
#include "GeometryHelper.h"
#include "Camera.h"
#include "GameObject.h"
#include "CameraMove.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Material.h"
#include "Model.h"
#include "ModelRenderer.h"
#include "ModelAnimator.h"
#include "Mesh.h"
#include "Transform.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Light.h"
#include "Scene.h"
#include "SphereCollider.h"
#include "TextureBuffer.h"
#include "Terrain.h"
#include "Camera.h"
#include "Button.h"
#include "MyBillboard.h"
#include "Billboard.h"
#include "SnowBillboard.h"
#include "OBBBoxCollider.h"
#include "SphereCollider.h"
#include "ParticleSystem.h"
#include <thread>

void ParticleDemo::Init()
{
	{
		// Camera
		auto camera = make_shared<GameObject>();
		camera->GetTransform()->SetPosition(Vec3{ 0.f, 0.f, -5.f });
		camera->AddComponent(make_shared<Camera>());
		camera->AddComponent(make_shared<CameraMove>());
		camera->GetCamera()->SetCullingMaskLayerOnOff(Layer_UI, true);
		CUR_SCENE->Add(camera);
	}

	// Terrain
	{
		auto terrainShader = make_shared<Shader>(L"19. RenderDemo.fx");
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(terrainShader);
			auto texture = RESOURCES->Load<Texture>(L"TerrainGrass", L"..\\Resources\\Textures\\Terrain\\grass.jpg");
			material->SetDiffuseMap(texture);
			MaterialDesc& desc = material->GetMaterialDesc();
			desc.ambient = Vec4(1.f);
			desc.diffuse = Vec4(1.f);
			desc.specular = Vec4(1.f);
			RESOURCES->Add(L"TerrainGrass", material);
		}
		auto obj = make_shared<GameObject>();
		obj->GetTransform()->SetLocalPosition(Vec3(-100.f, 0.f, -100.f));
		obj->AddComponent(make_shared<Terrain>());
		obj->GetTerrain()->Create(200, 200, RESOURCES->Get<Material>(L"TerrainGrass"));
		CUR_SCENE->Add(obj);
	}

	{
		auto particleShader = make_shared<Shader>(L"ParticleSystem.fx");
		auto obj = make_shared<GameObject>();
		obj->GetTransform()->SetLocalPosition(Vec3(0.f, 5.f, 0.f));
		obj->AddComponent(make_shared<ParticleSystem>());
		shared_ptr<ParticleSystem> particleSystem = obj->GetFixedComponent<ParticleSystem>(ComponentType::ParticleSystem);
		particleSystem->SetEmitDirW(Vec3(0.f, 4.f, 0.f));
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(particleShader);			
		//auto texture = RESOURCES->Load<Texture>(L"TerrainGrass", L"..\\Resources\\Textures\\Terrain\\grass.jpg");
		material->SetDiffuseMap(RESOURCES->Get<Texture>(L"TerrainGrass"));
		material->SetRandomTex(RESOURCES->Get<Texture>(L"RandomTex"));
		particleSystem->SetMaterial(material);
		CUR_SCENE->Add(obj);
	}
}

void ParticleDemo::Update()
{
	if (INPUT->GetButtonDown(KEY_TYPE::LBUTTON))
	{
		int32 mouseX = INPUT->GetMousePos().x;
		int32 mouseY = INPUT->GetMousePos().y;

		// Picking
		auto pickObj = CUR_SCENE->Pick(mouseX, mouseY);
		if (pickObj)
		{
			CUR_SCENE->Remove(pickObj);
		}
	}
	//this_thread::sleep_for(chrono::seconds(1));
}

void ParticleDemo::Render()
{
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}