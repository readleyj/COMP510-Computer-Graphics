//
//  Display a rotating cube
//
#include "Angel.h"

typedef vec4 color4;
typedef vec4 point4;

const int NumVertices = 36;

point4 points[NumVertices];
color4 colors[NumVertices];

const GLfloat FOV = 90.0;
const GLfloat zNear = 0.5;
const GLfloat zFar = 5.0;

point4 vertices[8] = {
    point4(-0.5, -0.5, 0.5, 1.0),
    point4(-0.5, 0.5, 0.5, 1.0),
    point4(0.5, 0.5, 0.5, 1.0),
    point4(0.5, -0.5, 0.5, 1.0),
    point4(-0.5, -0.5, -0.5, 1.0),
    point4(-0.5, 0.5, -0.5, 1.0),
    point4(0.5, 0.5, -0.5, 1.0),
    point4(0.5, -0.5, -0.5, 1.0)};

color4 vertex_colors[6] = {
    color4(1.0, 1.0, 1.0, 1.0), // white
    color4(1.0, 0.0, 0.0, 1.0), // red
    color4(0.0, 0.0, 1.0, 1.0), // blue
    color4(1.0, 0.5, 0.0, 1.0), // orange
    color4(0.0, 1.0, 0.0, 1.0), // green
    color4(1.0, 1.0, 0.0, 1.0), // yellow
};

// Array of rotation angles (in degrees) for each coordinate axis
enum
{
    Xaxis = 0,
    Yaxis = 1,
    Zaxis = 2,
    NumAxes = 3
};

enum CubeColor
{
    WHITE,
    RED,
    BLUE,
    ORANGE,
    GREEN,
    YELLOW
};

int Axis = Xaxis;
GLfloat Theta[NumAxes] = {0.0, 0.0, 0.0};

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

//----------------------------------------------------------------------------

int Index = 0;

void quad(int a, int b, int c, int d, CubeColor color)
{
    int colorIdx = static_cast<int>(color);

    colors[Index] = vertex_colors[colorIdx];
    points[Index] = vertices[a];
    Index++;

    colors[Index] = vertex_colors[colorIdx];
    points[Index] = vertices[b];
    Index++;

    colors[Index] = vertex_colors[colorIdx];
    points[Index] = vertices[c];
    Index++;

    colors[Index] = vertex_colors[colorIdx];
    points[Index] = vertices[a];
    Index++;

    colors[Index] = vertex_colors[colorIdx];
    points[Index] = vertices[c];
    Index++;

    colors[Index] = vertex_colors[colorIdx];
    points[Index] = vertices[d];
    Index++;
}

//----------------------------------------------------------------------------

// generate 12 triangles: 36 vertices and 36 colors
void colorcube()
{
    quad(1, 0, 3, 2, WHITE);
    quad(2, 3, 7, 6, RED);
    quad(3, 0, 4, 7, BLUE);
    quad(6, 5, 1, 2, ORANGE);
    quad(4, 5, 6, 7, GREEN);
    quad(5, 4, 0, 1, YELLOW);
}

//----------------------------------------------------------------------------

// OpenGL initialization
void init()
{
    colorcube();

    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);

    // Load shaders and use the resulting shader program
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

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

    // Set projection matrix
    mat4 projection;
    projection = Perspective(FOV, 1.0, zNear, zFar);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

//----------------------------------------------------------------------------

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //  Generate the model-view matrix
    const vec3 displacement(0.0, 0.0, -3.0);
    mat4 model_view = (Translate(displacement) * Scale(1.0, 1.0, 1.0) *
                       RotateX(Theta[Xaxis]) *
                       RotateY(Theta[Yaxis]) *
                       RotateZ(Theta[Zaxis]));

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

    glDrawArrays(GL_TRIANGLES, 0, NumVertices);

    glutSwapBuffers();
}
//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 033: // Escape key
    case 'q':
    case 'Q':
        exit(EXIT_SUCCESS);
        break;
    }
}

//----------------------------------------------------------------------------

void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN)
    {
        switch (button)
        {
        case GLUT_LEFT_BUTTON:
            Axis = Xaxis;
            break;
        case GLUT_MIDDLE_BUTTON:
            Axis = Yaxis;
            break;
        case GLUT_RIGHT_BUTTON:
            Axis = Zaxis;
            break;
        }
    }
}

//----------------------------------------------------------------------------

void idle(void)
{
    Theta[Axis] += 4.0;

    if (Theta[Axis] > 360.0)
    {
        Theta[Axis] -= 360.0;
    }

    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1024, 1024);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Rubic's Cube");

    glewInit();

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutIdleFunc(idle);

    glutMainLoop();

    return 0;
}
