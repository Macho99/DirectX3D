#pragma once
#include "IExecute.h"
#include "Geometry.h"

class GameObject;

class BillboardDemo : public IExecute
{
public:
	void Init() override;
	void Update() override;
	void Render() override;

private:
	void AddDebugImage(int32 width, int32 height, shared_ptr<Texture> texture, int techNum);

private:
	int32 _debugImagePosX;
};