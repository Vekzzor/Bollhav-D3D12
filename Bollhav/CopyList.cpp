#include "CopyList.h"
#include "pch.h"

CopyList::CopyList(ID3D12Device4* pDevice, ID3D12CommandAllocator* pCommandAlloc)
{
	//CopyList creation.
	TIF(pDevice->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_COPY, pCommandAlloc, nullptr, IID_PPV_ARGS(&m_pCopyList)));

	NAME_D3D12_OBJECT(m_pCopyList);
}

void CopyList::Prepare(ID3D12CommandAllocator* pAllocator, ID3D12Resource* vertexBuffer)
{
	//Create vertexbuffer resource and upload heap.
}

void CopyList::Finish() {}

ID3D12GraphicsCommandList* CopyList::operator->(void)
{
	return m_pCopyList.Get();
}

ID3D12GraphicsCommandList* CopyList::GetPtr()
{
	return m_pCopyList.Get();
}

HRESULT CopyList::CreateUploadHeap(ID3D12Device4* pDevice,
								   UINT dataSize,
								   ID3D12Resource1* UploadHeapResource)
{
	//I know TIF takes care of this but let me do it anyway. 

	HRESULT hrU;

	//Create Upload Heap
	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type					 = D3D12_HEAP_TYPE_UPLOAD;
	hp.CreationNodeMask		 = 1;
	hp.VisibleNodeMask		 = 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension		   = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width			   = dataSize; 
	rd.Height			   = 1;
	rd.DepthOrArraySize	= 1;
	rd.MipLevels		   = 1;
	rd.SampleDesc.Count	= 1;
	rd.Layout			   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	TIF(hrU = pDevice->CreateCommittedResource(&hp,
											   D3D12_HEAP_FLAG_NONE,
											   &rd,
											   D3D12_RESOURCE_STATE_GENERIC_READ,
											   nullptr,
											   IID_PPV_ARGS(&UploadHeapResource)));

	return hrU;
}

void CopyList::ScheduleCopy(VertexBuffer* vertexData)
{
	//Just something to hold the data for transfer to the GPU.
	D3D12_SUBRESOURCE_DATA vbData;
	vbData.pData	  = vertexData->GetVertexData();
	vbData.RowPitch   = vertexData->GetVertexView().SizeInBytes;
	vbData.SlicePitch = vbData.RowPitch;

	
}
