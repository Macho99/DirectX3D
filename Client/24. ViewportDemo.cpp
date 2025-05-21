#include "pch.h"
#include "24. ViewportDemo.h"
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
#include "TextureBuffer.h"

void ViewportDemo::Init()
{
	_shader = make_shared<Shader>(L"19. RenderDemo.fx");

	{
		// Camera
		auto camera = make_shared<GameObject>();
		camera->GetTransform()->SetPosition(Vec3{ 0.f, 0.f, -5.f });
		camera->AddComponent(make_shared<Camera>());
		camera->AddComponent(make_shared<CameraMove>());
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
		// Animation
		shared_ptr<Model> m1 = make_shared<Model>();
		m1->ReadModel(L"Kachujin/Kachujin");
		m1->ReadMaterial(L"Kachujin/Kachujin");
		m1->ReadAnimation(L"Kachujin/Idle");
		m1->ReadAnimation(L"Kachujin/Run");
		m1->ReadAnimation(L"Kachujin/Slash");

		for (int32 i = 0; i < 500; i++)
		{
			auto obj = make_shared<GameObject>();
			obj->GetTransform()->SetPosition(Vec3(rand() % 100, 0, rand() % 100));
			obj->GetTransform()->SetScale(Vec3(0.01f));
			obj->AddComponent(make_shared<ModelAnimator>(_shader));
			{
				obj->GetModelAnimator()->SetModel(m1);
				obj->GetModelAnimator()->SetPass(2);
			}
			CUR_SCENE->Add(obj);
		}
	}

	{
		// Model
		shared_ptr<class Model> m2 = make_shared<Model>();
		m2->ReadModel(L"Tower/Tower");
		m2->ReadMaterial(L"Tower/Tower");

		for (int32 i = 0; i < 100; i++)
		{
			auto obj = make_shared<GameObject>();
			obj->GetTransform()->SetPosition(Vec3(rand() % 100, 0, rand() % 100));
			obj->GetTransform()->SetScale(Vec3(0.01f));

			obj->AddComponent(make_shared<ModelRenderer>(_shader));
			{
				obj->GetModelRenderer()->SetModel(m2);
				obj->GetModelRenderer()->SetPass(1);
			}

			CUR_SCENE->Add(obj);
		}
	}

	{
		// Mesh
		// Material
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(_shader);
			auto texture = RESOURCES->Load<Texture>(L"Veigar", L"..\\Resources\\Textures\\veigar.jpg");
			//auto texture = make_shared<Texture>();
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
			obj->GetTransform()->SetLocalPosition(Vec3(2,0,0));
			obj->AddComponent(make_shared<MeshRenderer>());
			{
				obj->GetMeshRenderer()->SetMaterial(RESOURCES->Get<Material>(L"Veigar"));
			}
			{
				auto mesh = RESOURCES->Get<Mesh>(L"Sphere");
				obj->GetMeshRenderer()->SetMesh(mesh);
				obj->GetMeshRenderer()->SetPass(0);
			}

			CUR_SCENE->Add(obj);
		}
	}

	//RENDER->Init(_shader);
}

void ViewportDemo::Update()
{
	static float width = 800.f;
	static float height = 600.f;
	static float x = 0.f;
	static float y = 0.f;

	ImGui::InputFloat("Width", &width, 10.f);
	ImGui::InputFloat("Height", &height, 10.f);
	ImGui::InputFloat("X", &x, 10.f);
	ImGui::InputFloat("Y", &y, 10.f);

	GRAPHICS->SetViewport(width, height, x, y);

	static Vec3 pos = Vec3(2, 0, 0);
	ImGui::InputFloat3("Pos", (float*)&pos);

	Viewport& vp = GRAPHICS->GetViewport();
	Vec3 pos2D = vp.Project(pos, Matrix::Identity, Camera::S_MatView, Camera::S_MatProjection);

	ImGui::InputFloat3("Pos2D", (float*)&pos2D);
	{
		Vec3 temp = vp.Unproject(pos2D, Matrix::Identity, Camera::S_MatView, Camera::S_MatProjection);
		ImGui::InputFloat3("Recalc", (float*)&temp);
	}
}

void ViewportDemo::Render()
{

}