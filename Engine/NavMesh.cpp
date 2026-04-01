#include "pch.h"
#include "NavMesh.h"
#include "Renderer.h"
#include "MeshRenderer.h"
#include "OnGUIUtils.h"
#include "GeometryHelper.h"

NavMesh::NavMesh() : Super(StaticType)
{
    GameObjectRef objRef = CUR_SCENE->Add("WalkableMesh");
    GameObject* obj = objRef.Resolve();
    obj->AddComponent(make_unique<MeshRenderer>());
    _debugMeshRenderer = obj->GetFixedComponentRef<MeshRenderer>();
    MeshRenderer* walkableMeshRenderer = _debugMeshRenderer.Resolve();

    walkableMeshRenderer->SetMaterial(RESOURCES->GetResourceRefByPath<Material>("Materials\\DebugMat.mat"));
    ResourceRef<Mesh> meshRef = RESOURCES->AllocateTempResource<Mesh>();
    walkableMeshRenderer->SetMesh(meshRef);
    obj->SetActive(false);

    
    _builder.SetDebugOnMarkWalkableTriangles([this](const vector<InputTri>& tris)
        {
            if (_debugOption != NavDebugOption::MarkWalkable)
                return;

            MeshRenderer* meshRenderer = _debugMeshRenderer.Resolve();
            if (meshRenderer == nullptr)
                return;
            meshRenderer->GetGameObject()->SetActive(true);

            Mesh* mesh = meshRenderer->GetMesh().Resolve();
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

    _heightFieldDebugFunc = ([this](const HeightField& heightField)
        {
            MeshRenderer* meshRenderer = _debugMeshRenderer.Resolve();
            meshRenderer->GetGameObject()->SetActive(true);
            if (meshRenderer == nullptr)
                return;

            const int width = heightField.GetWidth();
            const int depth = heightField.GetDepth();
            const float cellSize = heightField.GetCellSize();
            const float halfCellSize = cellSize * 0.5f;
            const float cellHeight = heightField.GetCellHeight();
            const vector<vector<Span>>& columns = heightField.GetColumns();

            Mesh* mesh = meshRenderer->GetMesh().Resolve();
            ASSERT(mesh != nullptr);
            auto srcGeometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
            vector<VertexTextureNormalTangentData> vertices;
            vector<uint32> indices;

            for (int cx = 0; cx < width; cx++)
            {
                for (int cz = 0; cz < depth; cz++)
                {
                    vector<Span> column = columns[heightField.GetColumnIndex(cx, cz)];
                    if (column.empty())
                        continue;

                    for (int i = 0; i < column.size(); ++i)
                    {
                        const Span& span = column[i];

                        Vec3 origin;
                        heightField.GetWorldPos(cx, cz, origin.x, origin.z);
                        origin.y = (span.cminY + span.cmaxY) * 0.5f * cellHeight + heightField.GetBoundMin().y;

                        float worldHeight = (span.cmaxY - span.cminY) * cellHeight;

                        GeometryHelper::CreateCube(srcGeometry, halfCellSize, worldHeight * 0.5f, halfCellSize, origin);
                        uint32 baseIndex = static_cast<uint32>(vertices.size());
                        const auto& srcVertices = srcGeometry->GetVertices();
                        for (VertexTextureNormalTangentData v : srcVertices)
                        {
                            Vec3 normalAsColor = GetDebugColor(span.area);
                            v.normal = normalAsColor;
                            vertices.push_back(v);
                        }
                        const auto& srcIndices = srcGeometry->GetIndices();
                        for (const auto& idx : srcIndices)
                        {
                            indices.push_back(baseIndex + idx);
                        }
                    }
                }
            }

            auto dstGeometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
            dstGeometry->SetVertices(vertices);
            dstGeometry->SetIndices(indices);
            mesh->CreateFromGeometry(dstGeometry);
        });

    _builder.SetDebugOnBuildHeightField([this](const HeightField& heightField)
        {
            if (_debugOption != NavDebugOption::BuildHeightField)
                return;
            _heightFieldDebugFunc(heightField);
        });

    _builder.SetDebugOnFilterHeightField([this](const HeightField& heightField)
        {
            if (_debugOption != NavDebugOption::FilterHeightField)
                return;
            _heightFieldDebugFunc(heightField);
        });

    _builder.SetDebugOnCompactHeightField([this](const CompactHeightField& heightField)
        {
            if (_debugOption != NavDebugOption::CompactHeightField)
                return;

            MeshRenderer* meshRenderer = _debugMeshRenderer.Resolve();
            if (meshRenderer == nullptr)
                return;
            meshRenderer->GetGameObject()->SetActive(true);

            const int width = heightField.GetWidth();
            const int depth = heightField.GetDepth();
            const float cellSize = heightField.GetCellSize();
            const float halfCellSize = cellSize * 0.5f;
            const float cellHeight = heightField.GetCellHeight();
            const vector<CompactCell>& cells = heightField.GetCells();
            const vector<CompactSpan>& spans = heightField.GetSpans();

            Mesh* mesh = meshRenderer->GetMesh().Resolve();
            ASSERT(mesh != nullptr);
            auto srcGeometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
            vector<VertexTextureNormalTangentData> vertices;
            vector<uint32> indices;

            for (int cx = 0; cx < width; cx++)
            {
                for (int cz = 0; cz < depth; cz++)
                {
                    CompactCell cell = cells[heightField.GetColumnIndex(cx, cz)];
                    if (cell.count == 0)
                        continue;

                    for (int i = 0; i < cell.count; ++i)
                    {
                        const int spanIdx = cell.index + i;
                        const CompactSpan& span = spans[spanIdx];

                        Vec3 origin;
                        heightField.GetWorldPos(cx, cz, origin.x, origin.z);
                        uint16 ceiling = std::min<uint16>(span.h, span.y + 3);
                        origin.y = (span.y + ceiling) * 0.5f * cellHeight + heightField.GetBoundMin().y;

                        float worldHeight = (ceiling - span.y) * cellHeight;

                        GeometryHelper::CreateCube(srcGeometry, halfCellSize, worldHeight * 0.5f, halfCellSize, origin);
                        uint32 baseIndex = static_cast<uint32>(vertices.size());
                        const auto& srcVertices = srcGeometry->GetVertices();
                        for (VertexTextureNormalTangentData v : srcVertices)
                        {
                            Vec3 normalAsColor = GetDebugColor(span.region);
                            v.normal = normalAsColor;
                            vertices.push_back(v);
                        }
                        const auto& srcIndices = srcGeometry->GetIndices();
                        for (const auto& idx : srcIndices)
                        {
                            indices.push_back(baseIndex + idx);
                        }
                    }
                }
            }

            auto dstGeometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
            dstGeometry->SetVertices(vertices);
            dstGeometry->SetIndices(indices);
            mesh->CreateFromGeometry(dstGeometry);
        });
}

NavMesh::~NavMesh()
{
}

bool NavMesh::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();

    bool buildNavMesh = false;
    {
        bool showWalkableChanged = OnGUIUtils::DrawEnumCombo("DebugOption", _debugOption, NavDebugNames, (int)NavDebugOption::Max);
        changed |= showWalkableChanged;
        if (showWalkableChanged)
        {
            MeshRenderer* meshRenderer = _debugMeshRenderer.Resolve();
            if (meshRenderer != nullptr)
            {
                meshRenderer->GetGameObject()->SetActive(_debugOption != NavDebugOption::None);
            }

            if (_debugOption != NavDebugOption::None)
                buildNavMesh = true;
        }
    }
    changed |= OnGUIUtils::DrawVec3("Build Extent", &_buildExtent, 1.f);

    if (ImGui::Button("Build NavMesh") || buildNavMesh)
    {
        Vec3 curPos = GetTransform()->GetPosition();
        Bounds bounds{ -_buildExtent, _buildExtent };
        bounds.bmin += curPos;
        bounds.bmax += curPos;

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

Vec3 NavMesh::GetDebugColor(int id)
{
    switch (id)
    {
    case 0: return Vec3(1.0f, 0.0f, 0.0f); // Red
    case 1: return Vec3(0.0f, 1.0f, 0.0f); // Green
    case 2: return Vec3(0.0f, 0.0f, 1.0f); // Blue
    case 3: return Vec3(1.0f, 1.0f, 0.0f); // Yellow
    case 4: return Vec3(1.0f, 0.0f, 1.0f); // Magenta
    case 5: return Vec3(0.0f, 1.0f, 1.0f); // Cyan
    case 6: return Vec3(1.0f, 0.5f, 0.0f); // Orange
    case 7: return Vec3(0.5f, 0.0f, 1.0f); // Purple
    case 8: return Vec3(0.5f, 1.0f, 0.5f); // Light Green
    case 9: return Vec3(1.0f, 0.5f, 0.5f); // Light Red
    default: return Vec3(0.3f, 0.3f, 0.3f); // Default Gray
    }
}
