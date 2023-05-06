#include <Ume.h>

#include "GameLayer.h"

class Game : public Ume::Application
{
public:
	Game()
	{
		PushLayer(new GameLayer());
	}
	~Game()
	{

	}
};

//Ume::Application* Ume::CreateApplication()
//{
//	return new Game();
//}