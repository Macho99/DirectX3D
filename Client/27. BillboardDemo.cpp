#include "pch.h"
#include "27. BillboardDemo.h"
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

void BillboardDemo::Init()
{
	_shader = make_shared<Shader>(L"23. BillboardDemo.fx");

	{
		// Camera
		auto camera = make_shared<GameObject>();
		camera->GetOrAddTransform()->SetPosition(Vec3{ 0.f, 0.f, -5.f });
		camera->AddComponent(make_shared<Camera>());
		camera->AddComponent(make_shared<CameraMove>());
		camera->GetCamera()->SetCullingMaskLayerOnOff(Layer_UI, true);
		CUR_SCENE->Add(camera);
	}

	{
		// Light
		auto light = make_shared<GameObject>();
		light->AddComponent(make_shared<Light>());

		LightDesc lightDesc;
		lightDesc.ambient = Vec4(0.4f);
		lightDesc.diffuse = Vec4(1.f);
		lightDesc.specular = Vec4(0.1f);
		lightDesc.direction = Vec3(1.f, 0.f, 1.f);
		static_pointer_cast<Light>(light->GetFixedComponent(ComponentType::Light))->SetLightDesc(lightDesc);
		CUR_SCENE->Add(light);
	}

	// Billboard
	{
		auto obj = make_shared<GameObject>();
		obj->GetOrAddTransform()->SetLocalPosition(Vec3(0.f));
		obj->AddComponent(make_shared<Billboard>());
		{
			// Material
			{
				shared_ptr<Material> material = make_shared<Material>();
				material->SetShader(_shader);
				auto texture = RESOURCES->Load<Texture>(L"Grass", L"..\\Resources\\Textures\\grass.png");
				//auto texture = RESOURCES->Load<Texture>(L"Veigar", L"..\\Resources\\Textures\\veigar.jpg");
				material->SetDiffuseMap(texture);
				MaterialDesc& desc = material->GetMaterialDesc();
				desc.ambient = Vec4(1.f);
				desc.diffuse = Vec4(1.f);
				desc.specular = Vec4(1.f);
				RESOURCES->Add(L"Veigar", material);

				obj->GetBillboard()->SetMaterial(material);
			}
		}

		for (int32 i = 0; i < 10000; i++)
		{
			Vec2 scale = Vec2(1 + rand() % 3, 1 + rand() % 3);
			Vec2 position = Vec2(-100 + rand() % 200, -100 + rand() % 200);

			obj->GetBillboard()->Add(Vec3(position.x, scale.y * 0.5f, position.y), scale);
		}

		CUR_SCENE->Add(obj);
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
		obj->GetOrAddTransform()->SetLocalPosition(Vec3(-100.f, 0.f, -100.f));
		obj->AddComponent(make_shared<Terrain>());
		obj->GetTerrain()->Create(200, 200, RESOURCES->Get<Material>(L"TerrainGrass"));
		CUR_SCENE->Add(obj);
	}

	// SnowBillboard
	{	// Billboard
		{
			auto snowShader = make_shared<Shader>(L"24. SnowDemo.fx");
			auto obj = make_shared<GameObject>();
			obj->GetOrAddTransform()->SetLocalPosition(Vec3(0.f));
			obj->AddComponent(make_shared<SnowBillboard>(Vec3(100, 100, 100), 10000));
			{
				// Material
				{
					shared_ptr<Material> material = make_shared<Material>();
					material->SetShader(snowShader);
					auto texture = RESOURCES->Load<Texture>(L"Veigar", L"..\\Resources\\Textures\\grass.png");
					//auto texture = RESOURCES->Load<Texture>(L"Veigar", L"..\\Resources\\Textures\\veigar.jpg");
					material->SetDiffuseMap(texture);
					MaterialDesc& desc = material->GetMaterialDesc();
					desc.ambient = Vec4(1.f);
					desc.diffuse = Vec4(1.f);
					desc.specular = Vec4(1.f);
					RESOURCES->Add(L"Veigar", material);

					obj->GetSnowBillboard()->SetMaterial(material);
				}
			}

			CUR_SCENE->Add(obj);
		}
	}
}

void BillboardDemo::Update()
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
}

void BillboardDemo::Render()
{

}