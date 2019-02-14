#include <Core/Input/Input.h>
#include <Core/Render/Renderer.h>
#include <Core/Window/Window.h>


int main()
{
	Window window(VideoMode(), L"Hejsan");

	Renderer r;
	r.init(window.getSize().x, window.getSize().y, window.getHandle());

	while(false == Input::IsKeyPressed(VK_ESCAPE) && window.isOpen())
	{
		window.pollEvents();
	}

	return 0;
}