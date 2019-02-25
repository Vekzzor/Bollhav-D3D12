#pragma once

class ConstantBuffer
{
public:
	ConstantBuffer(ID3D12Device4* _pDevice, ID3D12DescriptorHeap* _pHeap, size_t _size);

	void SetData(void* _pData);
	D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() const;

private:
	UINT* m_pDataBegin;
	size_t m_dataSize;
	ComPtr<ID3D12Resource> m_pResource;
};