#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(ID3D12Device4* _pDevice, void* _pData, size_t _dataSize)
{
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
	desc.Width				= _dataSize;
	desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.MipLevels			= 1;
	desc.SampleDesc.Count   = 1;
	desc.SampleDesc.Quality = 0;

	TIF(_pDevice->CreateCommittedResource(&heapProp,
										  D3D12_HEAP_FLAG_NONE,
										  &desc,
										  D3D12_RESOURCE_STATE_GENERIC_READ,
										  nullptr,
										  IID_PPV_ARGS(&m_pVertexData)));

	UINT* pVertexDataBegin;
	D3D12_RANGE range;
	range.Begin = 0;
	range.End   = 0;
	m_pVertexData->Map(0, &range, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, _pData, _dataSize);
	m_pVertexData->Unmap(0, nullptr);

	m_BufferView.BufferLocation = m_pVertexData->GetGPUVirtualAddress();
	m_BufferView.SizeInBytes	= _dataSize;
	m_BufferView.StrideInBytes  = 6 * sizeof(float); // TODO(Henrik): Fix this!
}

const D3D12_VERTEX_BUFFER_VIEW& VertexBuffer::GetVertexView() const
{
	return m_BufferView;
}
