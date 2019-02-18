#include "CommandList.h"
#include "pch.h"

CommandList::CommandList(ID3D12Device4* _pDevice, ID3D12CommandAllocator* _pInitialAlloc)
{
	TIF(_pDevice->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, _pInitialAlloc, nullptr, IID_PPV_ARGS(&m_pCommandList)));
	TIF(m_pCommandList->Close());
}

ID3D12GraphicsCommandList* CommandList::operator->(void)
{
	return m_pCommandList.Get();
}

ID3D12GraphicsCommandList* CommandList::GetPtr()
{
	return m_pCommandList.Get();
}

