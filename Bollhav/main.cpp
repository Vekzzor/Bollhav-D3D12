#include <Core/Input/Input.h>
#include <Core/Render/Renderer.h>
#include <Core/Window/Window.h>


int main(int argc, char** argv)
{
	Window window(VideoMode(), L"Hejsan");

	Renderer r;
	r._init(window.getSize().x, window.getSize().y, window.getHandle());

	while(false == Input::IsKeyPressed(VK_ESCAPE) && window.isOpen())
	{
		window.pollEvents();
	}

	return 0;
}