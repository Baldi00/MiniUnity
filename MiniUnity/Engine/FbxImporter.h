#pragma once
#include "gl/glew.h"
#include <vector>
#include <string>

class FbxImporter
{
public:
    static bool ImportFBX(const char* filepath, std::vector<GLfloat>& outVertices, std::vector<GLuint>& outTriangles, std::string& diffuseTexturePath);
};

