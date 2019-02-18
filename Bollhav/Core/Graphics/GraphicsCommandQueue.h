#pragma once

class GraphicsCommandQueue
{
public:
	GraphicsCommandQueue(ID3D12Device4 * _device);

	ID3D12CommandQueue* GetCommandQueue() const;
private:
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
};
