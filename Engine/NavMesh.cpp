#include "pch.h"
#include "NavMesh.h"
#include "Renderer.h"
#include "MeshRenderer.h"

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
                Vec2 uv;
                if (tri.walkable)
                    uv = Vec2(0.f, 0.f);
                else
                    uv = Vec2(0.2f, 0.f);

                uint32 baseIndex = static_cast<uint32>(vertices.size());
                vertices.push_back(VertexTextureNormalTangentData{ tri.v0, uv, Vec3(0.f), Vec3(0.f) });
                vertices.push_back(VertexTextureNormalTangentData{ tri.v1, uv, Vec3(0.f), Vec3(0.f) });
                vertices.push_back(VertexTextureNormalTangentData{ tri.v2, uv, Vec3(0.f), Vec3(0.f) });
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
