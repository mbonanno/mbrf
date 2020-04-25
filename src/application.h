#pragma once

#include "renderer_vk.h"

namespace MBRF
{
	class Application
	{
	public:
		void Run();

		void Init();
		void Cleanup();
		void Update();
		void Draw();

	private:
		RendererVK m_rendererVK;

		GLFWwindow* m_window;

		static const int s_MaxFramesInFlight = 2;
		int currentFrame;
	};

}


