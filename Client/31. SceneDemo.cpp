#include "pch.h"
#include "31. SceneDemo.h"
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
#include "TessTerrain.h"
#include "GrassRenderer.h"

void SceneDemo::Init()
{
    auto sky = make_shared<Sky>();
    sky->SetMaterial(RESOURCES->GetResourceRefByPath<Material>("Materials\\SnowSkyMat.mat"));
    CUR_SCENE->SetSky(sky);

    ResourceRef<Shader> renderShader = RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\19. RenderDemo.fx");
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
        light.Resolve()->GetTransform()->SetRotation(Vec3(1.f, -1.f, 1.f));
        static_cast<Light*>(light.Resolve()->GetFixedComponent(ComponentType::Light))->SetLightDesc(lightDesc);
    }
    {
        ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\VeigarMaterial.mat");
        for (int32 i = 0; i < 1; i++)
        {
            auto objRef = CUR_SCENE->Add("Veigar");
            GameObject* obj = objRef.Resolve();
            obj->GetTransform()->SetLocalPosition(Vec3(0, 1, 0));
            obj->GetTransform()->SetLocalScale(Vec3(1.f));
            obj->AddComponent(make_unique<MeshRenderer>());
            {
                obj->GetMeshRenderer()->SetMaterial(materialRef);
            }
            {
                auto mesh = RESOURCES->GetCubeMesh();
                obj->GetMeshRenderer()->SetMesh(mesh);
                obj->GetMeshRenderer()->SetPass(0);
            }
            {
                obj->AddComponent(make_unique<OBBBoxCollider>());
            }
        }
    }
}

void SceneDemo::Update()
{
    if (INPUT->GetButtonDown(KEY_TYPE::LBUTTON))
    {
        int32 mouseX = INPUT->GetMousePos().x;
        int32 mouseY = INPUT->GetMousePos().y;

        // Picking
        auto pickObj = CUR_SCENE->Pick(mouseX, mouseY);
        if (pickObj)
        {
            DBG->Log(pickObj->GetName() + " Picked!!");
        }
    }
}

void SceneDemo::Render()
{
    DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}