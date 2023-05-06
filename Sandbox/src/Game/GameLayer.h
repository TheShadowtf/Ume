#include <Ume.h>
#include <imgui/imgui.h>

class GameLayer : public Ume::Layer
{
public:
	GameLayer();
	~GameLayer() = default;
	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnImGuiRender() override;
	void OnUpdate(Ume::Timestep ts) override;
	void OnEvent(Ume::Event& e) override;
	void CreateCamera(uint32_t width, uint32_t height);
	bool OnMouseButtonPressed(Ume::MouseButtonEvent& e);
	bool OnWindowResize(Ume::WindowResizeEvent& e);
private:
	Ume::Scope<Ume::OrthographicCamera> m_Camera;
	float m_Time = 0.0f;
	bool m_Blink = false;
	enum class GameState
	{
		Play = 0, MainMenu = 1, GameOver = 2
	};
	GameState m_State = GameState::MainMenu;
};
