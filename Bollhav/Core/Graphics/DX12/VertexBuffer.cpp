#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(ID3D12Device4* _pDevice, VERTEX_BUFFER_DESC* _pDesc, DX12Heap* heap, UINT heapOffset)
{
	Create(_pDevice, _pDesc, heap, heapOffset);
}

void VertexBuffer::Create(ID3D12Device4* _pDevice, VERTEX_BUFFER_DESC* _pDesc, DX12Heap* heap, UINT heapOffset)
{
	D3D12_HEAP_PROPERTIES heapProp;
	heapProp.Type				  = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty	  = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask	 = 1;
	heapProp.VisibleNodeMask	  = 1;

	m_desc.Alignment			= 0;
	m_desc.DepthOrArraySize   = 1;
	m_desc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	m_desc.Flags				= D3D12_RESOURCE_FLAG_NONE;
	m_desc.Format				= DXGI_FORMAT_UNKNOWN;
	m_desc.Height				= 1;
	m_desc.Width				= _pDesc->SizeInBytes;
	m_desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	m_desc.MipLevels			= 1;
	m_desc.SampleDesc.Count   = 1;
	m_desc.SampleDesc.Quality = 0;


	ID3D12Resource* resource = heap->InsertResource(
		_pDevice, heapOffset, m_desc, D3D12_RESOURCE_STATE_COPY_DEST, m_pVertexData.Get()); 

	m_pVertexData.Attach(resource);
	//Just something to hold the data for transfer to the GPU.
	m_vbData.pData	  = m_pVertexData.Get();
	m_vbData.RowPitch   = sizeof(m_pVertexData);
	m_vbData.SlicePitch = m_vbData.RowPitch;

  
	//Init view
	m_BufferView.BufferLocation = m_pVertexData->GetGPUVirtualAddress();
	m_BufferView.SizeInBytes	= m_desc.Width;
	m_BufferView.StrideInBytes  = _pDesc->StrideInBytes;

	m_VertexCount = m_BufferView.SizeInBytes / m_BufferView.StrideInBytes;
}

const D3D12_VERTEX_BUFFER_VIEW& VertexBuffer::GetVertexView() const
{
	return m_BufferView;
}

UINT VertexBuffer::GetVertexCount() const
{
	return m_VertexCount;
}

ID3D12Resource* VertexBuffer::GetBufferResource()
{
	return m_pVertexData.Get();
}

ComPtr<ID3D12Resource> VertexBuffer::GetBufferResourceComPtr()
{
	return m_pVertexData;
}

const D3D12_SUBRESOURCE_DATA& VertexBuffer::GetBufferData() const
{
	return m_vbData; 
}

D3D12_RESOURCE_DESC& VertexBuffer::GetResourceDesc()
{
	return m_desc; 
}
