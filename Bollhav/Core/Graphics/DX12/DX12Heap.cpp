#include "DX12Heap.h"
#include "pch.h"

DX12Heap::DX12Heap() {}

DX12Heap::~DX12Heap() {}

void DX12Heap::CreateWithCurrentSettings(ID3D12Device4* device)
{
	if(m_descriptionSet)
		TIF(device->CreateHeap(m_heapDesc, IID_PPV_ARGS(m_heap)));
	else
	{
		std::cout << "FAILED TO CREATE HEAP: DESCRIPTION NOT SET" << std::endl;
		exit(0);
	}
}

void DX12Heap::InsertResource(ID3D12Device4* device, UINT offset, D3D12_RESOURCE_DESC resDesc,
	D3D12_RESOURCE_STATES resState, D3D12_CLEAR_VALUE clearValue, ID3D12Resource* res)
{
	if (m_heapCreated)
	{
		device->CreatePlacedResource(
			m_heap.Get(), offset, resDesc, resState, clearValue, IID_PPV_ARGS(res));
	}
	else
	{
		std::cout << "RESOURCE COULD NOT BE INSERTED: NO HEAP IS CREATED." << std::endl; 
		exit(0); 
	}
}

DX12Heap* DX12Heap::Get()
{
	return m_heap.Get();
}

void DX12Heap::SetDesc(UINT64 SizeInBytes,
					   D3D12_HEAP_PROPERTIES Properties,
					   UINT64 Alignment,
					   D3D12_HEAP_FLAGS Flags)
{
	if(m_propertiesSet)
	{
		m_heapDesc.SizeInBytes = SizeInBytes;
		m_heapDesc.Properties  = Properties;
		m_heapDesc.Alignment   = Alignment;
		m_heapDesc.Flags	   = Flags;

		m_descriptionSet = true;
	}
	else
	{
		std::cout << "FAILED TO CREATE HEAP DESCRIPTION: PROPERTIES NOT SET" << std::endl;
		exit(0);
	}
}

void DX12Heap::SetProperties(D3D12_HEAP_TYPE Type,
							 D3D12_CPU_PAGE_PROPERTY CPUPageProperty,
							 D3D12_MEMORY_POOL MemoryPoolPreference,
							 UINT CreationNodeMask,
							 UINT VisibleNodeMask)
{
	m_heapProperties.Type				  = Type;
	m_heapProperties.CPUPageProperty	  = CPUPageProperty;
	m_heapProperties.MemoryPoolPreference = MemoryPoolPreference;
	m_heapProperties.CreationNodeMask	 = CreationNodeMask;
	m_heapProperties.VisibleNodeMask	  = VisibleNodeMask;

	m_propertiesSet = true;
}

D3D12_HEAP_DESC DX12Heap::GetDesc()
{
	return m_heapDesc;
}

D3D12_HEAP_PROPERTIES DX12Heap::GetProperties()
{
	return m_heapProperties;
}
