#include "pch.h"
#include "NavMesh.h"
#include "Renderer.h"
#include "MeshRenderer.h"
#include "LineRenderer.h"
#include "OnGUIUtils.h"
#include "GeometryHelper.h"
#include "../NavBuild/Contours.h"

NavMesh::NavMesh() : Super(StaticType)
{
    {
        GameObjectRef objRef = CUR_SCENE->Add("NavDebug MeshRenderer");
        GameObject* obj = objRef.Resolve();
        obj->AddComponent(make_unique<MeshRenderer>());
        _debugMeshRenderer = obj->GetFixedComponentRef<MeshRenderer>();
        MeshRenderer* meshRenderer = _debugMeshRenderer.Resolve();

        meshRenderer->SetMaterial(RESOURCES->GetResourceRefByPath<Material>("Materials\\DebugMat.mat"));
        ResourceRef<Mesh> meshRef = RESOURCES->AllocateTempResource<Mesh>();
        meshRenderer->SetMesh(meshRef);
        obj->SetActive(false);
    }

    {
        _debugLineRendererParent = CUR_SCENE->Add("NavDebug LineRenderer Parent"); 
        GameObject* parentObj = _debugLineRendererParent.Resolve();
        parentObj->SetActive(false);
    }

    
    _builder.SetDebugOnMarkWalkableTriangles([this](const vector<InputTri>& tris)
        {
            if (TryInitializeDebugMesh(NavDebugOption::MarkWalkable) == false)
                return;
            MeshRenderer* meshRenderer = _debugMeshRenderer.Resolve();

            Mesh* mesh = meshRenderer->GetMesh().Resolve();
            ASSERT(mesh != nullptr);
            auto geometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
            vector<VertexTextureNormalTangentData> vertices;
            vector<uint32> indices;
            for (const auto& tri : tris)
            {
                Vec3 tangentAsColor;
                if (tri.walkable)
                    tangentAsColor = Vec3(0.f, 1.f, 0.f);
                else
                    tangentAsColor = Vec3(1.f, 0.f, 0.f);

                //Vec2 uv = tri.walkable ? _walkableUV : _unwalkableUV;

                uint32 baseIndex = static_cast<uint32>(vertices.size());
                vertices.push_back(VertexTextureNormalTangentData{ tri.v0, Vec2(0.f), Vec3(0.f), tangentAsColor});
                vertices.push_back(VertexTextureNormalTangentData{ tri.v1, Vec2(0.f), Vec3(0.f), tangentAsColor});
                vertices.push_back(VertexTextureNormalTangentData{ tri.v2, Vec2(0.f), Vec3(0.f), tangentAsColor});
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
                            Vec3 tangentAsColor = GetDebugColor(span.area);
                            v.tangent = tangentAsColor;
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
            if (TryInitializeDebugMesh(NavDebugOption::BuildHeightField) == false)
                return;
            _heightFieldDebugFunc(heightField);
        });

    _builder.SetDebugOnFilterHeightField([this](const HeightField& heightField)
        {
            if (TryInitializeDebugMesh(NavDebugOption::FilterHeightField) == false)
                return;
            _heightFieldDebugFunc(heightField);
        });

    _builder.SetDebugOnCompactHeightField([this](const CompactHeightField& heightField)
        {
            if (TryInitializeDebugMesh(NavDebugOption::CompactHeightField) == false)
                return;
            MeshRenderer* meshRenderer = _debugMeshRenderer.Resolve();

            const int width = heightField.GetWidth();
            const int depth = heightField.GetDepth();
            const float cellSize = heightField.GetCellSize();
            const float halfCellSize = cellSize * 0.5f;
            const float cellHeight = heightField.GetCellHeight();
            const vector<CompactCell>& cells = heightField.GetCells();
            const vector<CompactSpan>& spans = heightField.GetSpans();
            const vector<int>& dists = heightField.GetDistances();
            const int maxDist = heightField.GetMaxDist();
            vector<Int2> spanCoords(spans.size(), Int2{ -1, -1 });

            for (int cx = 0; cx < width; ++cx)
            {
                for (int cz = 0; cz < depth; ++cz)
                {
                    const CompactCell& cell = cells[heightField.GetColumnIndex(cx, cz)];
                    for (int i = 0; i < cell.count; ++i)
                    {
                        spanCoords[cell.index + i] = Int2{ cx, cz };
                    }
                }
            }

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
                        const int dist = dists[spanIdx];

                        Vec3 origin;
                        heightField.GetWorldPos(cx, cz, origin.x, origin.z);
                        uint16 ceiling = std::min<uint16>(span.h, span.y + 1);
                        origin.y = (span.y + ceiling) * 0.5f * cellHeight + heightField.GetBoundMin().y;

                        float worldHeight = (ceiling - span.y) * cellHeight;

                        GeometryHelper::CreateCube(srcGeometry, halfCellSize, worldHeight * 0.5f, halfCellSize, origin);
                        uint32 baseIndex = static_cast<uint32>(vertices.size());
                        const auto& srcVertices = srcGeometry->GetVertices();
                        for (VertexTextureNormalTangentData v : srcVertices)
                        {
                            Vec3 tangentAsColor = GetDebugColor(span.region);
                            //float normalizedDist = maxDist > 0 ? (float)dist / maxDist : 0.0f;
                            //Vec3 tangentAsColor = Vec3(normalizedDist);
                            v.tangent = tangentAsColor;
                            vertices.push_back(v);
                        }
                        const auto& srcIndices = srcGeometry->GetIndices();
                        for (const auto& idx : srcIndices)
                        {
                            indices.push_back(baseIndex + idx);
                        }
                        const float linkThickness = std::max(halfCellSize * 0.12f, 0.01f);
                        const float linkHalfLength = halfCellSize * 0.45f;
                        for (int dir = 0; dir < 4; ++dir)
                        {
                            const uint32 neighborSpanIdx = span.connections[dir];
                            if (neighborSpanIdx == NOT_CONNECTED)
                                continue;

                            const Int2& neighborCoord = spanCoords[neighborSpanIdx];
                            if (neighborCoord.x < 0 || neighborCoord.z < 0)
                                continue;

                            Vec3 neighborOrigin;
                            heightField.GetWorldPos(neighborCoord.x, neighborCoord.z, neighborOrigin.x, neighborOrigin.z);
                            const CompactSpan& neighborSpan = spans[neighborSpanIdx];
                            uint16 neighborCeiling = std::min<uint16>(neighborSpan.h, neighborSpan.y + 1);
                            neighborOrigin.y = (neighborSpan.y + neighborCeiling) * 0.5f * cellHeight + heightField.GetBoundMin().y;

                            Vec3 direction = neighborOrigin - origin;
                            if (direction.LengthSquared() <= 0.00001f)
                                continue;
                            direction.Normalize();

                            Vec3 linkCenter = origin + direction * (halfCellSize * 0.5f);
                            linkCenter.y = origin.y + worldHeight * 0.5f;

                            const bool alongX = std::abs(direction.x) > std::abs(direction.z);
                            const float halfX = alongX ? linkHalfLength : linkThickness;
                            const float halfZ = alongX ? linkThickness : linkHalfLength;

                            GeometryHelper::CreateCube(srcGeometry, halfX, linkThickness, halfZ, linkCenter);
                            baseIndex = static_cast<uint32>(vertices.size());
                            for (VertexTextureNormalTangentData v : srcGeometry->GetVertices())
                            {
                                v.tangent = Vec3(1.0f, 1.0f, 1.0f);
                                vertices.push_back(v);
                            }
                            for (const auto& idx : srcGeometry->GetIndices())
                            {
                                indices.push_back(baseIndex + idx);
                            }
                        }
                    }
                }
            }

            auto dstGeometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
            dstGeometry->SetVertices(vertices);
            dstGeometry->SetIndices(indices);
            mesh->CreateFromGeometry(dstGeometry);
        });

    _contoursDebugFunc = [this](const Contours& contours)
        {

            int count = 0;
            int colorId = 0;
            const auto& contoursData = contours.GetContours();
            for (const auto& contour : contoursData)
            {
                colorId++;
                for (const auto& loop : contour)
                {
                    count++;
                    EnsureLineRendererCount(count);
                    LineRenderer* lineRenderer = _debugLineRenderers[count - 1].Resolve();
                    lineRenderer->ClearPoints();
                    Vec3 color = GetDebugColor(colorId);
                    lineRenderer->SetColor(Color(color.x, color.y, color.z, 1.0f));
                    for (const auto& vertex : loop)
                    {
                        Vec3 worldPos;
                        contours.GetVertexWorldPos(vertex.x, vertex.z, worldPos.x, worldPos.z);
                        contours.GetWorldHeight(vertex.y, worldPos.y);

                        lineRenderer->AddPoint(worldPos);
                    }
                    lineRenderer->SetLoop(true);
                }
            }
            for (int i = 0; i < _debugLineRenderers.size(); i++)
            {
                ComponentRef<LineRenderer>& lineRendererRef = _debugLineRenderers[i];
                if (i < count)
                {
                    LineRenderer* lineRenderer = lineRendererRef.Resolve();
                    lineRenderer->GetGameObject()->SetActive(true);
                }
                else
                {
                    LineRenderer* lineRenderer = lineRendererRef.Resolve();
                    lineRenderer->GetGameObject()->SetActive(false);
                }
            }
        };

    _builder.SetDebugOnBuildContours([this](const Contours& contours)
        {
            if (TryInitializeDebugMesh(NavDebugOption::BuildContours, false) == false)
                return;

            _contoursDebugFunc(contours);
        });

    _builder.SetDebugOnSimplifyContours([this](const Contours& contours)
        {
            if (TryInitializeDebugMesh(NavDebugOption::SimplifyContours, false) == false)
                return;

            _contoursDebugFunc(contours);
        });

    _builder.SetDebugOnBuildPolyMesh([this](const Contours& contours)
        {
            if (TryInitializeDebugMesh(NavDebugOption::BuildPolyMesh, true) == false)
                return;

            MeshRenderer* meshRenderer = _debugMeshRenderer.Resolve();
            Mesh* mesh = meshRenderer->GetMesh().Resolve();
            ASSERT(mesh != nullptr);
            auto geometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
            vector<VertexTextureNormalTangentData> vertices;
            vector<uint32> indices;

            const auto& polyMeshs = contours.GetPolyMeshs();
            const auto& contoursData = contours.GetContours();
            for (int regionIdx = 0; regionIdx < polyMeshs.size(); regionIdx++)
            {
                const int indicesBase = static_cast<int>(vertices.size());

                const auto& tris = polyMeshs[regionIdx].front();
                const auto& contour = contoursData[regionIdx].front();
                Vec3 tangentAsColor = GetDebugColor(regionIdx + 1);
                for (const ContourVertex& vertex : contour)
                {
                    Vec3 worldPos;
                    contours.GetVertexWorldPos(vertex.x, vertex.z, worldPos.x, worldPos.z);
                    contours.GetWorldHeight(vertex.y, worldPos.y);
                    vertices.push_back(VertexTextureNormalTangentData{ worldPos, Vec2(0.f), Vec3(0.f), tangentAsColor });
                }
                const int invalidBase = static_cast<int>(vertices.size());
                for (const ContourVertex& vertex : contour)
                {
                    Vec3 worldPos;
                    contours.GetVertexWorldPos(vertex.x, vertex.z, worldPos.x, worldPos.z);
                    contours.GetWorldHeight(vertex.y, worldPos.y);
                    vertices.push_back(VertexTextureNormalTangentData{ worldPos, Vec2(0.f), Vec3(0.f), Vec3(1,0,0)});
                }

                for (int i = 0; i < tris.first.size(); ++i)
                {
                    const Triangle& tri = tris.first[i];
                    if (_debugInvalidTriangle == false && tri.isValid == false)
                        continue;

                    const int base = tri.isValid ? indicesBase : invalidBase;
                    indices.push_back(base + tri.i0);
                    indices.push_back(base + tri.i2);
                    indices.push_back(base + tri.i1);
                }

                if (tris.second.empty() == false)
                {
                    GameObject* parentObj = _debugLineRendererParent.Resolve();
                    parentObj->SetActive(true);
                    EnsureLineRendererCount(2);
                    for (int i = 0; i < tris.second.size(); ++i)
                    {
                        const ContourVertex& vertex = tris.second[i];
                        Vec3 worldPos;
                        contours.GetVertexWorldPos(vertex.x, vertex.z, worldPos.x, worldPos.z);
                        contours.GetWorldHeight(vertex.y, worldPos.y);
                        _debugLineRenderers[1].Resolve()->AddPoint(worldPos);
                    }
                }
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
    changed |= OnGUIUtils::DrawFloat("Contour Simplify Max Error", &_contourSimplifyMaxError, 0.1f);
    changed |= OnGUIUtils::DrawBool("Debug Invalid Triangle", &_debugInvalidTriangle);
    changed |= OnGUIUtils::DrawInt32("Debug Count", &_debugSeedCount, 1.f);

    if (ImGui::Button("Build NavMesh") || buildNavMesh)
    {
        Vec3 curPos = GetTransform()->GetPosition();
        Bounds bounds{ -_buildExtent, _buildExtent };
        bounds.bmin += curPos;
        bounds.bmax += curPos;

        NavBuildInput input;
        input.settings.contourMaxError = _contourSimplifyMaxError;
        input.settings.debugSeedCount = _debugSeedCount;

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

    return changed;
}

Vec3 NavMesh::GetDebugColor(int id)
{
    // Limit to 10 different colors for better visualization
    if(id != 0)
        id = id % 10 + 1;
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

bool NavMesh::TryInitializeDebugMesh(NavDebugOption option, bool useMeshRenderer)
{
    DBG->Log(Utils::Format("TryInitializeDebugMesh: %s", NavDebugNames[(int)option]));

    if (_debugOption != option)
        return false;

    MeshRenderer* meshRenderer = _debugMeshRenderer.Resolve();
    if (meshRenderer == nullptr)
        return false;
    meshRenderer->GetGameObject()->SetActive(useMeshRenderer);

    GameObject* parentObj = _debugLineRendererParent.Resolve();
    parentObj->SetActive(!useMeshRenderer);

    return true;
}

void NavMesh::EnsureLineRendererCount(int totalCount)
{
    while (totalCount > _debugLineRenderers.size())
    {
        GameObjectRef objRef = CUR_SCENE->Add("NavDebug LineRenderer");
        GameObject* obj = objRef.Resolve();
        obj->AddComponent(make_unique<LineRenderer>());
        TransformRef lineParent = _debugLineRendererParent.Resolve()->GetFixedComponentRef<Transform>();
        obj->GetTransform()->SetParent(lineParent);
        _debugLineRenderers.push_back(obj->GetFixedComponentRef<LineRenderer>());
    }
}
