#include "umepch.h"
#include "Ume/Renderer/RenderCommand.h"

namespace Ume {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}