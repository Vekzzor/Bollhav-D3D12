#include "Device.h"
#include <pch.h>
Device::Device()
{

	ComPtr<ID3D12Debug> debugController;
	UINT dxgiFactoryFlags = 0;
	/*if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	ComPtr<ID3D12Debug1> debugcontroller1;
	debugController->QueryInterface(IID_PPV_ARGS(&debugcontroller1));
	debugcontroller1->SetEnableGPUBasedValidation(true);*/

	ComPtr<IDXGIFactory4> pFactory;
	ComPtr<IDXGIAdapter1> pAdapter;
	TIF(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pFactory)));
	//Loop through and find adapter
	for(UINT adapterIndex = 0;
		DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter);
		++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);
		printf("%ls\n", desc.Description);

		if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if(SUCCEEDED(
			   D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice))))
		{
			break;
		}
	}

	NAME_D3D12_OBJECT(m_pDevice);
}

ID3D12Device4* Device::GetDevice() const
{
	return m_pDevice.Get();
}

ID3D12Device4* Device::operator->(void)
{
	return m_pDevice.Get();
}
