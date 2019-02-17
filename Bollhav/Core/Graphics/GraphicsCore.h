#pragma once

class GraphicsCore
{
public:
	GraphicsCore();
	~GraphicsCore();

private:
	ComPtr<ID3D12Device4> m_pDevice;

private:
	void _init();
};
