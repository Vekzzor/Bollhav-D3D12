#pragma once

class ConstantBuffer
{
public:
	ConstantBuffer(ID3D12Device4* _pDevice);
private:
	ComPtr<ID3D12Resource> m_pResource;
};