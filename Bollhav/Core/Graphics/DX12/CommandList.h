#pragma once

class CommandList
{
public:
	CommandList() = default;
	CommandList(ID3D12Device4* _pDevice, ID3D12CommandAllocator* _pInitialAlloc);

	void Prepare(ID3D12CommandAllocator* _pAllocator, ID3D12Resource* _pRenderTarget);
	void Finish();

	ID3D12GraphicsCommandList* operator->(void);
	ID3D12GraphicsCommandList* GetPtr();

private:
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;

	ID3D12Resource* m_pCurrentRenderTarget;
};