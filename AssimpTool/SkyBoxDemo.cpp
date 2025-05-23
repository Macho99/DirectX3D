#include "pch.h"
#include "SkyBoxDemo.h"
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

void SkyBoxDemo::Init()
{
	RESOURCES->Init();
	_shader = make_shared<Shader>(L"17. SkyBoxDemo.fx");

	// Material
	{
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(_shader);
		auto texture = RESOURCES->Load<Texture>(L"Sky", L"..\\Resources\\Textures\\Sky01.jpg");
		material->SetDiffuseMap(texture);
		MaterialDesc& desc = material->GetMaterialDesc();
		desc.ambient = Vec4(1.f);
		desc.diffuse = Vec4(1.f);
		desc.specular = Vec4(1.f);
		RESOURCES->Add(L"Sky", material);
	}

	{
		// Object
		_obj = make_shared<GameObject>();
		_obj->GetTransform();
		_obj->AddComponent(make_shared<MeshRenderer>());
		{
			auto mesh = RESOURCES->Get<Mesh>(L"Sphere");
			_obj->GetMeshRenderer()->SetMesh(mesh);
		}
		{
			auto material = RESOURCES->Get<Material>(L"Sky");
			_obj->GetMeshRenderer()->SetMaterial(material);
		}
	}

	// Camera
	_camera = make_shared<GameObject>();
	_camera->GetTransform()->SetPosition(Vec3{ 0.f, 0.f, -5.f });
	_camera->AddComponent(make_shared<Camera>());
	_camera->AddComponent(make_shared<CameraMove>());

	RENDER->Init(_shader);
}

void SkyBoxDemo::Update()
{
	_camera->Update();
	RENDER->Update();

	{
		LightDesc lightDesc;
		lightDesc.ambient = Vec4(0.4f);
		lightDesc.diffuse = Vec4(1.f);
		lightDesc.specular = Vec4(0.f);
		lightDesc.direction = Vec3(1.f, 0.f, 1.f);
		RENDER->PushLightData(lightDesc);
	}

	{
		_obj->Update();
	}
}

void SkyBoxDemo::Render()
{

}