#include "CommandList.h"
#include "pch.h"

CommandList::CommandList(ID3D12Device4* _pDevice, ID3D12CommandAllocator* _pInitialAlloc)
{
	TIF(_pDevice->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, _pInitialAlloc, nullptr, IID_PPV_ARGS(&m_pCommandList)));

	// TODO(Henrik): Pass a bool if it should be closed
	//TIF(m_pCommandList->Close());

	NAME_D3D12_OBJECT(m_pCommandList);
}

void CommandList::Prepare(ID3D12CommandAllocator* _pAllocator, ID3D12Resource* _pRenderTarget)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pCurrentRenderTarget = _pRenderTarget;
	barrier.Transition.Subresource						  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore						  = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter						  = D3D12_RESOURCE_STATE_RENDER_TARGET;

	m_pCommandList->Reset(_pAllocator, nullptr);
	m_pCommandList->ResourceBarrier(1, &barrier);
}

void CommandList::Finish()
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = m_pCurrentRenderTarget;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;

	m_pCommandList->ResourceBarrier(1, &barrier);
	m_pCommandList->Close();
}

ID3D12GraphicsCommandList* CommandList::operator->(void)
{
	return m_pCommandList.Get();
}

ID3D12GraphicsCommandList* CommandList::GetPtr()
{
	return m_pCommandList.Get();
}
