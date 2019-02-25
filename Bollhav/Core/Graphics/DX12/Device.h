#pragma once

class Device
{
public:
	Device();

	ID3D12Device4* GetDevice() const;
	ID3D12Device4* operator->(void);

private:
	ComPtr<ID3D12Device4> m_pDevice;
};
