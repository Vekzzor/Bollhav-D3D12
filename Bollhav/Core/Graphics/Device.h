#pragma once

class Device
{
public:
	Device();

	ID3D12Device4* GetDevice() const;

private:
	ComPtr<ID3D12Device4> m_pDevice;
};
