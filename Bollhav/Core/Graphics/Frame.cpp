#include "Frame.h"

Frame::Frame(ID3D12Device4* _pDevice)
	: m_FenceValue(0)
{
	TIF(_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
										 IID_PPV_ARGS(&m_pCommandAllocator)));

	NAME_D3D12_OBJECT(m_pCommandAllocator);
}

ID3D12CommandAllocator* Frame::GetCommandAllocator() const
{
	return m_pCommandAllocator.Get();
}

UINT64 Frame::GetFenceValue() const
{
	return m_FenceValue;
}

void Frame::SetFenceValue(UINT64 _value)
{
	m_FenceValue = _value;
}
