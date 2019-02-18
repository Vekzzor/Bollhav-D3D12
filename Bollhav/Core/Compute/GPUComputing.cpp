#include "GPUComputing.h"
#include "pch.h"

GPUComputing::GPUComputing() {}

GPUComputing::~GPUComputing() {}

int GPUComputing::init(ID3D12Device4* device,
					   ID3DBlob* computeBlob,
					   ID3D12RootSignature* rootSignature)
{
	m_CreateCommandInterface(device);
	m_CreatePipeLineState(device, computeBlob, rootSignature); 
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

void GPUComputing::m_CreatePipeLineState(ID3D12Device4* device,
									   ID3DBlob* computeBlob,
									   ID3D12RootSignature* rootSignature)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC cpsd = {};
	cpsd.pRootSignature					   = rootSignature;
	cpsd.CS.pShaderBytecode				   = computeBlob->GetBufferPointer();
	cpsd.CS.BytecodeLength				   = computeBlob->GetBufferSize();
	cpsd.NodeMask						   = 0;

	TIF(device->CreateComputePipelineState(&cpsd, IID_PPV_ARGS(&m_pPipelineState)));
}
