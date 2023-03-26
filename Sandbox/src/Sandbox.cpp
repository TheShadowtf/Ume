#include <Ume.h>

class TestLayer : public Ume::Layer
{
public:
	TestLayer() : Layer("Test")
	{

	}

	void OnUpdate() override
	{
		UME_INFO("TestLayer::Update");
	}

	void OnEvent(Ume::Event& event) override
	{
		UME_TRACE("{0}", event);
	}
};

class Sandbox : public Ume::Application
{
public:
	Sandbox()
	{
		PushLayer(new TestLayer());
;	}

	~Sandbox()
	{

	}
};

Ume::Application* Ume::CreateApplication()
{
	return new Sandbox();
}