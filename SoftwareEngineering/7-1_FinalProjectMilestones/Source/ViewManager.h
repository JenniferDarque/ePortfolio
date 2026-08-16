#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "ShaderManager.h"

class ViewManager
{
public:
    ViewManager(ShaderManager* pShaderManager);
    ~ViewManager();

    GLFWwindow* CreateDisplayWindow(const char* windowTitle);

private:
    ShaderManager* m_pShaderManager;
    GLFWwindow* m_pWindow;
};
