#include "Angel.h"

typedef vec4 color4;
typedef vec4 point4;

const int NumVertices = 36;

point4 points[NumVertices];
color4 colors[NumVertices];

enum BALL_SHAPE
{
    SPHERE,
    CUBE,
    BUNNY,
    NUM_SHAPES
};

enum DRAWING_MODE
{
    SOLID,
    WIREFRAME
};

point4 vertices[8] = {
    point4(-0.5, -0.5, 0.5, 1.0),
    point4(-0.5, 0.5, 0.5, 1.0),
    point4(0.5, 0.5, 0.5, 1.0),
    point4(0.5, -0.5, 0.5, 1.0),
    point4(-0.5, -0.5, -0.5, 1.0),
    point4(-0.5, 0.5, -0.5, 1.0),
    point4(0.5, 0.5, -0.5, 1.0),
    point4(0.5, -0.5, -0.5, 1.0),
};

color4 vertex_colors[8] = {
    color4(0.0, 0.0, 0.0, 1.0), // black
    color4(1.0, 0.0, 0.0, 1.0), // red
    color4(1.0, 1.0, 0.0, 1.0), // yellow
    color4(0.0, 1.0, 0.0, 1.0), // green
    color4(0.0, 0.0, 1.0, 1.0), // blue
    color4(1.0, 0.0, 1.0, 1.0), // magenta
    color4(1.0, 1.0, 1.0, 1.0), // white
    color4(0.0, 1.0, 1.0, 1.0)  // cyan
};

GLuint vao[NUM_SHAPES];

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

//----------------------------------------------------------------------------

// quad generates two triangles for each face and assigns colors
//    to the vertices

int Index = 0;

void quad(int a, int b, int c, int d)
{
    // Initialize colors

    colors[Index] = vertex_colors[a];
    points[Index] = vertices[a];
    Index++;
    colors[Index] = vertex_colors[b];
    points[Index] = vertices[b];
    Index++;
    colors[Index] = vertex_colors[c];
    points[Index] = vertices[c];
    Index++;
    colors[Index] = vertex_colors[a];
    points[Index] = vertices[a];
    Index++;
    colors[Index] = vertex_colors[c];
    points[Index] = vertices[c];
    Index++;
    colors[Index] = vertex_colors[d];
    points[Index] = vertices[d];
    Index++;
}

//----------------------------------------------------------------------------

// generate 12 triangles: 36 vertices and 36 colors
void colorcube()
{
    quad(1, 0, 3, 2);
    quad(2, 3, 7, 6);
    quad(3, 0, 4, 7);
    quad(6, 5, 1, 2);
    quad(4, 5, 6, 7);
    quad(5, 4, 0, 1);
}

//----------------------------------------------------------------------------

// OpenGL initialization
void init()
{
    colorcube();

    // Create a vertex array object
    glGenVertexArrays(2, vao);
    glBindVertexArray(vao[0]);

    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);

    // Load shaders and use the resulting shader program
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");

    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)));

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");

    mat4 projection;
    // projection = Ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0); // Ortho(): user-defined function in mat.h
    projection = Perspective(45.0, 1.0, 0.5, 3.0); // try also perspective projection instead of ortho
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    // Set current program object
    glUseProgram(program);

    // Enable hiddden surface removal
    glEnable(GL_DEPTH_TEST);

    // Set state variable "clear color" to clear buffer with.
    glClearColor(1.0, 1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //  Generate the model-view matrix
    const vec3 displacement(0.0, 0.0, 0.0);
    mat4 model_view = (Translate(displacement) * Scale(0.2, 0.2, 0.2));

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

    glDrawArrays(GL_TRIANGLES, 0, NumVertices);

    glutSwapBuffers();
}

//---------------------------------------------------------------------
//
// reshape
//

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);

    // Set projection matrix
    mat4 projection;
    if (w <= h)
        projection = Ortho(-1.0, 1.0, -1.0 * (GLfloat)h / (GLfloat)w,
                           1.0 * (GLfloat)h / (GLfloat)w, -1.0, 1.0);
    else
        projection = Ortho(-1.0 * (GLfloat)w / (GLfloat)h, 1.0 * (GLfloat)w / (GLfloat)h, -1.0, 1.0, -1.0, 1.0);

    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    // reshape callback needs to be changed if perspective prohection is used
}

//----------------------------------------------------------------------------

void idle(void)
{
    glutPostRedisplay();
}

//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    if (key == 'Q' | key == 'q')
        exit(0);
}

//----------------------------------------------------------------------------

void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN)
    {
        switch (button)
        {
        }
    }
}

//----------------------------------------------------------------------------
void timer(int p)
{
    glutPostRedisplay();
    glutTimerFunc(0.03, timer, 0);
}

//----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Color Cube");

    glewExperimental = GL_TRUE;
    glewInit();

    init();

    glutDisplayFunc(display); // set display callback function
    glutReshapeFunc(reshape);
    // glutIdleFunc( idle );//can also use idle event for animation instaed of timer
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    glutTimerFunc(2, timer, 0);

    glutMainLoop();
    return 0;
}
