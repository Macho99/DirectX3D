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
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include "cereal/types/string.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/archives/json.hpp"
#include <cereal/types/polymorphic.hpp>
#include <optional>
#include <fstream>

#define NAMEOF(x) (#x)

struct InnerData
{
public:
    virtual ~InnerData() = default;
    virtual std::string getType() const = 0;

    int id = 999;
    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(id));
    }
};


struct ChildData : public InnerData
{
public:
    int childValue;

    std::string getType() const override
    {
        return NAMEOF(ChildData);
    }
    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(
            cereal::make_nvp("base", cereal::base_class<InnerData>(this)),
            CEREAL_NVP(childValue)
        );
    }
};

struct OtherData : InnerData
{
    float factor = 1.0f;
    std::string name;
    std::optional<int> newValue;

    std::string getType() const override
    {
        return NAMEOF(OtherData);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::make_nvp("base", cereal::base_class<InnerData>(this)),
            CEREAL_NVP(factor),
            CEREAL_NVP(name),
            CEREAL_NVP(newValue)
            );
    }
};

CEREAL_REGISTER_TYPE(ChildData);
CEREAL_REGISTER_TYPE(OtherData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(InnerData, ChildData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(InnerData, OtherData);


struct MyData
{
public:
    int intValue = 42;
    float floatValue = 3.14f;
    string strValue = "Hello, Cereal!";
    vector<unique_ptr<InnerData>> innerData;

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
    //    m1.innerData.push_back(make_unique<ChildData>());
    //    m1.innerData[0]->id = 100;
    //    m1.innerData.push_back(make_unique<OtherData>());
    //    m1.innerData[1]->id = 101;
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