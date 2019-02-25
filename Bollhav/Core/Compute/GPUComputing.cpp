#include "GPUComputing.h"

GPUComputing::GPUComputing() {}

GPUComputing::~GPUComputing() {}

int GPUComputing::init(ID3D12Device4* device)
{
	m_CreateCommandInterface(device);
	m_CreatePipeLineState(device); 
	return 0;
}

void GPUComputing::m_CreateCommandInterface(ID3D12Device4* device)
{
	D3D12_COMMAND_QUEUE_DESC descComputeQueue = {};
	descComputeQueue.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
	descComputeQueue.Type					  = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	TIF(device->CreateCommandQueue(&descComputeQueue, IID_PPV_ARGS(&m_pComputeQueue)));
	TIF(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
												 IID_PPV_ARGS(&m_pComputeAllocator)));
	TIF(device->CreateCommandList(0,
											D3D12_COMMAND_LIST_TYPE_COMPUTE,
											m_pComputeAllocator.Get(),
											nullptr,
											IID_PPV_ARGS(&m_pComputeList)));
	m_pComputeList->Close();

	m_pComputeAllocator->SetName(L"Compute Allocator");
	m_pComputeQueue->SetName(L"Compute Queue");
	m_pComputeList->SetName(L"Compute list");
}

void GPUComputing::m_CreatePipeLineState(ID3D12Device4* device)
{
	Microsoft::WRL::ComPtr<ID3DBlob> computeBlob;
	TIF(D3DCompileFromFile(L"Shaders/ComputeShader.hlsl",
						   nullptr,
						   nullptr,
						   "CS_main",
						   "cs_5_1",
						   0,
						   0,
						   &computeBlob,
						   nullptr));
	{
		D3D12_DESCRIPTOR_RANGE dtRanges[2];
		dtRanges[0].RangeType						  = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		dtRanges[0].NumDescriptors					  = 1;
		dtRanges[0].BaseShaderRegister				  = 0;
		dtRanges[0].RegisterSpace					  = 0;
		dtRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		dtRanges[1].RangeType						  = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		dtRanges[1].NumDescriptors					  = 1;
		dtRanges[1].BaseShaderRegister				  = 0;
		dtRanges[1].RegisterSpace					  = 0;
		dtRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_DESCRIPTOR_TABLE dt;
		dt.NumDescriptorRanges = ARRAYSIZE(dtRanges);
		dt.pDescriptorRanges   = dtRanges;

		D3D12_ROOT_PARAMETER rootParam[1];
		rootParam[0].ParameterType	= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam[0].DescriptorTable  = dt;
		rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rsDesc;
		rsDesc.Flags			 = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		rsDesc.NumParameters	 = ARRAYSIZE(rootParam);
		rsDesc.pParameters		 = rootParam;
		rsDesc.NumStaticSamplers = 0;
		rsDesc.pStaticSamplers   = nullptr;

		ID3DBlob* sBlob;
		D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sBlob, nullptr);

		device->CreateRootSignature(
			0, sBlob->GetBufferPointer(), sBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC cpsd = {};
	cpsd.pRootSignature					   = m_pRootSignature.Get();
	cpsd.CS.pShaderBytecode				   = computeBlob->GetBufferPointer();
	cpsd.CS.BytecodeLength				   = computeBlob->GetBufferSize();
	cpsd.NodeMask						   = 0;

	TIF(device->CreateComputePipelineState(&cpsd, IID_PPV_ARGS(&m_pPipelineState)));
}
