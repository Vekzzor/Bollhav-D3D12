#include <Core/Render/Renderer.h>
#include <Core/Window/Window.h>

int main()
{
	Window window(VideoMode(), L"Hejsan");

	Renderer r;
	r.init(window.getSize().x, window.getSize().y, window.getHandle());

	while(window.isOpen())
	{
		window.pollEvents();
	}

	return 0;
}