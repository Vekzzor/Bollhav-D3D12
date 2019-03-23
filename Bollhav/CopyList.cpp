#include "CopyList.h"
#include "pch.h"

CopyList::CopyList(ID3D12Device4* pDevice)
{
	//Create command allocator
	pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,
									IID_PPV_ARGS(&m_commandAllocator));
	NAME_D3D12_OBJECT(m_commandAllocator);
	//CopyList creation.
	TIF(pDevice->CreateCommandList(0,
								   D3D12_COMMAND_LIST_TYPE_COPY,
								   m_commandAllocator.Get(),
								   nullptr,
								   IID_PPV_ARGS(&m_pCopyList)));

	//m_pCopyList->Close();

	NAME_D3D12_OBJECT(m_pCopyList);
}

void CopyList::Prepare(ID3D12Resource* vertexBuffer) {}

void CopyList::Finish(ID3D12Resource* vertexData)
{
	//Change the barrier
	m_pCopyList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(vertexData,
											  D3D12_RESOURCE_STATE_COPY_DEST,
											  D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
											  D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
											  D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY));
	ResetCopyAllocator();
	ResetCopyList();
}

ID3D12GraphicsCommandList* CopyList::operator->(void)
{
	return m_pCopyList.Get();
}

ID3D12GraphicsCommandList* CopyList::GetPtr()
{
	return m_pCopyList.Get();
}

ComPtr<ID3D12GraphicsCommandList> CopyList::GetList()
{
	return m_pCopyList;
}

ComPtr<ID3D12Resource> CopyList::GetUploadHeap()
{
	return m_pUploadHeapResource;
}

HRESULT CopyList::CreateUploadHeap(ID3D12Device4* pDevice, UINT dataSize)
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
											   IID_PPV_ARGS(&m_pUploadHeapResource)));

	return hrU;
}

void CopyList::ScheduleCopy(ID3D12Resource* copyDest,
							ID3D12Resource* uploadHeapRes,
							D3D12_SUBRESOURCE_DATA copyData,
							UINT offset)
{
	UpdateSubresources<1>(m_pCopyList.Get(), copyDest, uploadHeapRes, offset, 0, 1, &copyData);
}
void CopyList::ExecuteCopy(ID3D12CommandQueue* commandQueue)
{
	executeList.push_back(m_pCopyList.Get());
	commandQueue->ExecuteCommandLists(static_cast<UINT>(executeList.size()), executeList.data());

	executeList.clear();

	//Make sure to wait for GPU after this function call.
}

void CopyList::ResetCopyAllocator()
{
	TIF(m_commandAllocator->Reset());
}

void CopyList::ResetCopyList()
{
	TIF(m_pCopyList->Reset(m_commandAllocator.Get(), nullptr));
}
