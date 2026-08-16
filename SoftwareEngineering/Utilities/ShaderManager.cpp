#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "ShaderManager.h"



/***********************************************************
 *  LoadShaders()
 *
 *  This method is called to load the shader data from 
 *  external GLSL compatible files.
 ***********************************************************/
GLuint ShaderManager::LoadShaders(const char* vertex_file_path, const char* fragment_file_path)
{
    // Create shader objects
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // ------------------------------------------------------------
    // Read Vertex Shader
    // ------------------------------------------------------------
    std::string VertexShaderCode;
    {
        std::ifstream vsStream(vertex_file_path);
        if (!vsStream.is_open()) {
            std::cout << "ERROR: Cannot open vertex shader file: " << vertex_file_path << std::endl;
            return 0;
        }
        std::stringstream ss;
        ss << vsStream.rdbuf();
        VertexShaderCode = ss.str();
    }

    // ------------------------------------------------------------
    // Read Fragment Shader
    // ------------------------------------------------------------
    std::string FragmentShaderCode;
    {
        std::ifstream fsStream(fragment_file_path);
        if (!fsStream.is_open()) {
            std::cout << "ERROR: Cannot open fragment shader file: " << fragment_file_path << std::endl;
            return 0;
        }
        std::stringstream ss;
        ss << fsStream.rdbuf();
        FragmentShaderCode = ss.str();
    }

    GLint Result = GL_FALSE;
    GLint InfoLogLength = 0;

    // ------------------------------------------------------------
    // Compile Vertex Shader
    // ------------------------------------------------------------
    std::cout << "Compiling shader: " << vertex_file_path << std::endl;
    const char* vsrc = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &vsrc, nullptr);
    glCompileShader(VertexShaderID);

    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

    if (Result == GL_FALSE) {
        std::vector<char> error(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, nullptr, error.data());
        std::cout << "VERTEX SHADER ERROR:\n" << error.data() << std::endl;
        return 0;
    }

    // ------------------------------------------------------------
    // Compile Fragment Shader
    // ------------------------------------------------------------
    std::cout << "Compiling shader: " << fragment_file_path << std::endl;
    const char* fsrc = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &fsrc, nullptr);
    glCompileShader(FragmentShaderID);

    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

    if (Result == GL_FALSE) {
        std::vector<char> error(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, nullptr, error.data());
        std::cout << "FRAGMENT SHADER ERROR:\n" << error.data() << std::endl;
        return 0;
    }

    // ------------------------------------------------------------
    // Link Program
    // ------------------------------------------------------------
    std::cout << "Linking shader program..." << std::endl;

    GLuint ProgramID = glCreateProgram();
    m_programID = ProgramID;

    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);

    if (Result == GL_FALSE) {
        std::vector<char> error(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, nullptr, error.data());
        std::cout << "PROGRAM LINK ERROR:\n" << error.data() << std::endl;
        return 0;
    }

    std::cout << "Linked shader program ID: " << ProgramID << std::endl;

    // Cleanup
    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}
