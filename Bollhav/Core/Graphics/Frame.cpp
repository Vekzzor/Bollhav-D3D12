#include "Frame.h"

Frame::Frame(ID3D12Device4* _pDevice)
	: m_FenceValue(0)
{
	TIF(_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
										 IID_PPV_ARGS(&m_pDirectAllocator)));

	NAME_D3D12_OBJECT(m_pDirectAllocator);

	TIF(_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
										 IID_PPV_ARGS(&m_pComputeAllocator)));

	NAME_D3D12_OBJECT(m_pComputeAllocator);
}

ID3D12CommandAllocator* Frame::GetDirectAllocator() const
{
	return m_pDirectAllocator.Get();
}

ID3D12CommandAllocator* Frame::GetComputeAllocator() const
{
	return m_pComputeAllocator.Get();
}

UINT64 Frame::GetFenceValue() const
{
	return m_FenceValue;
}

void Frame::SetFenceValue(UINT64 _value)
{
	m_FenceValue = _value;
}
