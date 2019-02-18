// dear imgui: standalone example application for DirectX 12
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// FIXME: 64-bit only for now! (Because sizeof(ImTextureId) == sizeof(void*))

#include <Core/Graphics/Device.h>
#include <Core/Graphics/FrameManager.h>
#include <Core/Graphics/GraphicsCommandQueue.h>
#include <Core/Graphics/Swapchain.h>
#include <Core/Window/Window.h>

int main(int, char**)
{
	Window window(VideoMode(1280, 720), L"Hejsan");

	Device device;
	Swapchain sc(device.GetDevice());
	GraphicsCommandQueue CommandQueue(device.GetDevice());
	sc.Init(device.GetDevice(), CommandQueue.GetCommandQueue());

	FrameManager fm(device.GetDevice());

	//// Initialize Direct3D
	//if(CreateDeviceD3D(window.getHandle()) < 0)
	//{

	//	return 1;
	//}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	ComPtr<ID3D12DescriptorHeap> g_imguiSRVHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors				= 1;
	desc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	TIF(device.GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_imguiSRVHeap)));

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(window.getHandle());
	ImGui_ImplDX12_Init(device.GetDevice(),
						NUM_BACKBUFFERS,
						DXGI_FORMAT_R8G8B8A8_UNORM,
						g_imguiSRVHeap->GetCPUDescriptorHandleForHeapStart(),
						g_imguiSRVHeap->GetGPUDescriptorHandleForHeapStart());


	bool show_demo_window	= true;
	bool show_another_window = false;
	ImVec4 clear_color		 = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while(window.isOpen())
	{

		window.pollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if(show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f	 = 0.0f;
			static int counter = 0;

			ImGui::Begin(
				"Hello, world!"); // Create a window called "Hello, world!" and append into it.

			ImGui::Text(
				"This is some useful text."); // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window",
							&show_demo_window); // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat(
				"float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color",
							  (float*)&clear_color); // Edit 3 floats representing a color

			if(ImGui::Button(
				   "Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
						1000.0f / ImGui::GetIO().Framerate,
						ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if(show_another_window)
		{
			ImGui::Begin(
				"Another Window",
				&show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if(ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		// Rendering
		Frame* frameCtxt = fm.GetReadyFrame(&sc);
		frameCtxt->GetCommandAllocator()->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = sc.GetCurrentRenderTarget();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

		fm.m_pCommandList->Reset(frameCtxt->GetCommandAllocator(), NULL);
		fm.m_pCommandList->ResourceBarrier(1, &barrier);
		fm.m_pCommandList->ClearRenderTargetView(
			sc.GetCurrentDescriptor(), (float*)&clear_color, 0, NULL);
		fm.m_pCommandList->OMSetRenderTargets(1, &sc.GetCurrentDescriptor(), FALSE, NULL);
		ID3D12DescriptorHeap* ppHeaps[] = {g_imguiSRVHeap.Get()};
		fm.m_pCommandList->SetDescriptorHeaps(1, ppHeaps);
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), fm.m_pCommandList.Get());
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
		fm.m_pCommandList->ResourceBarrier(1, &barrier);
		fm.m_pCommandList->Close();

		ID3D12CommandList* listsToExecute[] = {fm.m_pCommandList.Get()};
		CommandQueue.GetCommandQueue()->ExecuteCommandLists(_countof(listsToExecute),
															listsToExecute);

		sc.Present();

		fm.SyncCommandQueue(frameCtxt, CommandQueue.GetCommandQueue());
	}


	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}