#include <umepch.h>
#include "RendererCommand.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Ume
{
	RendererAPI* RendererCommand::s_RendererAPI = new OpenGLRendererAPI;
}