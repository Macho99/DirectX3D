#include "pch.h"
#include "NavMesh.h"
#include "../NavBuild/NavMeshBuilder.h"
#include "Renderer.h"

NavMesh::NavMesh() : Super(StaticType)
{
}

NavMesh::~NavMesh()
{
}

bool NavMesh::OnGUI()
{
    if (ImGui::Button("Build NavMesh"))
    {
        NavMeshBuilder builder;
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
        bool result = builder.Build(input, "InvalidPath");
    }

    return false;
}
