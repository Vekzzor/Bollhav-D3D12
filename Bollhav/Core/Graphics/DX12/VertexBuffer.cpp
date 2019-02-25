#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(ID3D12Device4* _pDevice, VERTEX_BUFFER_DESC* _pDesc, CopyList* cpyList)
{
	Create(_pDevice, _pDesc, cpyList);
}

void VertexBuffer::Create(ID3D12Device4* _pDevice, VERTEX_BUFFER_DESC* _pDesc, CopyList* copyList)
{
	D3D12_HEAP_PROPERTIES heapProp;
	heapProp.Type				  = D3D12_HEAP_TYPE_DEFAULT;
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
	desc.Width				= _pDesc->SizeInBytes;
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

	//Just something to hold the data for transfer to the GPU.
	D3D12_SUBRESOURCE_DATA vbData;
	vbData.pData	  = m_pVertexData.Get();
	vbData.RowPitch   = sizeof(m_pVertexData);
	vbData.SlicePitch = vbData.RowPitch;

	//Create Upload Heap for copy. 
	copyList->CreateUploadHeap(_pDevice, vbData.RowPitch); 

	//Schedule copy
	UpdateSubresources<1>(copyList->GetList().Get(), 
		m_pVertexData.Get(),
		copyList->GetUploadHeap().Get(),
						  0,
						  0,
						  1,
						  &vbData);
	
	copyList->GetList().Get()->Close();  

	//Init view
	m_BufferView.BufferLocation = m_pVertexData->GetGPUVirtualAddress();
	m_BufferView.SizeInBytes	= desc.Width;
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

ID3D12Resource* VertexBuffer::GetVertexData() const
{
	return m_pVertexData.Get();
}
