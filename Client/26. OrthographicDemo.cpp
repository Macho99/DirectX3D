#include "pch.h"
#include "26. OrthographicDemo.h"
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

void OrthographicDemo::Init()
{
	_shader = make_shared<Shader>(L"19. RenderDemo.fx");

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
		// UICamera
		auto camera = make_shared<GameObject>();
		camera->GetOrAddTransform()->SetPosition(Vec3{ 0.f, 0.f, -5.f });
		camera->AddComponent(make_shared<Camera>());
		camera->GetCamera()->SetProjectionType(ProjectionType::Orthographic);
		camera->GetCamera()->SetNear(1.0f);
		camera->GetCamera()->SetFar(100.0f);
		camera->GetCamera()->SetCullingMaskAll();
		camera->GetCamera()->SetCullingMaskLayerOnOff(Layer_UI, false);
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

	{
		// Mesh
		// Material
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(_shader);
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
			obj->GetOrAddTransform()->SetLocalPosition(Vec3(rand() % 2, 0, rand() % 2));
			obj->AddComponent(make_shared<MeshRenderer>());
			{
				obj->GetMeshRenderer()->SetMaterial(RESOURCES->Get<Material>(L"Veigar"));
			}
			{
				auto mesh = RESOURCES->Get<Mesh>(L"Sphere");
				obj->GetMeshRenderer()->SetMesh(mesh);
				obj->GetMeshRenderer()->SetPass(0);
			}
			{
				auto collider = make_shared<SphereCollider>();
				collider->SetRadius(0.5f);
				obj->AddComponent(collider);
			}

			CUR_SCENE->Add(obj);
		}
	}

	// UI
	{
		auto obj = make_shared<GameObject>();
		obj->SetLayerIndex(Layer_UI);
		obj->AddComponent(make_shared<Button>());
		obj->GetButton()->Create(Vec2(100, 100), Vec2(100, 100), RESOURCES->Get<Material>(L"Veigar"));

		obj->GetButton()->AddOnClickedEvent([obj]() { CUR_SCENE->Remove(obj); });

		CUR_SCENE->Add(obj);
	}

	// Terrain
	{
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(_shader);
			auto texture = RESOURCES->Load<Texture>(L"Grass", L"..\\Resources\\Textures\\Terrain\\grass.jpg");
			material->SetDiffuseMap(texture);
			MaterialDesc& desc = material->GetMaterialDesc();
			desc.ambient = Vec4(1.f);
			desc.diffuse = Vec4(1.f);
			desc.specular = Vec4(1.f);
			RESOURCES->Add(L"Grass", material);
		}
		auto obj = make_shared<GameObject>();
		obj->GetOrAddTransform()->SetLocalPosition(Vec3(0.f, -1.f, 0.f));
		obj->AddComponent(make_shared<Terrain>());
		obj->GetTerrain()->Create(10, 10, RESOURCES->Get<Material>(L"Grass"));
		CUR_SCENE->Add(obj);
	}
}

void OrthographicDemo::Update()
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

void OrthographicDemo::Render()
{

}