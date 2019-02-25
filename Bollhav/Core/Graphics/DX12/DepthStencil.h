#pragma once

class DepthStencil
{
public:
	DepthStencil(ID3D12Device4* _pDevice, UINT64 _Width, UINT64 _Height);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart();
	void Clear(ID3D12GraphicsCommandList* _pCmdList, D3D12_CLEAR_FLAGS _Flags = D3D12_CLEAR_FLAG_DEPTH);

private:
	float m_fDepthValue;
	ComPtr<ID3D12Resource> m_pDepthResource;
	ComPtr<ID3D12DescriptorHeap> m_pDSVHeap;
};