#include "DepthStencil.h"
#include "pch.h"

DepthStencil::DepthStencil(ID3D12Device4* _pDevice, UINT64 _Width, UINT64 _Height)
	: m_fDepthValue(1.0f)
{
	// Create Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors			   = 1;
	dsvHeapDesc.Type					   = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags					   = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	TIF(_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDSVHeap)));
	NAME_D3D12_OBJECT(m_pDSVHeap);

	// Resource
	D3D12_HEAP_PROPERTIES hp;
	hp.Type					= D3D12_HEAP_TYPE_DEFAULT;
	hp.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask		= 1;
	hp.VisibleNodeMask		= 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension		   = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rd.Alignment		   = 0;
	rd.Width			   = _Width;
	rd.Height			   = _Height;
	rd.DepthOrArraySize	= 1;
	rd.MipLevels		   = 0;
	rd.SampleDesc.Count	= 1;
	rd.SampleDesc.Quality  = 0;
	rd.Flags			   = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	rd.Layout			   = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rd.Format			   = DXGI_FORMAT_D32_FLOAT;

	D3D12_CLEAR_VALUE depthOp	= {};
	depthOp.Format				 = DXGI_FORMAT_D32_FLOAT;
	depthOp.DepthStencil.Depth   = m_fDepthValue;
	depthOp.DepthStencil.Stencil = 0;

	TIF(_pDevice->CreateCommittedResource(&hp,
										  D3D12_HEAP_FLAG_NONE,
										  &rd,
										  D3D12_RESOURCE_STATE_DEPTH_WRITE,
										  &depthOp,
										  IID_PPV_ARGS(&m_pDepthResource)));

	// Depth Stencil View
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format						   = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension				   = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags						   = D3D12_DSV_FLAG_NONE;

	_pDevice->CreateDepthStencilView(m_pDepthResource.Get(),
									 &depthStencilDesc,
									 m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthStencil::GetCPUDescriptorHandleForHeapStart()
{
	return m_pDSVHeap->GetCPUDescriptorHandleForHeapStart();
}

void DepthStencil::Clear(ID3D12GraphicsCommandList* _pCmdList, D3D12_CLEAR_FLAGS _Flags)
{
	_pCmdList->ClearDepthStencilView(
		m_pDSVHeap->GetCPUDescriptorHandleForHeapStart(), _Flags, m_fDepthValue, 0, 0, nullptr);
}
