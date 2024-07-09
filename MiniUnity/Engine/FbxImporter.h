#pragma once
#include "gl/glew.h"
#include <vector>

class FbxImporter
{
public:
    static bool ImportFBX(const char* filepath, std::vector<GLfloat>& outVertices, std::vector<GLuint>& outTriangles, unsigned int& outTrianglesCount);
};

