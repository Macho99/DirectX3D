#pragma once
#include "Component.h"
#include "ComponentRef.h"

class Transform : public Component
{
	using Super = Component;
public:
	static Vec3 ToEulerAngles(Quaternion q);

	Transform();
	~Transform();

	virtual void Awake() override;
	virtual void Update() override;

	void UpdateTransform();

	// Local
	Vec3 GetLocalScale() { return _localScale; }
	void SetLocalScale(const Vec3& localScale) { _localScale = localScale; UpdateTransform(); }
	Vec3 GetLocalRotation() { return _localRotation; }
	void SetLocalRotation(const Vec3& localRotation) { _localRotation = localRotation; UpdateTransform(); }
	Vec3 GetLocalPosition() { return _localPosition; }
	void SetLocalPosition(const Vec3& localPosition) { _localPosition = localPosition; UpdateTransform(); }

	// World
	Vec3 GetScale() { return _scale; }
	void SetScale(const Vec3& scale);
	Vec3 GetRotation() { return _rotation; }
	void SetRotation(const Vec3& rotation);
	Vec3 GetPosition() { return _position; }
	void SetPosition(const Vec3& position);

	Vec3 GetRight() { return _matWorld.Right(); }
	Vec3 GetUp() { return _matWorld.Up(); }
	Vec3 GetLook() { return _matWorld.Backward(); }

	void SetWorldMatrix(Matrix& newWorld);
	Matrix GetWorldMatrix() { return _matWorld; }

	// °èÃþ °ü°è
	bool HasParent() { return _parent.Resolve() != nullptr; }
    bool TryGetParent(OUT Transform*& outParent)
    {
        outParent = _parent.Resolve();
        return outParent != nullptr;
    }
	
	Transform* GetParent() { return _parent.Resolve(); }
	void SetParent(TransformRef& parent);
	void SetSiblingIndex(int index);

	vector<TransformRef>& GetChildren() { return _children; }

private:
    bool IsAncestorOf(TransformRef& target);
	void RemoveFromTransforms(vector<TransformRef>& transforms, TransformRef targetId);

private:
	Vec3 _localScale = { 1.f, 1.f, 1.f }; 
	Vec3 _localRotation = { 0.f, 0.f, 0.f };
	Vec3 _localPosition = { 0.f, 0.f, 0.f };

	// Cache
	Matrix _matLocal = Matrix::Identity;
	Matrix _matWorld = Matrix::Identity;
	
	Vec3 _scale;
	Vec3 _rotation;
	Vec3 _position;

private:
	TransformRef _parent;
	vector<TransformRef> _children;
};

