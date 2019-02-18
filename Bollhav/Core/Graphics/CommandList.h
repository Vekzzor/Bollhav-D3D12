#pragma once

class CommandList
{
public:
	CommandList(ID3D12Device4* _pDevice, ID3D12CommandAllocator* _pInitialAlloc);
	ID3D12GraphicsCommandList* operator->(void);
	ID3D12GraphicsCommandList* GetPtr();
private:
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
};