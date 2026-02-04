#include "pch.h"
#include "29. SceneSerializeDemo.h"
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
#include "cereal/types/string.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/archives/json.hpp"
#include <fstream>

struct InnerData
{
    int id;
    string name;
    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(id),
            CEREAL_NVP(name));
    }
};

struct MyData
{
    int intValue = 42;
    float floatValue = 3.14f;
    string strValue = "Hello, Cereal!";
    InnerData innerData;

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(intValue),
            CEREAL_NVP(floatValue),
            CEREAL_NVP(strValue),
            CEREAL_NVP(innerData));
    }
};

void SceneSerializeDemo::Init()
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

    {
        // Light
        auto light = make_shared<GameObject>(L"Light");
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

    //{
    //    std::ofstream os("data.json");
    //    cereal::JSONOutputArchive archive(os);
    //
    //    MyData m1;
    //    int someInt =0 ;
    //    double d = 313;
    //
    //    archive(CEREAL_NVP(m1), // Names the output the same as the variable name
    //        someInt,        // No NVP - cereal will automatically generate an enumerated name
    //        cereal::make_nvp("this_name_is_way_better", d)); // specify a name of your choosing
    //}

    {
        std::ifstream is("data.json");
        cereal::JSONInputArchive archive(is);

        MyData m1;
        int someInt;
        double d;

        archive(m1, someInt, d); // NVPs not strictly necessary when loading
        // but could be used (even out of order)
    }
}

void SceneSerializeDemo::Update()
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

void SceneSerializeDemo::Render()
{
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}