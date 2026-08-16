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
#include "NavMesh.h"
#include "NavAgent.h"
#include "MathUtils.h"
#include "SsrRenderer.h"
#include "Font.h"
#include "Text.h"
#include "UIImage.h"
#include "ThirdPersonCamMove.h"
#include "TargetFollower.h"
#include "PlayerAnimEventHandler.h"
#include "EditorCamController.h"

void AssetSerializeDemo::Init()
{
    auto sky = make_shared<Sky>();
    sky->SetMaterial(RESOURCES->GetResourceRefByPath<Material>("Materials\\SkyMat.mat"));
    CUR_SCENE->SetSky(sky);

    const float baseHeight = 8.f;

    ResourceRef<Shader> renderShader = RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\19. RenderDemo.fx");

    {
        // Light
        GameObjectRef light = CUR_SCENE->Add("Light");
        light.Resolve()->AddComponent(make_unique<Light>());

        LightDesc lightDesc;
        lightDesc.ambient = Vec4(0.4f);
        lightDesc.diffuse = Vec4(1.f);
        lightDesc.specular = Vec4(0.15f);
        light.Resolve()->GetTransform()->SetRotation(MathUtils::RadToDeg(Vec3(1.f, -1.f, 1.f)));
        static_cast<Light*>(light.Resolve()->GetFixedComponent(ComponentType::Light))->SetLightDesc(lightDesc);
    }

    {
        ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\VeigarMaterial.mat");
        for (int32 i = 0; i < 1; i++)
        {
            auto objRef = CUR_SCENE->Add("Veigar");
            GameObject* obj = objRef.Resolve();
            obj->GetTransform()->SetLocalPosition(Vec3(0, baseHeight + 2, 0));
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
        tessTerrain->SetPositionOffset(Vec3::Up * 4);
        tessTerrain->SetTerrainData(RESOURCES->GetResourceRefByPath<TerrainData>(L"Textures\\Terrain\\TerrainData.terrain"));

        ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\TerrainMat.mat");
        tessTerrain->SetMaterial(materialRef);
        tessTerrain->SetBrushTexture(RESOURCES->GetResourceRefByPath<Texture>(L"Textures\\Terrain\\Brush\\circleBrush.png"));

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
        obj->SetActive(false);
        obj->GetTransform()->SetLocalPosition(Vec3(0.f));
        AssetId uvAssetId;
        RESOURCES->TryGetAssetIdByPath(L"Textures\\Grass\\Grass_A_BaseColor_Split.txt", OUT uvAssetId);
        AssetRef uvAsset = AssetRef(uvAssetId);
        auto grassRenderer = make_unique<GrassRenderer>(grassComputeShader, tessTerrainRef, uvAsset);
        {
            // Material
            {
                ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\GrassRenderMat.mat");
                grassRenderer->SetMaterial(materialRef);
            }
        }
        obj->AddComponent(std::move(grassRenderer));
    
        auto foliageController = make_unique<FoliageController>();
        obj->AddComponent(std::move(foliageController));
    }

    //Particle
    {
        auto particleShader = RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\ParticleSystem.fx");
        auto objRef = CUR_SCENE->Add("Fire");
        GameObject* obj = objRef.Resolve();
        obj->SetActive(false);
        obj->GetTransform()->SetLocalPosition(Vec3(0.f, baseHeight + 5, 0.f));
        obj->AddComponent(make_unique<ParticleSystem>());
        ParticleSystem* particleSystem = obj->GetFixedComponent<ParticleSystem>(ComponentType::ParticleSystem);
        particleSystem->SetEmitDirW(Vec3(0.f, 2.f, 0.f));

        ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\FireMat.mat");
        particleSystem->SetMaterial(materialRef);
    }

    // SnowBillboard
    {	// Billboard
        {
            //auto snowShader = RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\24. SnowDemo.fx");
            auto objRef = CUR_SCENE->Add("Rain");
            GameObject* obj = objRef.Resolve();
            obj->SetActive(false);
            obj->GetTransform()->SetLocalPosition(Vec3(0.f));
            obj->AddComponent(make_unique<SnowBillboard>());
            {
                // Material
                {
                    ResourceRef<Material> materialRef = RESOURCES->GetResourceRefByPath<Material>(L"Materials\\SnowMat.mat");
                    obj->GetSnowBillboard()->SetMaterial(materialRef);
                }
                obj->GetSnowBillboard()->SetExtent(Vec3(100, 100, 100));
                obj->GetSnowBillboard()->SetVelocity(Vec3(0, -30, 0));
                obj->GetSnowBillboard()->SetDrawCount(2000);
            }
        }
    }

    {
        //animShader->SetTechNum(RenderTech::NormalDepth, -1);
        // Animation
        ResourceRef<Model> modelRef = RESOURCES->GetResourceRefByPath<Model>(L"Models\\Paladin\\Paladin.model");
        auto parentObjRef = CUR_SCENE->Add("Paladins");
        auto parentTransformRef = parentObjRef.Resolve()->GetFixedComponentRef<Transform>();

        auto objRef = CUR_SCENE->Add("Paladin");
        GameObject* obj = objRef.Resolve();
        {
            Transform* objTransform = obj->GetTransform();
            objTransform->SetPosition(Vec3{ -4.f, baseHeight, 65.f });
            objTransform->SetRotation(Vec3{ 0.f, 90.f, 0.f });
            objTransform->SetScale(Vec3(0.01f));
            objTransform->SetParent(parentTransformRef);
            obj->AddComponent(make_unique<ModelAnimator>());
            {
                obj->GetModelAnimator()->SetModel(modelRef);
                obj->GetModelAnimator()->SetPass(2);
            }
            obj->AddComponent(make_unique<PlayerAnimEventHandler>());
        }
        
        {
            GameObjectRef cameraRef = CUR_SCENE->Add("Camera");
            GameObject* camera = cameraRef.Resolve();
            {
                camera->AddComponent(make_unique<Camera>());
                camera->GetCamera()->SetCullingMaskLayerOnOff(Layer_UI, true);
                camera->GetCamera()->SetFar(500.f);
            }

            auto followerObjRef = CUR_SCENE->Add("TargetFollower");
            GameObject* followerObj = followerObjRef.Resolve();

            followerObj->GetTransform()->SetParent(parentTransformRef);
            Transform* followerTransform = followerObjRef.Resolve()->GetTransform();
            TransformRef followerTransformRef = followerTransform->GetRef();
            camera->GetTransform()->SetParent(followerTransformRef);
            camera->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, -4.f));

            {
                unique_ptr<TargetFollower> targetFollower = make_unique<TargetFollower>();
                targetFollower->SetTarget(obj->GetTransform());
                targetFollower->SetPositionOffset(Vec3(0.f, 1.5f, 0.f));
                targetFollower->SetFollowPositionX(true);
                targetFollower->SetFollowPositionY(true);
                targetFollower->SetFollowPositionZ(true);
                targetFollower->SetPositionFollowMode(TargetPositionFollowMode::Interpolated);
                followerObj->AddComponent(std::move(targetFollower));
            }

            {
                unique_ptr<ThirdPersonCamMove> thirdPersonCamMove = make_unique<ThirdPersonCamMove>();
                thirdPersonCamMove->SetTarget(obj->GetTransformRef());
                camera->AddComponent(std::move(thirdPersonCamMove));
            }
            camera->AddComponent(make_unique<CameraMove>());
            camera->AddComponent(make_unique<EditorCamController>());
        }
    }

    {
        auto parentObjRef = CUR_SCENE->Add("Towers");
        auto parentTransformRef = parentObjRef.Resolve()->GetFixedComponentRef<Transform>();
        //parentTransformRef.Resolve()->SetScale(Vec3(3.f, 1.f, 3.f));

        // Model
        ResourceRef<Model> model = RESOURCES->GetResourceRefByPath<Model>(L"Models\\Tower\\Tower.fbx");
        for (int32 i = 0; i < 5; i++)
        {
            auto objRef = CUR_SCENE->Add("Tower" + std::to_string(i));
            GameObject* obj = objRef.Resolve();
            obj->GetTransform()->SetPosition(Vec3(rand() % 100, baseHeight, rand() % 100));
            obj->GetTransform()->SetParent(parentTransformRef);

            auto modelRenderer = make_unique<ModelRenderer>();
            modelRenderer->SetModel(model);
            obj->AddComponent(std::move(modelRenderer));
        }
    }

    ResourceRef<Shader> foliageShader(RESOURCES->GetResourceRefByPath<Shader>(L"Shaders\\Foliage.fx"));
    {
        auto parentObjRef = CUR_SCENE->Add("Trees");
        auto parentTransformRef = parentObjRef.Resolve()->GetFixedComponentRef<Transform>();

        // Model
        //shared_ptr<Model> m2 = make_shared<Model>();
        //m2->ReadModel(L"Tree1/Tree");
        //m2->ReadMaterial(L"Tree1/Tree");
        ResourceRef<Model> m2(RESOURCES->GetResourceRefByPath<Model>(L"Models\\Tree1\\TreeModel.model"));
        for (int32 i = 0; i < 10; i++)
        {
            auto objRef = CUR_SCENE->Add("Tree" + std::to_string(i));
            GameObject* obj = objRef.Resolve();
            obj->GetTransform()->SetPosition(Vec3(rand() % 100, baseHeight - 1, rand() % 100));
            obj->GetTransform()->SetParent(parentTransformRef);

            obj->AddComponent(make_unique<ModelRenderer>());
            {
                obj->GetModelRenderer()->SetModel(m2);
            }

            auto foliageController = make_unique<FoliageController>();
            foliageController->SetBendFactor(5.f);
            obj->AddComponent(std::move(foliageController));
        }

        Vec3 windDir = Vec3(1.f, 0.f, 1.f);
        windDir.Normalize();
        FoliageController::S_WindDesc.windDirection = windDir;
        FoliageController::S_WindDesc.waveFrequency = 0.1f;
        FoliageController::S_WindDesc.windStrength = 1.f;
    }

    {
        // NavMesh
        auto objRef = CUR_SCENE->Add("NavMesh");
        GameObject* obj = objRef.Resolve();
        obj->AddComponent(make_unique<NavMesh>());
    }

    {
        auto objRef = CUR_SCENE->Add("Water");
        GameObject* obj = objRef.Resolve();
        obj->AddComponent(make_unique<SsrRenderer>());
        SsrRenderer* ssrRenderer = obj->GetFixedComponentRef<SsrRenderer>().Resolve();
        ssrRenderer->SetMaterial(RESOURCES->GetResourceRefByPath<Material>(L"Materials\\WaterSSRMat.mat"));
        ssrRenderer->SetMesh(RESOURCES->GetQuadYMesh());

        Transform* transform = obj->GetTransform();
        transform->SetPosition(Vec3(30.f, baseHeight - 0.5f, 120.f));
        transform->SetScale(Vec3(200.f, 1.f, 200.f));
    }

    auto uiObjRef = CUR_SCENE->Add("UI", true);
    Transform* uiTransform = uiObjRef.Resolve()->GetTransform();
    {
        auto objRef = CUR_SCENE->Add("Text", uiTransform);
        GameObject* obj = objRef.Resolve();
        obj->AddComponent(make_unique<Text>());
        obj->SetLayerIndex(Layer_UI);

        auto textRef = obj->GetFixedComponentRef<Text>().Resolve();
        textRef->SetFont(RESOURCES->GetResourceRefByPath<Font>(L"Fonts\\arial.fnt"));
        textRef->SetMaterial(RESOURCES->GetResourceRefByPath<Material>(L"Materials\\ArialFontMat.mat"));
    }

    {
        auto objRef = CUR_SCENE->Add("ImageTop", uiTransform);
        GameObject* obj = objRef.Resolve();
        obj->AddComponent(make_unique<UIImage>());
        UIImage* uiImage = obj->GetUIImage();
        obj->SetLayerIndex(Layer_UI);
    }

    {
        auto objRef = CUR_SCENE->Add("UICamera");
        GameObject* obj = objRef.Resolve();
        obj->GetTransform()->SetPosition(Vec3{ 0.f, 0.f, -5.f });
        obj->AddComponent(make_unique<Camera>());

        Camera* camera = obj->GetCamera();
        camera->SetProjectionType(ProjectionType::Orthographic);
        camera->SetCullingMaskAll();
        camera->SetCullingMaskLayerOnOff(Layer_UI, false);
    }

    {
        auto objRef = CUR_SCENE->Add("Impulse");
        GameObject* obj = objRef.Resolve();
        obj->SetActive(false);
        MeshRenderer* meshRenderer = obj->AddComponent<MeshRenderer>().Resolve();
        meshRenderer->SetMesh(RESOURCES->GetSphereMesh());
        meshRenderer->SetMaterial(RESOURCES->GetResourceRefByPath<Material>(L"Materials\\ImpulseMat.mat"));
        Transform* transform = obj->GetTransform();
        transform->SetPosition(Vec3(0.f, baseHeight + 4.f, 67.f));
        transform->SetScale(Vec3(10.f));
    }

    {
        auto objRef = CUR_SCENE->Add("ProbsBatcing");
        GameObject* obj = objRef.Resolve();
        obj->AddComponent<ModelRenderer>();
        obj->GetFixedComponent<ModelRenderer>()->SetModel(RESOURCES->GetResourceRefByPath<Model>(L"Batches\\Rural_Cabin\\Probs.model"));
    }
    {
        auto objRef = CUR_SCENE->Add("ModularBatcing");
        GameObject* obj = objRef.Resolve();
        obj->AddComponent<ModelRenderer>();
        obj->GetFixedComponent<ModelRenderer>()->SetModel(RESOURCES->GetResourceRefByPath<Model>(L"Batches\\Rural_Cabin\\Modular.model"));
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