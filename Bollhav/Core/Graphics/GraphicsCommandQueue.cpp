#include "GraphicsCommandQueue.h"

GraphicsCommandQueue::GraphicsCommandQueue(ID3D12Device4* _device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type					  = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask				  = 1;
	TIF(_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_pCommandQueue)));
}

ID3D12CommandQueue* GraphicsCommandQueue::GetCommandQueue() const
{
	return m_pCommandQueue.Get();
}
