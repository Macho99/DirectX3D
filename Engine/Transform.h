#pragma once
#include "Component.h"

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

	Matrix GetWorldMatrix() { return _matWorld; }

	// °èÃþ °ü°è
	bool HasParent() { return _parent.lock() != nullptr; }
    bool TryGetParent(OUT shared_ptr<Transform>& outParent)
    {
        outParent = _parent.lock();
        return outParent != nullptr;
    }
	
	shared_ptr<Transform> GetParent() { return _parent.lock(); }
	void SetParent(shared_ptr<Transform> parent);
	void SetSiblingIndex(int index);

	vector<shared_ptr<Transform>>& GetChildren() { return _children; }

    TransformID GetID() const { return _id; }

private:
    bool IsAncestorOf(shared_ptr<Transform>& transform);
	void RemoveFromTransforms(vector<shared_ptr<Transform>>& transforms, TransformID targetId);

private:
	TransformID _id;

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
	weak_ptr<Transform> _parent;
	vector<shared_ptr<Transform>> _children;
};

