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
		//UME_INFO("TestLayer::Update");
	}

	void OnEvent(Ume::Event& e) override
	{
		if (e.IsInCategory(Ume::EventCategoryApplication))
			UME_TRACE("{0}", 1);
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