#include <Ume.h>
#include <umepch.h>

class TestLayer : public Ume::Layer
{
public:
	TestLayer() : Layer("Test")
	{

	}

	void OnUpdate() override
	{
		if (Ume::Input::IsKeyPressed(Ume::Tab))
			UME_INFO("TestLayer::Update");
	}

	void OnEvent(Ume::Event& e) override
	{
		if (e.GetEventType() == Ume::EventType::KeyPressed)
		{
			Ume::KeyPressedEvent& event = (Ume::KeyPressedEvent&)e;
			UME_TRACE("{0}", (char)event.GetKeyCode());
		}
	}
};

class Sandbox : public Ume::Application
{
public:
	Sandbox()
	{
		PushLayer(new TestLayer());
		pushOverlay(new Ume::ImGuiLayer());
;	}

	~Sandbox()
	{

	}
};

Ume::Application* Ume::CreateApplication()
{
	return new Sandbox();
}