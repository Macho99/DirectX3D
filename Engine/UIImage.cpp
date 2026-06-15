#include "pch.h"
#include "UIImage.h"
#include "GameObject.h"
#include "Geometry.h"
#include "Material.h"
#include "Mesh.h"
#include "MeshRenderer.h"
#include "OnGUIUtils.h"
#include "RectTransform.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "VertexData.h"

UIImage::UIImage()
    : Super(StaticType)
{
}

UIImage::~UIImage()
{
}

void UIImage::Awake()
{
    EnsureMeshRenderer();
    ApplyMaterial();
    _isDirty = true;
}

void UIImage::Start()
{
    RebuildMesh();
}

void UIImage::Update()
{
    const Vec2 size = GetCurrentSize();
    if (size != _lastSize)
        _isDirty = true;

    if (_isDirty)
        RebuildMesh();
}

bool UIImage::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();

    if (OnGUIUtils::DrawResourceRef("Texture", _texture))
    {
        SetTexture(_texture);
        changed = true;
    }

    if (OnGUIUtils::DrawResourceRef("Material", _material))
    {
        SetMaterial(_material);
        changed = true;
    }

    bool colorChanged = OnGUIUtils::DrawColor("Color", &_color);
    changed |= colorChanged;
    if (colorChanged)
        SetColor(_color);

    if (OnGUIUtils::DrawBool("Preserve Aspect", &_preserveAspect))
    {
        _isDirty = true;
        changed = true;
    }

    return changed;
}

void UIImage::SetTexture(ResourceRef<Texture> texture)
{
    _texture = texture;
    _isDirty = true;
    ApplyMaterial();
}

void UIImage::SetMaterial(ResourceRef<Material> material)
{
    _material = material;
    ApplyMaterial();
}

void UIImage::SetColor(const Color& color)
{
    _color = color;
    ApplyMaterial();
}

void UIImage::SetPreserveAspect(bool preserveAspect)
{
    if (_preserveAspect != preserveAspect)
    {
        _preserveAspect = preserveAspect;
        _isDirty = true;
    }
}

void UIImage::EnsureMeshRenderer()
{
    GameObject* gameObject = GetGameObject();
    if (gameObject == nullptr)
        return;

    gameObject->SetLayerIndex(Layer_UI);

    MeshRenderer* meshRenderer = gameObject->GetMeshRenderer();
    if (meshRenderer == nullptr)
    {
        gameObject->AddComponent(make_unique<MeshRenderer>());
        meshRenderer = gameObject->GetMeshRenderer();
    }

    _meshRenderer = gameObject->GetFixedComponentRef<MeshRenderer>();

    if (_mesh.IsValid() == false)
        _mesh = RESOURCES->AllocateTempResource<Mesh>();

    if (meshRenderer != nullptr)
    {
        meshRenderer->SetMesh(_mesh);
        meshRenderer->SetPass(0);
    }
}

void UIImage::RebuildMesh()
{
    _isDirty = false;
    EnsureMeshRenderer();

    Mesh* mesh = _mesh.Resolve();
    if (mesh == nullptr)
        return;

    Vec2 fillSize = Vec2(1.0f, 1.0f);
    _lastSize = GetCurrentSize();

    Texture* texture = _texture.Resolve();
    if (_preserveAspect && texture != nullptr)
    {
        const Vec2 textureSize = texture->GetSize();
        if (textureSize.x > 0.0f && textureSize.y > 0.0f && _lastSize.x > 0.0f && _lastSize.y > 0.0f)
        {
            const float imageAspect = textureSize.x / textureSize.y;
            const float rectAspect = std::abs(_lastSize.x) / std::abs(_lastSize.y);
            if (imageAspect > rectAspect)
                fillSize.y = rectAspect / imageAspect;
            else
                fillSize.x = imageAspect / rectAspect;
        }
    }

    const float left = -0.5f * fillSize.x;
    const float right = 0.5f * fillSize.x;
    const float bottom = -0.5f * fillSize.y;
    const float top = 0.5f * fillSize.y;
    const Vec3 normal = Vec3(0.0f, 0.0f, -1.0f);
    const Vec3 tangent = Vec3(1.0f, 0.0f, 0.0f);

    auto geometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
    geometry->AddVertex(VertexTextureNormalTangentData{ Vec3(left, bottom, 0.0f), Vec2(0.0f, 1.0f), normal, tangent });
    geometry->AddVertex(VertexTextureNormalTangentData{ Vec3(left, top, 0.0f), Vec2(0.0f, 0.0f), normal, tangent });
    geometry->AddVertex(VertexTextureNormalTangentData{ Vec3(right, bottom, 0.0f), Vec2(1.0f, 1.0f), normal, tangent });
    geometry->AddVertex(VertexTextureNormalTangentData{ Vec3(right, top, 0.0f), Vec2(1.0f, 0.0f), normal, tangent });
    geometry->AddIndices({ 0, 1, 2, 2, 1, 3 });

    mesh->CreateFromGeometry(geometry);
    ApplyMaterial();
}

void UIImage::ApplyMaterial()
{
    EnsureMeshRenderer();

    Material* material = _material.Resolve();
    if (material == nullptr)
    {
        _material = RESOURCES->AllocateUIDefaultMaterial();
        material = _material.Resolve();
    }

    MeshRenderer* meshRenderer = _meshRenderer.Resolve();
    if (meshRenderer != nullptr)
        meshRenderer->SetMaterial(_material);

    material->SetDiffuseMap(_texture);
    material->GetMaterialDesc().diffuse = _color;
}

Vec2 UIImage::GetCurrentSize() const
{
    Transform* transform = const_cast<UIImage*>(this)->GetTransform();
    if (transform == nullptr)
        return Vec2(1.0f, 1.0f);

    RectTransform* rectTransform = dynamic_cast<RectTransform*>(transform);
    if (rectTransform != nullptr)
        return rectTransform->GetSize();

    const Vec3 scale = transform->GetScale();
    return Vec2(scale.x, scale.y);
}
