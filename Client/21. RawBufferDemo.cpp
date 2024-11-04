#include "pch.h"
#include "21. RawBufferDemo.h"
#include "RawBuffer.h"

void RawBufferDemo::Init()
{
	_shader = make_shared<Shader>(L"20. RawBufferDemo.fx");

	// 하나의 쓰레드 그룹 내에서 운영할 쓰레드 개수
	uint32 threadCount = 10 * 8 * 3;
	uint32 groupCount = 2 * 1 * 1;
	uint32 count = threadCount * groupCount;

	vector<Input> inputs(count);
	for (int32 i = 0; i < count; i++)
		inputs[i].value = rand() % 10000;

	shared_ptr<RawBuffer> rawBuffer = make_shared<RawBuffer>(inputs.data(), sizeof(Input) * count, sizeof(Output) * count);
	_shader->GetSRV("Input")->SetResource(rawBuffer->GetSRV().Get());
	_shader->GetUAV("Output")->SetUnorderedAccessView(rawBuffer->GetUAV().Get());
	
	_shader->Dispatch(0, 0, 2, 1, 1);

	vector<Output> outputs(count);
	rawBuffer->CopyFromOutput(outputs.data());

	FILE* file;
	::fopen_s(&file, "../RawBuffer.csv", "w");

	::fprintf
	(
		file,
		"GroupID(X),GroupID(Y),GroupID(Z),GroupThreadID(X),GroupThreadID(Y),GroupThreadID(Z),DispatchThreadID(X),DispatchThreadID(Y),DispatchThreadID(Z),GroupIndex,Value\n"
	);

	for (uint32 i = 0; i < count; i++)
	{
		const Output& temp = outputs[i];

		::fprintf
		(
			file,
			"%d,%d,%d,	%d,%d,%d,	%d,%d,%d,	%d,%f\n",
			temp.groupID[0], temp.groupID[1], temp.groupID[2],
			temp.groupThreadID[0], temp.groupThreadID[1], temp.groupThreadID[2],
			temp.dispatchThreadID[0], temp.dispatchThreadID[1], temp.dispatchThreadID[2],
			temp.groupIndex, temp.value
		);
	}

	::fclose(file);
}

void RawBufferDemo::Update()
{
}

void RawBufferDemo::Render()
{

}
