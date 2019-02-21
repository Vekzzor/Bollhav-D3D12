#include "ConstantBuffer.h"
#include "pch.h"

ConstantBuffer::ConstantBuffer(ID3D12Device4* _pDevice, ID3D12DescriptorHeap* _pHeap, size_t _size)
	: m_dataSize(_size)
{
	UINT cbSizeAligned = (m_dataSize + 255) & ~255;
	D3D12_HEAP_PROPERTIES heapProp;
	heapProp.Type				  = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CPUPageProperty	  = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask	 = 1;
	heapProp.VisibleNodeMask	  = 1;

	D3D12_RESOURCE_DESC desc;
	desc.Alignment			= 0;
	desc.DepthOrArraySize   = 1;
	desc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Flags				= D3D12_RESOURCE_FLAG_NONE;
	desc.Format				= DXGI_FORMAT_UNKNOWN;
	desc.Height				= 1;
	desc.Width				= cbSizeAligned;
	desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.MipLevels			= 1;
	desc.SampleDesc.Count   = 1;
	desc.SampleDesc.Quality = 0;

	TIF(_pDevice->CreateCommittedResource(&heapProp,
										  D3D12_HEAP_FLAG_NONE,
										  &desc,
										  D3D12_RESOURCE_STATE_GENERIC_READ,
										  nullptr,
										  IID_PPV_ARGS(&m_pResource)));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation					= m_pResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cbSizeAligned; // CB size is required to be 256-byte aligned.
	_pDevice->CreateConstantBufferView(&cbvDesc, _pHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_RANGE range;
	range.Begin = 0;
	range.End   = 0;
	m_pResource->Map(0, &range, reinterpret_cast<void**>(&m_pDataBegin));
}

void ConstantBuffer::SetData(void* _pData)
{
	memcpy(m_pDataBegin, _pData, m_dataSize);
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetVirtualAddress() const
{
	return m_pResource->GetGPUVirtualAddress();
}
