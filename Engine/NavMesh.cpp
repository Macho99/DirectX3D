#include "pch.h"
#include "NavMesh.h"
#include "Renderer.h"
#include "MeshRenderer.h"
#include "OnGUIUtils.h"

NavMesh::NavMesh() : Super(StaticType)
{
    _builder.SetDebugOnMarkWalkableTriangles([this](const vector<InputTri>& tris)
        {
            MeshRenderer* walkableMeshRenderer = _walkableMeshRenderer.Resolve();

            if (walkableMeshRenderer == nullptr)
            {
                GameObjectRef objRef = CUR_SCENE->Add("WalkableMesh");
                objRef.Resolve()->AddComponent(make_unique<MeshRenderer>());
                _walkableMeshRenderer = objRef.Resolve()->GetFixedComponentRef<MeshRenderer>();
                walkableMeshRenderer = _walkableMeshRenderer.Resolve();

                walkableMeshRenderer->SetMaterial(RESOURCES->GetResourceRefByPath<Material>("Materials\\DebugMat.mat"));
                ResourceRef<Mesh> meshRef = RESOURCES->AllocateTempResource<Mesh>();
                walkableMeshRenderer->SetMesh(meshRef);
            }

            Mesh* mesh = walkableMeshRenderer->GetMesh().Resolve();
            ASSERT(mesh != nullptr);
            auto geometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
            vector<VertexTextureNormalTangentData> vertices;
            vector<uint32> indices;
            for (const auto& tri : tris)
            {
                Vec3 normalAsColor;
                if (tri.walkable)
                    normalAsColor = Vec3(0.f, 1.f, 0.f);
                else
                    normalAsColor = Vec3(1.f, 0.f, 0.f);

                //Vec2 uv = tri.walkable ? _walkableUV : _unwalkableUV;

                uint32 baseIndex = static_cast<uint32>(vertices.size());
                vertices.push_back(VertexTextureNormalTangentData{ tri.v0, Vec2(0.f), normalAsColor, Vec3(0.f) });
                vertices.push_back(VertexTextureNormalTangentData{ tri.v1, Vec2(0.f), normalAsColor, Vec3(0.f) });
                vertices.push_back(VertexTextureNormalTangentData{ tri.v2, Vec2(0.f), normalAsColor, Vec3(0.f) });
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex + 2);
            }
            geometry->SetVertices(vertices);
            geometry->SetIndices(indices);
            mesh->CreateFromGeometry(geometry);
        });
}

NavMesh::~NavMesh()
{
}

bool NavMesh::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    changed |= OnGUIUtils::DrawVec2("WalkableUV", &_walkableUV, 0.01f);
    changed |= OnGUIUtils::DrawVec2("UnwalkableUV", &_unwalkableUV, 0.01f);


    if (ImGui::Button("Build NavMesh"))
    {
        Bounds bounds{ Vec3(-10, -10, -10), Vec3(10, 10, 10) };
        NavBuildInput input;

        const auto& objs = CUR_SCENE->GetObjects();
        for (auto& gameObjectRef : objs)
        {
            GameObject* object = gameObjectRef.Resolve();
            ASSERT(object != nullptr);

            Renderer* renderer = object->GetRenderer();
            if (renderer == nullptr)
                continue;

            renderer->SubmitTriangles(bounds, input.triangles);
        }
        bool result = _builder.Build(input, "InvalidPath");
    }

    return false;
}
