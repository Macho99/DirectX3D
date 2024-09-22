#include "pch.h"
#include "11. AmbientDemo.h"
#include "GeometryHelper.h"
#include "Camera.h"
#include "CameraMove.h"
#include "GameObject.h"
#include "Transform.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "RenderManager.h"

void AmbientDemo::Init()
{
	RESOURCES->Init();
	_shader = make_shared<Shader>(L"08. Lighting_Ambient.fx");

	// Camera
	_camera = make_shared<GameObject>();
	_camera->GetOrAddTransform()->SetPosition(Vec3::Forward * 10.f);
	_camera->AddComponent(make_shared<Camera>());
	_camera->AddComponent(make_shared<CameraMove>());

	_obj = make_shared<GameObject>();
	_obj->GetOrAddTransform();
	_obj->AddComponent(make_shared<MeshRenderer>());
	{
		_obj->GetMeshRenderer()->SetShader(_shader);
	}
	{
		shared_ptr<Mesh> mesh = RESOURCES->Get<Mesh>(L"Sphere");
		_obj->GetMeshRenderer()->SetMesh(mesh);

		shared_ptr<Texture> texture = RESOURCES->Load<Texture>(L"Grass", L"..\\Resources\\Textures\\Terrain\\grass.jpg");
		_obj->GetMeshRenderer()->SetTexture(texture);
	}
	_obj2 = make_shared<GameObject>();
	_obj2->GetOrAddTransform()->SetPosition(Vec3(0.5f, 0.f, 2.f));
	_obj2->AddComponent(make_shared<MeshRenderer>());
	{
		_obj2->GetMeshRenderer()->SetShader(_shader);
	}
	{
		shared_ptr<Mesh> mesh = RESOURCES->Get<Mesh>(L"Cube");
		_obj2->GetMeshRenderer()->SetMesh(mesh);

		shared_ptr<Texture> texture = RESOURCES->Load<Texture>(L"Veigar", L"..\\Resources\\Textures\\Veigar.jpg");
		_obj2->GetMeshRenderer()->SetTexture(texture);
	}
	RENDER->Init(_shader);
}

void AmbientDemo::Update()
{
	_camera->Update();
	RENDER->Update();

	Vec4 lightAmbient(0.15);
	_shader->GetVector("LightAmbient")->SetFloatVector((float*)&lightAmbient);

	{
		Vec4 matAmbient(1.f);
		_shader->GetVector("MaterialAmbient")->SetFloatVector((float*)&matAmbient);
		_obj->Update();
	}

	{
		Vec4 matAmbient(1.f);
		_shader->GetVector("MaterialAmbient")->SetFloatVector((float*)&matAmbient);
		_obj2->Update();
	}
}

void AmbientDemo::Render()
{

}
