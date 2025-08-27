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
#include "Sky.h"
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

void BillboardDemo::Init()
{
	CUR_SCENE->SetSky(make_shared<Sky>(L"..\\Resources\\Textures\\Sky\\snowcube1024.dds", L"Sky.fx"));
	shared_ptr<Shader> renderShader = make_shared<Shader>(L"19. RenderDemo.fx");
	{
		// Camera
		auto camera = make_shared<GameObject>();
		camera->GetTransform()->SetPosition(Vec3{ 0.f, 2.f, -15.f });
		camera->AddComponent(make_shared<Camera>());
		camera->AddComponent(make_shared<CameraMove>());
		camera->GetCamera()->SetCullingMaskLayerOnOff(Layer_UI, true);
		camera->GetCamera()->SetSsaoSize();
		CUR_SCENE->Add(camera);
	}

	{
		// Mesh
		// Material
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(renderShader);
			auto texture = RESOURCES->Load<Texture>(L"Veigar", L"..\\Resources\\Textures\\veigar.jpg");
			material->SetDiffuseMap(texture);
			MaterialDesc& desc = material->GetMaterialDesc();
			desc.ambient = Vec4(1.f);
			desc.diffuse = Vec4(1.f);
			desc.specular = Vec4(1.f);
			RESOURCES->Add(L"Veigar", material);
		}

		for (int32 i = 0; i < 1; i++)
		{
			auto obj = make_shared<GameObject>();
			obj->GetTransform()->SetLocalPosition(Vec3(0, 1, 0));
			obj->GetTransform()->SetLocalScale(Vec3(1.f));
			obj->AddComponent(make_shared<MeshRenderer>());
			{
				obj->GetMeshRenderer()->SetMaterial(RESOURCES->Get<Material>(L"Veigar"));
			}
			{
				auto mesh = RESOURCES->Get<Mesh>(L"Cube");
				obj->GetMeshRenderer()->SetMesh(mesh);
				obj->GetMeshRenderer()->SetPass(0);
			}
			{
				auto collider = make_shared<OBBBoxCollider>();
				obj->AddComponent(collider);
			}

			CUR_SCENE->Add(obj);
		}
	}

	{
		// Light
		auto light = make_shared<GameObject>();
		light->AddComponent(make_shared<Light>());

		LightDesc lightDesc;
		lightDesc.ambient = Vec4(0.4f);
		lightDesc.diffuse = Vec4(1.f);
		lightDesc.specular = Vec4(0.1f);
		lightDesc.direction = Vec3(1.f, -1.f, 1.f);
		light->GetTransform()->SetRotation(lightDesc.direction);
		static_pointer_cast<Light>(light->GetFixedComponent(ComponentType::Light))->SetLightDesc(lightDesc);
		CUR_SCENE->Add(light);
	}

	// Billboard
	{
		shared_ptr<Shader> shader = make_shared<Shader>(L"23. BillboardDemo.fx");
		auto obj = make_shared<GameObject>();
		obj->GetTransform()->SetLocalPosition(Vec3(0.f));
		obj->AddComponent(make_shared<Billboard>());
		{
			// Material
			{
				shared_ptr<Material> material = make_shared<Material>();
				material->SetShader(shader);
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
		//auto terrainShader = make_shared<Shader>(L"19. RenderDemo.fx");
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(renderShader);
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

	//Particle
	{
		auto particleShader = make_shared<Shader>(L"ParticleSystem.fx");
		auto obj = make_shared<GameObject>();
		obj->GetTransform()->SetLocalPosition(Vec3(0.f, 5.f, 0.f));
		obj->AddComponent(make_shared<ParticleSystem>());
		shared_ptr<ParticleSystem> particleSystem = obj->GetFixedComponent<ParticleSystem>(ComponentType::ParticleSystem);
		particleSystem->SetEmitDirW(Vec3(0.f, 2.f, 0.f));
		shared_ptr<Material> material = make_shared<Material>();
		material->SetRenderQueue(RenderQueue::Transparent);
		material->SetShader(particleShader);
		auto texture = RESOURCES->Load<Texture>(L"Flare", L"..\\Resources\\Textures\\flare0.png");
		material->SetDiffuseMap(texture);
		material->SetRandomTex(RESOURCES->Get<Texture>(L"RandomTex"));
		particleSystem->SetMaterial(material);
		CUR_SCENE->Add(obj);
	}

	// SnowBillboard
	{	// Billboard
		{
			auto snowShader = make_shared<Shader>(L"24. SnowDemo.fx");
			auto obj = make_shared<GameObject>();
			obj->GetTransform()->SetLocalPosition(Vec3(0.f));
			obj->AddComponent(make_shared<SnowBillboard>(Vec3(100, 100, 100), 10000));
			{
				// Material
				{
					shared_ptr<Material> material = make_shared<Material>();
					material->SetShader(snowShader);
					auto texture = RESOURCES->Load<Texture>(L"SnowGrass", L"..\\Resources\\Textures\\grass.png");
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

	{
		shared_ptr<Shader> animShader = make_shared<Shader>(L"19. RenderDemo.fx");
		//animShader->SetTechNum(RenderTech::NormalDepth, -1);
		// Animation
		shared_ptr<Model> m1 = make_shared<Model>();
		m1->ReadModel(L"Kachujin/Kachujin");
		m1->ReadMaterial(L"Kachujin/Kachujin");
		m1->ReadAnimation(L"Kachujin/Idle");
		m1->ReadAnimation(L"Kachujin/Run");
		m1->ReadAnimation(L"Kachujin/Slash");

		for (int32 i = 0; i < 100; i++)
		{
			auto obj = make_shared<GameObject>();
			obj->GetTransform()->SetPosition(Vec3(rand() % 100, 0, rand() % 100));
			obj->GetTransform()->SetScale(Vec3(0.01f));
			obj->AddComponent(make_shared<ModelAnimator>(animShader));
			{
				obj->GetModelAnimator()->SetModel(m1);
				obj->GetModelAnimator()->SetPass(2);
			}
			CUR_SCENE->Add(obj);
		}
	}

	{
		// Model
		shared_ptr<Model> m2 = make_shared<Model>();
		m2->ReadModel(L"Tower/Tower");
		m2->ReadMaterial(L"Tower/Tower");

		for (int32 i = 0; i < 100; i++)
		{
			auto obj = make_shared<GameObject>();
			obj->GetTransform()->SetPosition(Vec3(rand() % 100, -1, rand() % 100));
			obj->GetTransform()->SetScale(Vec3(0.01f));

			obj->AddComponent(make_shared<ModelRenderer>(renderShader));
			{
				obj->GetModelRenderer()->SetModel(m2);
				obj->GetModelRenderer()->SetPass(1);
			}

			CUR_SCENE->Add(obj);
		}
	}

	AddDebugImage(200, 200, GRAPHICS->GetShadowMap(), 1);
	AddDebugImage(200 * 16 / 9, 200, GRAPHICS->GetNormalDepthMap(), 0);
	AddDebugImage(200 * 16 / 9, 200, GRAPHICS->GetSsaoMap(), 1);
	AddDebugImage(200 * 16 / 9, 200, GRAPHICS->GetPostProcessDebugTexture(), 0);

	{
		// UICamera
		auto camera = make_shared<GameObject>();
		camera->GetTransform()->SetPosition(Vec3{ 0.f, 0.f, -5.f });
		camera->AddComponent(make_shared<Camera>());
		camera->GetCamera()->SetProjectionType(ProjectionType::Orthographic);
		camera->GetCamera()->SetNear(1.0f);
		camera->GetCamera()->SetFar(100.0f);
		camera->GetCamera()->SetCullingMaskAll();
		camera->GetCamera()->SetCullingMaskLayerOnOff(Layer_UI, false);
		CUR_SCENE->Add(camera);
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
	//this_thread::sleep_for(chrono::seconds(1));
}

void BillboardDemo::Render()
{
}

void BillboardDemo::AddDebugImage(int32 width, int32 height, shared_ptr<Texture> texture, int techNum)
{
	// UI
	{
		const int debugUISize = 10;
		auto obj = make_shared<GameObject>();
		obj->SetLayerIndex(Layer_UI);
		obj->AddComponent(make_shared<Button>());
		auto material = make_shared<Material>();
		//auto texture = make_shared<Texture>();
		//texture->SetSRV(GRAPHICS->GetShadowMapSRV());
		material->SetDiffuseMap(texture);
		material->SetShader(make_shared<Shader>(L"DebugTexture.fx"));
		material->GetShader()->SetTechNum(RenderTech::Draw, techNum);
		obj->GetButton()->Create(Vec2(width / 2 + _debugImagePosX + 20, height / 2 + 20), Vec2(width, height), material);
		obj->GetButton()->AddOnClickedEvent([obj]() { CUR_SCENE->Remove(obj); });

		CUR_SCENE->Add(obj);
	}

	_debugImagePosX += width + 5;
}
