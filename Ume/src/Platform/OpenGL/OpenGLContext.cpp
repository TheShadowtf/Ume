#include <umepch.h>
#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Ume
{
	OpenGLContext::OpenGLContext(GLFWwindow* window) : m_WindowHandle(window)
	{
		UME_CORE_ASSERT(window, "Handle is null!")
	}

	void OpenGLContext::Init()
	{
		UME_PROFILE_FUNC();

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		UME_CORE_ASSERT(status, "Failed to initialize GLAD!");

		const GLubyte* vendor = glGetString(GL_VENDOR);
		const GLubyte* renderer = glGetString(GL_RENDERER);
		const GLubyte* version = glGetString(GL_VERSION);
		const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

		GLint major, minor;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);

		//printf("OpenGL Info:                      \n");
		//printf("GL Vendor            : %s\n", vendor);
		//printf("GL Renderer          : %s\n", renderer);
		//printf("GL Version (string)  : %s\n", version);
		//printf("GL Version (integer) : %d.%d\n", major, minor);
		//printf("GLSL Version         : %s\n", glslVersion);

		UME_CORE_INFO("OpenGL Informations");
		UME_CORE_INFO("   GL Vendor            : {0}",     (char*)vendor);
		UME_CORE_INFO("   GL Renderer          : {0}",     (char*)renderer);
		UME_CORE_INFO("   GL Version (string)  : {0}",     (char*)version);
		UME_CORE_INFO("   GL Version (integer) : {0} {1}", major, minor);
		UME_CORE_INFO("   GLSL Version         : {0}",     (char*)glslVersion);

#ifdef UME_ENABLE_ASSERTS
		int versionMajor;
		int versionMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

		UME_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5), "Ume requires at least OpenGL version 4.5!");
#endif
	}

	void OpenGLContext::SwapBuffers()
	{
		UME_PROFILE_FUNC();

		glfwSwapBuffers(m_WindowHandle);
	}
}