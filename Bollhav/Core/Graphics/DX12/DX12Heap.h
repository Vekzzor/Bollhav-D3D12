#pragma once

class DX12Heap
{
private:
	ComPtr<ID3D12Heap> m_heap;
	D3D12_HEAP_DESC m_heapDesc;
	D3D12_HEAP_PROPERTIES m_heapProperties; 

	bool m_propertiesSet = false; 
	bool m_descriptionSet	 = false; 
	bool m_heapCreated		 = false; 

public:
	DX12Heap();
	~DX12Heap();

	void CreateWithCurrentSettings(ID3D12Device4* device); 

	void InsertResource(ID3D12Device4* device,
						UINT offset,
						D3D12_RESOURCE_DESC resDesc,
						D3D12_RESOURCE_STATES resState,
						D3D12_CLEAR_VALUE clearValue,
						ID3D12Resource* res); 

	DX12Heap* Get(); 

	void SetDesc(UINT64 SizeInBytes,
				 D3D12_HEAP_PROPERTIES Properties,
				 UINT64 Alignment,
				 D3D12_HEAP_FLAGS Flags);

	void SetProperties(D3D12_HEAP_TYPE Type, D3D12_CPU_PAGE_PROPERTY CPUPageProperty,
					   D3D12_MEMORY_POOL MemoryPoolPreference,
					   UINT CreationNodeMask,
					   UINT VisibleNodeMask); 

	D3D12_HEAP_DESC GetDesc(); 
	D3D12_HEAP_PROPERTIES GetProperties(); 
};