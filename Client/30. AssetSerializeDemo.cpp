#include "pch.h"
#include "30. AssetSerializeDemo.h"
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
#include <FoliageController.h>
#include "Sky.h"

void AssetSerializeDemo::Init()
{
    CUR_SCENE->SetSky(make_shared<Sky>(L"..\\Resources\\Textures\\Sky\\snowcube1024.dds", L"Shaders\\Sky.fx"));

    {
        GameObjectRef cameraRef = CUR_SCENE->Add("Camera");
        GameObject* camera = cameraRef.Resolve();
        camera->GetTransform()->SetPosition(Vec3{ 0.f, 2.f, -15.f });
        camera->AddComponent(make_unique<Camera>());
        camera->AddComponent(make_unique<CameraMove>());
        camera->GetCamera()->SetCullingMaskLayerOnOff(Layer_UI, true);
        camera->GetCamera()->SetFar(500.f);
    }

    {
        // Light
        GameObjectRef light = CUR_SCENE->Add("Light");
        light.Resolve()->AddComponent(make_unique<Light>());

        LightDesc lightDesc;
        lightDesc.ambient = Vec4(0.4f);
        lightDesc.diffuse = Vec4(1.f);
        lightDesc.specular = Vec4(0.1f);
        lightDesc.direction = Vec3(1.f, -1.f, 1.f);
        light.Resolve()->GetTransform()->SetRotation(lightDesc.direction);
        static_cast<Light*>(light.Resolve()->GetFixedComponent(ComponentType::Light))->SetLightDesc(lightDesc);
    }

    //shared_ptr<Model> m1 = make_shared<Model>();
    //m1->ReadModel(L"Kachujin/Kachujin");
    //m1->ReadMaterial(L"Kachujin/Kachujin");
    //m1->ReadAnimation(L"Kachujin/Idle");
    
    //shared_ptr<Shader> foliageShader = make_shared<Shader>(L"Foliage.fx");
    //ResourceRef<Texture> textureRef = ResourceRef<Texture>::CreateByPath(L"..\\Assets\\Images\\grass.png");
    //Texture* texture = textureRef.Resolve();
    ResourceRef<Shader> foliageShader(RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\Foliage.fx"));
    {
        // Model
        //shared_ptr<Model> m2 = make_shared<Model>();
        //m2->ReadModel(L"Tree1/Tree");
        //m2->ReadMaterial(L"Tree1/Tree");
        ResourceRef<Model> m2(RESOURCES->GetResourceRefByPath<Model>(L"Models\\Tree1\\Tree.fbx"));
        for (int32 i = 0; i < 10; i++)
        {
            auto objRef = CUR_SCENE->Add("Tree" + std::to_string(i));
            GameObject* obj = objRef.Resolve();
            obj->GetTransform()->SetPosition(Vec3(rand() % 100, -1, rand() % 100));
            obj->GetTransform()->SetScale(Vec3(5.f));
    
            obj->AddComponent(make_unique<ModelRenderer>(foliageShader));
            {
                obj->GetModelRenderer()->SetModel(m2);
                obj->GetModelRenderer()->SetPass(0);
            }
    
            auto foliageController = make_unique<FoliageController>();
            foliageController->SetBendFactor(1.f);
            foliageController->SetStiffness(0.8f);
            obj->AddComponent(std::move(foliageController));
        }
    
        Vec3 windDir = Vec3(1.f, 0.f, 1.f);
        windDir.Normalize();
        FoliageController::S_WindDesc.windDirection = windDir;
        FoliageController::S_WindDesc.waveFrequency = 0.1f;
        FoliageController::S_WindDesc.windStrength = 2.f;
    }
}

void AssetSerializeDemo::Update()
{
    if (INPUT->GetButtonDown(KEY_TYPE::LBUTTON))
    {
        int32 mouseX = INPUT->GetMousePos().x;
        int32 mouseY = INPUT->GetMousePos().y;

        // Picking
        auto pickObj = CUR_SCENE->Pick(mouseX, mouseY);
        if (pickObj)
        {
            DBG->Log("Picked!!");
        }
    }
}

void AssetSerializeDemo::Render()
{
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}