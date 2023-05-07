#include <Ume.h>
#include <Ume/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Ume
{
	class UmeEditor : public Application
	{
	public:
		UmeEditor() : Application("Ume Editor")
		{
			PushLayer(new EditorLayer());
		}
		~UmeEditor()
		{

		}
	};

	Application* CreateApplication()
	{
		return new UmeEditor();
	}
}