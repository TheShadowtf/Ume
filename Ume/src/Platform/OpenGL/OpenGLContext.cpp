#include "umepch.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Ume {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		UME_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		UME_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		UME_CORE_ASSERT(status, "Failed to initialize Glad!");

		UME_CORE_INFO("OpenGL Info:");
		UME_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		UME_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		UME_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));

		UME_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Ume requires at least OpenGL version 4.5!");
	}

	void OpenGLContext::SwapBuffers()
	{
		UME_PROFILE_FUNCTION();

		glfwSwapBuffers(m_WindowHandle);
	}

}
