#pragma once

class Frame
{
public:
	Frame() = default;
	Frame(ID3D12Device4* _pDevice);

	ID3D12CommandAllocator* GetDirectAllocator() const;
	ID3D12CommandAllocator* GetComputeAllocator() const;

	UINT64 GetFenceValue() const;
	void SetFenceValue(UINT64 _value);

private:
	UINT64 m_FenceValue;
	ComPtr<ID3D12CommandAllocator> m_pDirectAllocator;
	ComPtr<ID3D12CommandAllocator> m_pComputeAllocator;
};
