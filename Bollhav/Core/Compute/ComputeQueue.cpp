#include "ComputeQueue.h"
#include "pch.h"

ComputeQueue::ComputeQueue(ID3D12Device4* _pDevice)
	: m_FenceSignal(0)
{
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	cqd.Type					 = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	cqd.Priority				 = 0;
	cqd.Flags					 = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cqd.NodeMask				 = 0;
	TIF(_pDevice->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_pCommandQueue)));
	NAME_D3D12_OBJECT(m_pCommandQueue);

	TIF(_pDevice->CreateFence(m_FenceSignal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
	NAME_D3D12_OBJECT(m_pFence);
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void ComputeQueue::SubmitList(ID3D12GraphicsCommandList* _pCmdList)
{
	m_pCommandListsToExecute.push_back(_pCmdList);
}

void ComputeQueue::Execute()
{
	m_pCommandQueue->ExecuteCommandLists(m_pCommandListsToExecute.size(),
										 m_pCommandListsToExecute.data());
	m_pCommandListsToExecute.clear();
}

void ComputeQueue::WaitForGPU()
{
	TIF(m_pCommandQueue->Signal(m_pFence.Get(), m_FenceSignal));
	TIF(m_pFence->SetEventOnCompletion(m_FenceSignal, m_hFenceEvent));
	m_FenceSignal++;
	WaitForSingleObject(m_hFenceEvent, INFINITE);
}
