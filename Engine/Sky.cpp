#include "pch.h"
#include "Sky.h"
#include "Camera.h"
#include "Material.h"

Sky::Sky(const std::wstring& cubemapFilename, const wstring& shaderFileName)
{
	//_cubeMapSRV = Utils::LoadTexture(device, cubemapFilename);
	_texture = make_shared<Texture>();
	_texture->Load(cubemapFilename);

	//GeometryGenerator::MeshData sphere;
	//GeometryGenerator geoGen;
	//geoGen.CreateSphere(skySphereRadius, 30, 30, sphere);
	shared_ptr<Mesh> sphereMesh = RESOURCES->Get<Mesh>(L"Sphere");
	auto sphereGeometry = sphereMesh->GetGeometry();
	
	const vector<VertexTextureNormalTangentData>& geoVerteices = sphereGeometry->GetVertices();

	std::vector<Vec3> vertices(geoVerteices.size());
	for (size_t i = 0; i < geoVerteices.size(); ++i)
	{
		vertices[i] = geoVerteices[i].position;
	}

	//D3D11_BUFFER_DESC vbd;
	//vbd.Usage = D3D11_USAGE_IMMUTABLE;
	//vbd.ByteWidth = sizeof(XMFLOAT3) * vertices.size();
	//vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	//vbd.CPUAccessFlags = 0;
	//vbd.MiscFlags = 0;
	//vbd.StructureByteStride = 0;
	//
	//D3D11_SUBRESOURCE_DATA vinitData;
	//vinitData.pSysMem = &vertices[0];
	//
	//CHECK(DEVICE->CreateBuffer(&vbd, &vinitData, _vb.GetAddressOf()));
	_vb = make_shared<VertexBuffer>();
	_vb->Create(vertices, "SkyVB", 0, false, true);

	const vector<uint32>& geoindices = sphereGeometry->GetIndices();
	_indexCount = geoindices.size();

	//D3D11_BUFFER_DESC ibd;
	//ibd.Usage = D3D11_USAGE_IMMUTABLE;
	//ibd.ByteWidth = sizeof(USHORT) * _indexCount;
	//ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	//ibd.CPUAccessFlags = 0;
	//ibd.StructureByteStride = 0;
	//ibd.MiscFlags = 0;
	//
	//std::vector<USHORT> indices16;
	//indices16.assign(geoindices.begin(), geoindices.end());
	//
	//D3D11_SUBRESOURCE_DATA iinitData;
	//iinitData.pSysMem = &indices16[0];
	//
	//CHECK(DEVICE->CreateBuffer(&ibd, &iinitData, _ib.GetAddressOf()));
	_ib = make_shared<IndexBuffer>();
	_ib->Create(geoindices);

	_material = make_shared<Material>();
	_material->SetShader(make_shared<Shader>(shaderFileName));
	_material->SetCubeMap(_texture);
}

Sky::~Sky()
{
}

ComPtr<ID3D11ShaderResourceView> Sky::CubeMapSRV()
{
	return _texture->GetComPtr();
}

void Sky::Render(Camera* camera)
{
	// center Sky about eye in world space
	//XMFLOAT3 eyePos = camera.GetPosition();
	//XMMATRIX T = ::XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);

	//XMMATRIX WVP = ::XMMatrixMultiply(T, camera.ViewProj());

	//Effects::SkyFX->SetWorldViewProj(WVP);
	//Effects::SkyFX->SetCubeMap(_cubeMapSRV.Get());
	Vec3 eyePos = camera->GetTransform()->GetPosition();
	Matrix world = Matrix::CreateTranslation(eyePos);

	Matrix v = camera->GetViewMatrix();
	Matrix p = camera->GetProjectionMatrix();
	Matrix wvp = world * v * p;

	Shader* shader = _material->GetShader();
	shader->PushTransformData(TransformDesc(wvp));
	shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);
	_material->Update();

	//uint32 stride = sizeof(Vec3);
	//uint32 offset = 0;
	//DC->IASetVertexBuffers(0, 1, _vb.GetAddressOf(), &stride, &offset);
	//DC->IASetIndexBuffer(_ib.Get(), DXGI_FORMAT_R16_UINT, 0);
	_vb->PushData();
	_ib->PushData();
	//DC->IASetInputLayout(InputLayouts::Pos.Get());
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	shader->DrawIndexed(0, 0, _indexCount, 0, 0);

	//for (uint32 p = 0; p < techDesc.Passes; ++p)
	//{
	//	ComPtr<ID3DX11EffectPass> pass = Effects::SkyFX->SkyTech->GetPassByIndex(p);
	//
	//	pass->Apply(0, dc.Get());
	//
	//	dc->DrawIndexed(_indexCount, 0, 0);
	//}
}