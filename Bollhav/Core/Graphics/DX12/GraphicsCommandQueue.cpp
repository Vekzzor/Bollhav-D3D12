#include "GraphicsCommandQueue.h"

GraphicsCommandQueue::GraphicsCommandQueue(ID3D12Device4* _pDevice)
	: m_FenceSignal(0)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type					  = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask				  = 1;
	TIF(_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_pCommandQueue)));
	NAME_D3D12_OBJECT(m_pCommandQueue);

	TIF(_pDevice->CreateFence(m_FenceSignal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
	NAME_D3D12_OBJECT(m_pFence);
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void GraphicsCommandQueue::SubmitList(ID3D12CommandList* _pCommandList)
{
	m_pExecuteList.push_back(_pCommandList);
}

void GraphicsCommandQueue::Execute()
{
	m_pCommandQueue->ExecuteCommandLists(static_cast<UINT>(m_pExecuteList.size()),
										 m_pExecuteList.data());
	m_pExecuteList.clear();
}

ID3D12CommandQueue* GraphicsCommandQueue::GetCommandQueue() const
{
	return m_pCommandQueue.Get();
}

void GraphicsCommandQueue::WaitForGPU()
{
	TIF(m_pCommandQueue->Signal(m_pFence.Get(), m_FenceSignal));
	TIF(m_pFence->SetEventOnCompletion(m_FenceSignal, m_hFenceEvent));
	m_FenceSignal++;
	WaitForSingleObject(m_hFenceEvent, INFINITE);
}
