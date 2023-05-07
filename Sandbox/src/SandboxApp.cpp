#include <Ume.h>
#include <Ume/Core/EntryPoint.h>

#include "SandboxLayer.h"

class UmeEditor : public Ume::Application
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

Ume::Application* Ume::CreateApplication()
{
	return new UmeEditor();
}
