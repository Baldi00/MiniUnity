#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

int main()
{
    sf::Shader shader;

    // load only the vertex shader
    //if (!shader.loadFromMemory(vertexShader, sf::Shader::Vertex))
    //{
    //    // error...
    //}

    //// load only the fragment shader
    //if (!shader.loadFromMemory(fragmentShader, sf::Shader::Fragment))
    //{
    //    // error...
    //}

    // load both shaders
    if (!shader.loadFromFile("vertex_shader.vert", "fragment_shader.frag"))
    {
        // error...
        return 0;
    }

    // create the window
    sf::Window window(sf::VideoMode(1920, 1080), "OpenGL", sf::Style::Default, sf::ContextSettings(32));
    window.setVerticalSyncEnabled(true);

    // activate the window
    window.setActive(true);

    // load resources, initialize the OpenGL states, ...

    // clear the buffers
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glColor3f(1.0, 1.0, 1.0);

    // Tell the rendering engine not to draw backfaces.  Without this code,
    // all four faces of the tetrahedron would be drawn and it is possible
    // that faces farther away could be drawn after nearer to the viewer.
    // Since there is only one closed polyhedron in the whole scene,
    // eliminating the drawing of backfaces gives us the realism we need.
    // THIS DOES NOT WORK IN GENERAL.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Set the camera lens so that we have a perspective viewing volume whose
    // horizontal bounds at the near clipping plane are -2..2 and vertical
    // bounds are -1.5..1.5.  The near clipping plane is 1 unit from the camera
    // and the far clipping plane is 40 units away.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-2, 2, -1.125, 1.125, 1, 100);

    // Set up transforms so that the tetrahedron which is defined right at
    // the origin will be rotated and moved into the view volume.  First we
    // rotate 70 degrees around y so we can see a lot of the left side.
    // Then we rotate 50 degrees around x to "drop" the top of the pyramid
    // down a bit.  Then we move the object back 3 units "into the screen".
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -5);
    glRotatef(30, 1, 0, 0);
    glRotatef(-30, 0, 1, 0);

    // run the main loop
    bool running = true;

    sf::Clock clock;

    while (running)
    {
        float deltaTime = clock.restart().asSeconds();
        // handle events
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                // end the program
                running = false;
            }
            else if (event.type == sf::Event::Resized)
            {
                // adjust the viewport when the window is resized
                glViewport(0, 0, event.size.width, event.size.height);
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glRotatef(-(50 * deltaTime), 0, 1, 0);

        // Draw a white grid "floor" for the tetrahedron to sit on.
        glBegin(GL_LINES);

        glColor3f(.3, .3, .3);
        for (int i = -10; i < 10; i += 1)
        {
            glVertex3f(i, 0, -10); glVertex3f(i, 0, 10);
            glVertex3f(-10, 0, i); glVertex3f(10, 0, i);
        }

        glColor3f(1, 0, 0);
        glVertex3f(0, 0, 0); glVertex3f(100, 0, 0);
        glColor3f(0, 1, 0);
        glVertex3f(0, 0, 0); glVertex3f(0, 100, 0);
        glColor3f(0, 0, 1);
        glVertex3f(0, 0, 0); glVertex3f(0, 0, 100);

        glEnd();

        // Draw the tetrahedron.  It is a four sided figure, so when defining it
        // with a triangle strip we have to repeat the last two vertices.

        sf::Shader::bind(&shader);

        // draw your OpenGL entity here...

        glBegin(GL_QUADS);
        glColor3f(1, 1, 1);

        // Front face
        glNormal3f(0, 0, 1);
        glVertex3f(-1, 0, 1);
        glNormal3f(0, 0, 1);
        glVertex3f(1, 0, 1);
        glNormal3f(0, 0, 1);
        glVertex3f(1, 2, 1);
        glNormal3f(0, 0, 1);
        glVertex3f(-1, 2, 1);
        // Back face
        glNormal3f(0, 1, 1);
        glVertex3f(-1, 2, -1);
        glNormal3f(0, 1, 1);
        glVertex3f(1, 2, -1);
        glNormal3f(0, 1, 1);
        glVertex3f(1, 0, -1);
        glNormal3f(0, 1, 1);
        glVertex3f(-1, 0, -1);
        // Left face
        glNormal3f(1, 1, 0);
        glVertex3f(-1, 0, 1);
        glNormal3f(1, 1, 0);
        glVertex3f(-1, 2, 1);
        glNormal3f(1, 1, 0);
        glVertex3f(-1, 2, -1);
        glNormal3f(1, 1, 0);
        glVertex3f(-1, 0, -1);
        // Right face
        glNormal3f(1, 0, 0);
        glVertex3f(1, 0, -1);
        glNormal3f(1, 0, 0);
        glVertex3f(1, 2, -1);
        glNormal3f(1, 0, 0);
        glVertex3f(1, 2, 1);
        glNormal3f(1, 0, 0);
        glVertex3f(1, 0, 1);
        // Up face
        glNormal3f(0, 1, 0);
        glVertex3f(-1, 2, 1);
        glNormal3f(0, 1, 0);
        glVertex3f(1, 2, 1);
        glNormal3f(0, 1, 0);
        glVertex3f(1, 2, -1);
        glNormal3f(0, 1, 0);
        glVertex3f(-1, 2, -1);
        // Down face
        glNormal3f(1, 1, 1);
        glVertex3f(-1, 0, -1);
        glNormal3f(1, 1, 1);
        glVertex3f(1, 0, -1);
        glNormal3f(1, 1, 1);
        glVertex3f(1, 0, 1);
        glNormal3f(1, 1, 1);
        glVertex3f(-1, 0, 1);

        glEnd();

        // bind no shader
        sf::Shader::bind(NULL);

        glFlush();

        // end the current frame (internally swaps the front and back buffers)
        window.display();
    }

    // release resources...

    return 0;
}