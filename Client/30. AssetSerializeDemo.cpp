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
#include "TessTerrain.h"
#include "GrassRenderer.h"

void AssetSerializeDemo::Init()
{
    CUR_SCENE->SetSky(make_shared<Sky>(L"..\\Resources\\Textures\\Sky\\snowcube1024.dds", L"Shaders\\Sky.fx"));

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
        //lightDesc.direction = Vec3(1.f, -1.f, 1.f);
        light.Resolve()->GetTransform()->SetRotation(Vec3(1.f, -1.f, 1.f));
        static_cast<Light*>(light.Resolve()->GetFixedComponent(ComponentType::Light))->SetLightDesc(lightDesc);
    }
    {
        ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\VeigarMaterial.mat");
        // Mesh
        // Material
        {
            //Material* material = materialRef.Resolve();
            //material->SetShader(renderShader);
            //auto texture = RESOURCES->GetResourceRefByPath<Texture>(L"Textures\\veigar.jpg");
            //material->SetDiffuseMap(texture);
            //MaterialDesc& desc = material->GetMaterialDesc();
            //desc.ambient = Vec4(1.f);
            //desc.diffuse = Vec4(1.f);
            //desc.specular = Vec4(1.f);
            //RESOURCES->Add(L"Veigar", material);
        }

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

    ComponentRef<TessTerrain> tessTerrainRef;
    {
        unique_ptr<TessTerrain> tessTerrain = make_unique<TessTerrain>();
        //TessTerrain::InitInfo info;
        //ZeroMemory(&info, sizeof(info));
        //info.heightMapFilename = L"../Resources/Textures/Terrain/terrain.raw";
        //info.layerMapFilename0 = L"../Resources/Textures/Terrain/grass.dds";
        //info.layerMapFilename1 = L"../Resources/Textures/Terrain/darkdirt.dds";
        //info.layerMapFilename2 = L"../Resources/Textures/Terrain/stone.dds";
        //info.layerMapFilename3 = L"../Resources/Textures/Terrain/lightdirt.dds";
        //info.layerMapFilename4 = L"../Resources/Textures/Terrain/snow.dds";
        //info.blendMapFilename = L"../Resources/Textures/Terrain/blend.dds";
        //info.heightScale = 50.0f;
        //info.heightmapWidth = 2049;
        //info.heightmapHeight = 2049;
        //info.cellSpacing = 0.5f;
        //tessTerrain->Init();
        tessTerrain->SetTerrainData(RESOURCES->GetResourceRefByPath<TerrainData>(L"Textures\\Terrain\\TerrainData.terrain"));

        ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\TerrainMat.mat");
        //Material* material = materialRef.Resolve();
        //material->SetShader(RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\Terrain.fx"));
        //material->SetRenderQueue(RenderQueue::Opaque);
        //material->GetMaterialDesc().ambient = Vec4(1.f);
        tessTerrain->SetMaterial(materialRef);

        auto objRef = CUR_SCENE->Add("Terrain");
        objRef.Resolve()->GetTransform()->SetPosition(Vec3(0, 0, 0));
        objRef.Resolve()->AddComponent(std::move(tessTerrain));
        tessTerrainRef = objRef.Resolve()->GetFixedComponentRef<TessTerrain>();
    }

    {
        //ResourceRef<Shader> grassRenderShader = RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\GrassRender.fx");
        ResourceRef<Shader> grassComputeShader = RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\GrassCompute.fx");
        auto objRef = CUR_SCENE->Add("GrassRenderer");
        GameObject* obj = objRef.Resolve();
        obj->GetTransform()->SetLocalPosition(Vec3(0.f));
        auto grassRenderer = make_unique<GrassRenderer>(grassComputeShader, tessTerrainRef, L"..\\Assets\\Textures\\Grass\\Grass_A_BaseColor_Split.txt");
        {
            // Material
            {
                ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\GrassRenderMat.mat");
                //Material* material = materialRef.Resolve();
                //material->SetShader(grassRenderShader);
                //auto texture = RESOURCES->GetResourceRefByPath<Texture>(L"Textures\\Grass\\Grass_A_BaseColor.tif");
                ////auto texture = RESOURCES->Load<Texture>(L"Veigar", L"..\\Resources\\Textures\\veigar.jpg");
                //material->SetDiffuseMap(texture);
                //MaterialDesc& desc = material->GetMaterialDesc();
                //desc.ambient = Vec4(1.f);
                //desc.diffuse = Vec4(1.f);
                //desc.specular = Vec4(1.f);
                grassRenderer->SetMaterial(materialRef);
            }
        }
        obj->AddComponent(std::move(grassRenderer));
    
        auto foliageController = make_unique<FoliageController>();
        foliageController->SetBendFactor(0.1f);
        foliageController->SetStiffness(0.65f);
        obj->AddComponent(std::move(foliageController));
    }

    //Particle
    {
        auto particleShader = RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\ParticleSystem.fx");
        auto objRef = CUR_SCENE->Add("Fire");
        GameObject* obj = objRef.Resolve();
        obj->GetTransform()->SetLocalPosition(Vec3(0.f, 5.f, 0.f));
        obj->AddComponent(make_unique<ParticleSystem>());
        ParticleSystem* particleSystem = obj->GetFixedComponent<ParticleSystem>(ComponentType::ParticleSystem);
        particleSystem->SetEmitDirW(Vec3(0.f, 2.f, 0.f));

        ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\FireMat.mat");
        //Material* material = materialRef.Resolve();
        //material->GetMaterialDesc().diffuse = Vec4(1.f);
        //material->SetRenderQueue(RenderQueue::Transparent);
        //material->SetShader(particleShader);
        //auto texture = RESOURCES->GetResourceRefByPath<Texture>(L"Textures\\flare0.png");
        //material->SetDiffuseMap(texture);
        //material->SetRandomTex(true);

        particleSystem->SetMaterial(materialRef);
    }

    // SnowBillboard
    {	// Billboard
        {
            //auto snowShader = RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\24. SnowDemo.fx");
            auto objRef = CUR_SCENE->Add("Snow");
            GameObject* obj = objRef.Resolve();
            obj->GetTransform()->SetLocalPosition(Vec3(0.f));
            obj->AddComponent(make_unique<SnowBillboard>(Vec3(100, 100, 100), 10000));
            {
                // Material
                {
                    ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\SnowMat.mat");
                    //Material* material = materialRef.Resolve();
                    //material->SetShader(snowShader);
                    //auto texture = RESOURCES->GetResourceRefByPath<Texture>(L"Textures\\grass.png");
                    ////auto texture = RESOURCES->Load<Texture>(L"Veigar", L"..\\Resources\\Textures\\veigar.jpg");
                    //material->SetDiffuseMap(texture);
                    //MaterialDesc& desc = material->GetMaterialDesc();
                    //desc.ambient = Vec4(1.f);
                    //desc.diffuse = Vec4(1.f);
                    //desc.specular = Vec4(1.f);
                    obj->GetSnowBillboard()->SetMaterial(materialRef);
                }
            }
        }
    }

    {
        //animShader->SetTechNum(RenderTech::NormalDepth, -1);
        // Animation
        ResourceRef<Model> modelRef = RESOURCES->GetResourceRefByPath<Model>(L"Models\\Kachujin\\Mesh.fbx");
        Model* model = modelRef.Resolve();

        model->GetAnimations().clear();
        ResourceRef<ModelAnimation> animIdleRef = RESOURCES->GetResourceRefByPath<ModelAnimation>(L"Models\\Kachujin\\Idle.fbx");
        model->AddAnimation(animIdleRef);
        ResourceRef<ModelAnimation> animRunRef = RESOURCES->GetResourceRefByPath<ModelAnimation>(L"Models\\Kachujin\\Run.fbx");
        model->AddAnimation(animRunRef);
        ResourceRef<ModelAnimation> animSlashRef = RESOURCES->GetResourceRefByPath<ModelAnimation>(L"Models\\Kachujin\\Slash.fbx");
        model->AddAnimation(animSlashRef);

        for (int32 i = 0; i < 10; i++)
        {
            auto objRef = CUR_SCENE->Add("Kachujin" + std::to_string(i));
            GameObject* obj = objRef.Resolve();
            obj->GetTransform()->SetPosition(Vec3(rand() % 100, 0, rand() % 100));
            obj->GetTransform()->SetScale(Vec3(0.01f));
            obj->AddComponent(make_unique<ModelAnimator>(renderShader));
            {
                obj->GetModelAnimator()->SetModel(modelRef);
                obj->GetModelAnimator()->SetPass(2);
            }
        }
    }

    {
        // Model
        ResourceRef<Model> model = RESOURCES->GetResourceRefByPath<Model>(L"Models\\Tower\\Tower.fbx");
        for (int32 i = 0; i < 5; i++)
        {
            auto objRef = CUR_SCENE->Add("Tower" + std::to_string(i));
            GameObject* obj = objRef.Resolve();
            obj->GetTransform()->SetPosition(Vec3(rand() % 100, -1, rand() % 100));
            obj->GetTransform()->SetScale(Vec3(0.01f));

            auto modelRenderer = make_unique<ModelRenderer>(renderShader);
            modelRenderer->SetModel(model);
            modelRenderer->SetPass(1);
            obj->AddComponent(std::move(modelRenderer));
        }
    }

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
            DBG->Log(pickObj->GetName() + " Picked!!");
        }
    }
}

void AssetSerializeDemo::Render()
{
    DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}