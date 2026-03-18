#pragma once
#include "Component.h"

#define PROJECTION_TYPE_LIST(X) \
    X(Perspective)              \
    X(Orthographic) \

enum class ProjectionType
{
#define X(name) name,
	PROJECTION_TYPE_LIST(X)
#undef X

	Max
};

static const char* ProjectionTypeNames[] =
{
#define X(name) #name,
	PROJECTION_TYPE_LIST(X)
#undef X
};

class Camera :  public Component
{
	using Super = Component;
public:
    static constexpr ComponentType StaticType = ComponentType::Camera;
	
	Camera();
	virtual ~Camera();
	
	void OnSize();

	virtual void LateUpdate() override;

	void SetProjectionType(ProjectionType type) { _type = type; }
	ProjectionType GetProjectionType() { return _type; }

	void UpdateMatrix();

	void SetNear(float value) { _near = value; }
	void SetFar(float value) { _far = value; SsaoOnSize(); }
	void SetFOV(float value) { _fov = value; SsaoOnSize(); }
	void SetWidth(float value) { _width = value; }
	void SetHeight(float value) { _height = value; }
	void SsaoOnSize();

	Matrix& GetViewMatrix() { return _matView; }
	Matrix& GetProjectionMatrix() { return _matProjection; }

	float GetWidht() { return _width; }
	float GetHeight() { return _height; }
    float GetNear() { return _near; }
    float GetFar() { return _far; }
    float GetFOV() { return _fov; }

    virtual bool OnGUI() override;
    template<typename Archive>
	void serialize(Archive& ar)
	{
		Super::serialize(ar);
		ar(
            CEREAL_NVP(_type),
            CEREAL_NVP(_near),
            CEREAL_NVP(_far),
            CEREAL_NVP(_fov),
            CEREAL_NVP(_width),
            CEREAL_NVP(_height),
            CEREAL_NVP(_cullingMask)
		);

        if (Archive::is_loading::value)
        {
			SsaoOnSize();
        }
	}

private:
	ProjectionType _type = ProjectionType::Perspective;
	Matrix _matView = Matrix::Identity;
	Matrix _matProjection = Matrix::Identity;

	float _near = 1.f;
	float _far = 1000.f;
	float _fov = XM_PI / 4.f;
	float _width = 0.f;
	float _height = 0.f;

public:
	static Matrix S_MatView;
	static Matrix S_MatProjection;
	static Vec3 S_Pos;

public:
	void SortGameObject();
	void SetStaticData();
	void Render_Forward(RenderTech renderTech);
	void Render_Backward(RenderTech renderTech);

	void SetCullingMaskLayerOnOff(uint8 layer, bool on)
	{
		if (on)
			_cullingMask |= (1 << layer);
		else
			_cullingMask &= ~(1 << layer);
	}

	void SetCullingMaskAll() { SetCullingMask(UINT32_MAX); }
	void SetCullingMask(uint32 mask) { _cullingMask = mask; }
	bool IsCulled(uint8 layer) { return (_cullingMask & (1 << layer)) != 0; }

private:
	uint32 _cullingMask = 0;
	vector<GameObject*> _vecForward;
	vector<GameObject*> _vecBackward;
};