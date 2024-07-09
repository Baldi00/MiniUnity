
////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

/// GLEW is needed to provide OpenGL extensions.
#include <GL/glew.h>

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

/// GLM is needed to provide 3D math properties, particularly matrices for 3D transformations.
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp" 
#include "glm/gtx/transform.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Engine/FbxImporter.h"

#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif

///Shader Types
enum class ShaderType { Vertex, Fragment, Geometry, Count };
///Standard Uniforms in the shader.
enum class UniformType { TransformPVM, Count };
///Vertex attributes for shaders and the input vertex array.
enum class VertexAttribute { Position, Normal, TexCoord, Count };

///Shader Program
GLuint program = 0;

///List of shaders set up for a 3D scene.
GLuint shader[static_cast<unsigned int>(ShaderType::Count)];
///List of uniforms that can be defined values for the shader.
GLint uniform[static_cast<unsigned int>(UniformType::Count)];

///Vertex Array Object ID.
GLuint vao = 0;
///Vertex Buffer Object for Vertices.
GLuint vertexVBO = 0;
///Vertex Buffer Object for Indices.
GLuint indexVBO = 0;
///Depending on input, the amount of vertices or indices that are needed to be drawn for this object.
unsigned int drawCount;

///Checks for any errors specific to the shaders. It will output any errors within the shader if it's not valid.
void checkError(GLuint l_shader, GLuint l_flag, bool l_program, const std::string& l_errorMsg)
{
    GLint success = 0;
    GLchar error[1024] = { 0 };
    if (l_program) { glGetProgramiv(l_shader, l_flag, &success); }
    else { glGetShaderiv(l_shader, l_flag, &success); }

    if (success) { return; }
    if (l_program) {
        glGetProgramInfoLog(l_shader, sizeof(error), nullptr, error);
    }
    else {
        glGetShaderInfoLog(l_shader, sizeof(error), nullptr, error);
    }

    std::cout << l_errorMsg << "\n";
}

///Creates and compiles a shader.
GLuint buildShader(const std::string& l_src, unsigned int l_type)
{
    GLuint shaderID = glCreateShader(l_type);
    if (!shaderID) {
        std::cout << "Bad shader type!";
        return 0;
    }
    const GLchar* sources[1];
    GLint lengths[1];
    sources[0] = l_src.c_str();
    lengths[0] = l_src.length();
    glShaderSource(shaderID, 1, sources, lengths);
    glCompileShader(shaderID);
    checkError(shaderID, GL_COMPILE_STATUS, false, "Shader compile error: ");
    return shaderID;
}

///Function to load the shaders from string data.
void LoadFromMemory(const std::string& shaderData, ShaderType type)
{
    if (shaderData.empty())
        return;

    if (shader[static_cast<unsigned int>(type)])
    {
        glDetachShader(program, shader[static_cast<unsigned int>(type)]);
        glDeleteShader(shader[static_cast<unsigned int>(type)]);
    }

    switch (type)
    {
    case ShaderType::Vertex:
        shader[static_cast<unsigned int>(type)] = buildShader(shaderData, GL_VERTEX_SHADER);
        break;
    case ShaderType::Geometry:
        shader[static_cast<unsigned int>(type)] = buildShader(shaderData, GL_GEOMETRY_SHADER);
        break;
    case ShaderType::Fragment:
        shader[static_cast<unsigned int>(type)] = buildShader(shaderData, GL_FRAGMENT_SHADER);
        break;
    default:
        break;
    }

    if (program == 0)
    {
        program = glCreateProgram();
    }

    glAttachShader(program, shader[static_cast<unsigned int>(type)]);
    glBindAttribLocation(program, static_cast<GLuint>(VertexAttribute::Position), "position");
    glBindAttribLocation(program, static_cast<GLuint>(VertexAttribute::Normal), "normal");
    glBindAttribLocation(program, static_cast<GLuint>(VertexAttribute::TexCoord), "texCoord");

    glLinkProgram(program);
    checkError(program, GL_LINK_STATUS, true, "Shader link error:");
    glValidateProgram(program);
    checkError(program, GL_VALIDATE_STATUS, true, "Invalid shader:");

    uniform[static_cast<unsigned int>(UniformType::TransformPVM)] = glGetUniformLocation(program, "pvm");
}

std::string loadShaderFromFile(const std::string& filePath)
{
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open())
    {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return "";
    }

    std::stringstream shaderData;
    shaderData << shaderFile.rdbuf();
    shaderFile.close();

    return shaderData.str();
}

////////////////////////////////////////////////////////////
/// Entry point of application
///
/// \return Application exit code
///
////////////////////////////////////////////////////////////
int main()
{
    bool exit = false;
    bool sRgb = false;

    while (!exit)
    {
        // Request a 24-bits depth buffer when creating the window
        sf::ContextSettings contextSettings;
        contextSettings.depthBits = 24;
        contextSettings.sRgbCapable = sRgb;

        // Create the main window
        sf::RenderWindow window(sf::VideoMode(1920, 1080), "SFML graphics with OpenGL", sf::Style::Default, contextSettings);
        window.setVerticalSyncEnabled(true);

        // Initialise GLEW for the extended functions.
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
            return EXIT_FAILURE;

        // Create a sprite for the background
        sf::Texture backgroundTexture;
        backgroundTexture.setSrgb(sRgb);
        if (!backgroundTexture.loadFromFile("resources/background.jpg"))
            return EXIT_FAILURE;
        sf::Sprite background(backgroundTexture);

        // Create some text to draw on top of our OpenGL object
        sf::Font font;
        if (!font.loadFromFile("resources/hey_comic.ttf"))
            return EXIT_FAILURE;
        sf::Text text("SFML / OpenGL demo", font);
        sf::Text sRgbInstructions("Press space to toggle sRGB conversion", font);
        sf::Text mipmapInstructions("Press return to toggle mipmapping", font);
        text.setFillColor(sf::Color(255, 255, 255, 170));
        sRgbInstructions.setFillColor(sf::Color(255, 255, 255, 170));
        mipmapInstructions.setFillColor(sf::Color(255, 255, 255, 170));
        text.setPosition(250.f, 450.f);
        sRgbInstructions.setPosition(150.f, 500.f);
        mipmapInstructions.setPosition(180.f, 550.f);

        // Load a texture to apply to our 3D cube
        sf::Texture texture;
        if (!texture.loadFromFile("resources/texture.jpg"))
            return EXIT_FAILURE;

        // Attempt to generate a mipmap for our cube texture
        // We don't check the return value here since
        // mipmapping is purely optional in this example
        texture.generateMipmap();

        // Make the window the active window for OpenGL calls
        window.setActive(true);

        // Load the shaders we need.
        if (program == 0)
        {
            LoadFromMemory(loadShaderFromFile("resources/vertex_shader.glsl"), ShaderType::Vertex);
            LoadFromMemory(loadShaderFromFile("resources/fragment_shader.glsl"), ShaderType::Fragment);
        }

        // Enable Z-buffer read and write and culling.
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Setup a perspective projection
        GLfloat ratio = static_cast<float>(window.getSize().x) / window.getSize().y;
        glm::mat4 projection = glm::frustum(-ratio, ratio, -1.f, 1.f, 1.f, 1000.0f);

        std::vector<GLfloat> objectVertices;
        std::vector<GLuint> objectTriangles;

        FbxImporter::ImportFBX("resources/Kleo.fbx", objectVertices, objectTriangles, drawCount);

        // Stride is the number of bytes per array element.
        auto stride = sizeof(GLfloat) * 8;
        // Data offset for normals in bytes.
        auto normalOffset = sizeof(GLfloat) * 3;
        // Data offset for texture coordinate in bytes.
        auto textureCoordOffset = sizeof(GLfloat) * 6;


        // Generate Vertex Array and Vertex Buffer and point the area of data to assign to each attribute.
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vertexVBO);
        glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
        glBufferData(GL_ARRAY_BUFFER, drawCount * stride, objectVertices.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttribute::Position));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttribute::Position), 3, GL_FLOAT, GL_FALSE, stride, 0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttribute::Normal));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttribute::Normal), 3, GL_FLOAT, GL_FALSE, stride, (void*)normalOffset);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttribute::TexCoord));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttribute::TexCoord), 2, GL_FLOAT, GL_FALSE, stride, (void*)textureCoordOffset);

        // Generate Index Buffer and define the amount of indices to point to.
        glGenBuffers(1, &indexVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, drawCount * sizeof(objectTriangles[0]), objectTriangles.data(), GL_DYNAMIC_DRAW);

        //Make sure to bind the vertex array to null if you wish to define more objects.
        glBindVertexArray(0);

        // Make the window no longer the active window for OpenGL calls
        window.setActive(false);

        // Create a clock for measuring the time elapsed
        sf::Clock clock;

        // Flag to track whether mipmapping is currently enabled
        bool mipmapEnabled = true;

        // Start game loop
        while (window.isOpen())
        {
            // Process events
            sf::Event event;
            while (window.pollEvent(event))
            {
                // Close window: exit
                if (event.type == sf::Event::Closed)
                {
                    exit = true;
                    window.close();
                }

                // Escape key: exit
                if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))
                {
                    exit = true;
                    window.close();
                }

                // Return key: toggle mipmapping
                if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Return))
                {
                    if (mipmapEnabled)
                    {
                        // We simply reload the texture to disable mipmapping
                        if (!texture.loadFromFile("resources/texture.jpg"))
                            return EXIT_FAILURE;

                        mipmapEnabled = false;
                    }
                    else
                    {
                        texture.generateMipmap();

                        mipmapEnabled = true;
                    }
                }

                // Space key: toggle sRGB conversion
                if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Space))
                {
                    sRgb = !sRgb;
                    window.close();
                }

                // Adjust the viewport when the window is resized
                if (event.type == sf::Event::Resized)
                {
                    // Make the window the active window for OpenGL calls
                    window.setActive(true);

                    glViewport(0, 0, event.size.width, event.size.height);

                    // Make the window no longer the active window for OpenGL calls
                    window.setActive(false);
                }
            }

            // Clear the depth buffer
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Configure the viewport (the same size as the window)
            glViewport(0, 0, window.getSize().x, window.getSize().y);

            // Draw the background
            window.pushGLStates();
            //window.draw(background);
            window.popGLStates();

            // Make the window the active window for OpenGL calls
            window.setActive(true);

            // Bind the texture
            sf::Texture::bind(&texture);
            glBindVertexArray(vao);

            // We get the position of the mouse cursor, so that we can move the box accordingly
            float x = sf::Mouse::getPosition(window).x * 200.f / window.getSize().x - 100.f;
            float y = -sf::Mouse::getPosition(window).y * 200.f / window.getSize().y + 100.f;

            // Apply some transformations
            glm::mat4 matrix_pos = glm::translate(glm::vec3(x, y, -200.f));
            glm::mat4 matrix_rotX = glm::rotate(clock.getElapsedTime().asSeconds() * 0.f * (3.1415926f / 180.0f), glm::vec3(1.f, 0.f, 0.f));
            glm::mat4 matrix_rotY = glm::rotate(clock.getElapsedTime().asSeconds() * 30.f * (3.1415926f / 180.0f), glm::vec3(0.f, 1.f, 0.f));
            glm::mat4 matrix_rotZ = glm::rotate(clock.getElapsedTime().asSeconds() * 0.f * (3.1415926f / 180.0f), glm::vec3(0.f, 0.f, 1.f));

            glm::mat4 matrix_rotation = matrix_rotZ * matrix_rotY * matrix_rotX;
            glm::mat4 transform = matrix_pos * matrix_rotation;

            glm::mat4 identity;
            glm::mat4 viewProj = projection * transform;

            //Bind the shaders.
            glUseProgram(program);

            //Set the uniforms for the shader to use.
            if (uniform[(int)UniformType::TransformPVM] >= 0)
                glUniformMatrix4fv((unsigned int)uniform[(int)UniformType::TransformPVM], 1, GL_FALSE, &viewProj[0][0]);

            // Draw the cube
            glDrawElements(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, 0);

            // Reset the vertex array bound, shader and texture for other assets to draw.
            glBindVertexArray(0);
            glUseProgram(0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Make the window no longer the active window for OpenGL calls
            window.setActive(false);

            // Draw some text on top of our OpenGL object
            window.pushGLStates();
            //window.draw(text);
            //window.draw(sRgbInstructions);
            //window.draw(mipmapInstructions);
            window.popGLStates();

            // Finally, display the rendered frame on screen
            window.display();
        }

        //Destroy all buffers, shaders and programs.
        glDeleteBuffers(1, &vertexVBO);
        glDeleteBuffers(1, &indexVBO);
        glDeleteVertexArrays(1, &vao);

        //Setting these values to zero will allow them to be initialised with new data on reset.
        vertexVBO = 0;
        indexVBO = 0;
        vao = 0;

        for (unsigned int i = 0; i < static_cast<unsigned int>(ShaderType::Count); i++)
        {
            glDetachShader(program, shader[i]);
            glDeleteShader(shader[i]);
            shader[i] = 0;
        }

        for (unsigned int i = 0; i < static_cast<unsigned int>(UniformType::Count); i++)
        {
            uniform[i] = -1;
        }

        glDeleteProgram(program);
        program = 0;
    }

    return EXIT_SUCCESS;
}