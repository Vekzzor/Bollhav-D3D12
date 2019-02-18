#include <Core/Graphics/FrameManager.h>
#include <Core/Graphics/GraphicsCore.h>
#include <Core/Input/Input.h>
#include <Core/Render/Renderer.h>
#include <Core/Window/Window.h>

ComPtr<ID3D12DescriptorHeap> g_imguiSRVHeap;

int main(int argc, char** argv)
{
	Window window(VideoMode(), L"Hejsan");

	Device Device;
	Swapchain sc(Device.GetDevice());
	GraphicsCommandQueue CommandQueue(Device.GetDevice());
	sc.Init(Device.GetDevice(), CommandQueue.GetCommandQueue());

	FrameManager fm(Device.GetDevice());

	if(false == IMGUI_CHECKVERSION())
		std::cerr << "Failed to init imgui\n";
	if(nullptr == ImGui::CreateContext())
		std::cerr << "Failed to init imgui\n";

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors				= 1;
	desc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	TIF(Device.GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_imguiSRVHeap)));

	if(false == ImGui_ImplWin32_Init(GetActiveWindow()))
	{
		std::cerr << "Failed to init win32 imgui\n";
	}
	if(false == ImGui_ImplDX12_Init(Device.GetDevice(),
									NUM_BACKBUFFERS,
									DXGI_FORMAT_R8G8B8A8_UNORM,
									g_imguiSRVHeap->GetCPUDescriptorHandleForHeapStart(),
									g_imguiSRVHeap->GetGPUDescriptorHandleForHeapStart()))
	{
		std::cerr << "Failed to init DX12 imgui\n";
	}
	bool lol = true;
	while(false == Input::IsKeyPressed(VK_ESCAPE) && window.isOpen())
	{
		window.pollEvents();

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow(&lol);

		Frame* frame = fm.GetReadyFrame(&sc);

		TIF(frame->GetCommandAllocator()->Reset());
		
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = sc.GetCurrentRenderTarget();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

		FLOAT clearC[] = {0, 0, 0, 0};

		TIF(fm.m_pCommandList->Reset(frame->GetCommandAllocator(), NULL));
		fm.m_pCommandList->ResourceBarrier(1, &barrier);
		fm.m_pCommandList->OMSetRenderTargets(1, &sc.GetCurrentDescriptor(), FALSE, NULL);
		fm.m_pCommandList->ClearRenderTargetView(sc.GetCurrentDescriptor(), clearC, 0, NULL);

		ID3D12DescriptorHeap* ppHeaps[] = {g_imguiSRVHeap.Get()};
		fm.m_pCommandList->SetDescriptorHeaps(1, ppHeaps);

		ImGui::Render();
		//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), fm.m_pCommandList.Get());

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
		fm.m_pCommandList->ResourceBarrier(1, &barrier);

		TIF(fm.m_pCommandList->Close());

		ID3D12CommandList* listsToExecute[] = {fm.m_pCommandList.Get()};
		CommandQueue.GetCommandQueue()->ExecuteCommandLists(_countof(listsToExecute),
															listsToExecute);

		sc.Present();

		fm.SyncCommandQueue(frame, CommandQueue.GetCommandQueue());


	}

	return 0;
}