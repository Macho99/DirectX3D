#include "pch.h"
#include "AssimpTool.h"
#include "Converter.h"

void AssimpTool::Init()
{
	//{
	//	shared_ptr<Converter> converter = make_shared<Converter>();
	//	converter->ReadAssetFile(L"Kachujin/Mesh.fbx");
	//	converter->ExportMaterialData(L"Kachujin/Kachujin");
	//	converter->ExportModelData(L"Kachujin/Kachujin");
	//}
	//{
	//	shared_ptr<Converter> converter = make_shared<Converter>();
	//	converter->ReadAssetFile(L"Kachujin/Idle.fbx");
	//	converter->ExportAnimationData(L"Kachujin/Idle");
	//}
	//{
	//	shared_ptr<Converter> converter = make_shared<Converter>();
	//	converter->ReadAssetFile(L"Kachujin/Run.fbx");
	//	converter->ExportAnimationData(L"Kachujin/Run");
	//}
	//{
	//	shared_ptr<Converter> converter = make_shared<Converter>();
	//	converter->ReadAssetFile(L"Kachujin/Slash.fbx");
	//	converter->ExportAnimationData(L"Kachujin/Slash");
	//}
	{
		shared_ptr<Converter> converter = make_shared<Converter>();
		converter->ReadAssetFile(L"Tree1/Tree.fbx");
		converter->ExportModelData(L"Tree1/Tree");
        converter->ExportMaterialData(L"Tree1/Tree");
	}
}

void AssimpTool::Update()
{
}

void AssimpTool::Render()
{
}
