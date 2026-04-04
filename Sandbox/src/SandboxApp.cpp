#include <Ume.h>
#include <Ume/Core/EntryPoint.h>

#include "Sandbox2D.h"
#include "ExampleLayer.h"

class Sandbox : public Ume::Application
{
public:
	Sandbox(const Ume::ApplicationSpecification& specification)
		: Ume::Application(specification)
	{
		// PushLayer(new ExampleLayer());
		PushLayer(new Sandbox2D());
	}

	~Sandbox()
	{
	}
};

Ume::Application* Ume::CreateApplication(Ume::ApplicationCommandLineArgs args)
{
	ApplicationSpecification spec;
	spec.Name = "Sandbox";
	spec.WorkingDirectory = "../UmeEditor";
	spec.CommandLineArgs = args;

	return new Sandbox(spec);
}
