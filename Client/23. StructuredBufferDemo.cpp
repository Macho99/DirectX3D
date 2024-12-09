#include "pch.h"
#include "23. StructuredBufferDemo.h"
#include "GeometryHelper.h"
#include "Camera.h"
#include "GameObject.h"
#include "CameraMove.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Material.h"
#include "Model.h"
#include "ModelRenderer.h"
#include "ModelAnimator.h"
#include "Mesh.h"
#include "Transform.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Light.h"
#include "Scene.h"
#include "TextureBuffer.h"
#include "StructuredBuffer.h"

void StructuredBufferDemo::Init()
{
	_shader = make_shared<Shader>(L"22. StructuredBufferDemo.fx");

	vector<Matrix> inputs(500, Matrix::Identity);

	auto buffer = make_shared<StructuredBuffer>(inputs.data(), sizeof(Matrix), 500, sizeof(Matrix), 500);

	_shader->GetSRV("Input")->SetResource(buffer->GetSRV().Get());
	_shader->GetUAV("Output")->SetUnorderedAccessView(buffer->GetUAV().Get());

	_shader->Dispatch(0, 0, 1, 1, 1);

	vector<Matrix> outputs(500);
	buffer->CopyFromOutput(outputs.data());


}

void StructuredBufferDemo::Update()
{
}

void StructuredBufferDemo::Render()
{

}