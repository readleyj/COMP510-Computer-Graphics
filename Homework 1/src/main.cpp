#include "Angel.h"

typedef vec4 color4;
typedef vec4 point4;

const vec3 TOP_LEFT_CORNER = vec3(-0.7, 1.0, -2.0);
const float INITIAL_HORIZONTAL_SPEED = 0.01;
const float INITIAL_VERTICAL_SPEED = -0.015;
const float INITIAL_Z_SPEED = -0.01;

const float SCALE_FACTOR = 0.20;
const float BALL_RADIUS = SCALE_FACTOR;
const float FOV = 90.0;
const float zNear = 0.5;
const float zFar = 3.0;

vec3 displacement = TOP_LEFT_CORNER;

float curHorizontalSpeed = INITIAL_HORIZONTAL_SPEED;
float curVerticalSpeed = INITIAL_VERTICAL_SPEED;
float curZSpeed = INITIAL_Z_SPEED;

bool is3D = true;

float leftWallBoundary = -1.0;
float rightWallBoundary = 1.0;
float bottomWallBoundary = -1.0;
float topWallBoundary = 1.0;
float backWallBoundary = -zFar;
float frontWallBoundary = -zNear;

int curWidth;
int curHeight;

color4 VERTEX_COLORS[8] = {
    color4(0.0, 0.0, 0.0, 1.0), // black
    color4(1.0, 0.0, 0.0, 1.0), // red
    color4(1.0, 1.0, 0.0, 1.0), // yellow
    color4(0.0, 1.0, 0.0, 1.0), // green
    color4(0.0, 0.0, 1.0, 1.0), // blue
    color4(1.0, 0.0, 1.0, 1.0), // magenta
    color4(1.0, 1.0, 1.0, 1.0), // white
    color4(0.0, 1.0, 1.0, 1.0)  // cyan
};

namespace cubeContext
{
    GLuint buffer;

    const int NumVertices = 36;

    point4 points[NumVertices];
    color4 colors[NumVertices];

    point4 vertices[8] = {
        point4(-1.0, -1.0, 1.0, 1.0),
        point4(-1.0, 1.0, 1.0, 1.0),
        point4(1.0, 1.0, 1.0, 1.0),
        point4(1.0, -1.0, 1.0, 1.0),
        point4(-1.0, -1.0, -1.0, 1.0),
        point4(-1.0, 1.0, -1.0, 1.0),
        point4(1.0, 1.0, -1.0, 1.0),
        point4(1.0, -1.0, -1.0, 1.0),
    };

    // quad generates two triangles for each face and assigns colors
    //    to the vertices

    int Index = 0;

    void quad(int a, int b, int c, int d)
    {
        colors[Index] = VERTEX_COLORS[a];
        points[Index] = vertices[a];
        Index++;

        colors[Index] = VERTEX_COLORS[b];
        points[Index] = vertices[b];
        Index++;

        colors[Index] = VERTEX_COLORS[c];
        points[Index] = vertices[c];
        Index++;

        colors[Index] = VERTEX_COLORS[a];
        points[Index] = vertices[a];
        Index++;

        colors[Index] = VERTEX_COLORS[c];
        points[Index] = vertices[c];
        Index++;

        colors[Index] = VERTEX_COLORS[d];
        points[Index] = vertices[d];
        Index++;
    }

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
}

namespace sphereContext
{
    GLuint buffer;

    const int NumTimesToSubdivide = 5;
    const int NumTriangles = 4096;
    const int NumVertices = 3 * NumTriangles;

    point4 points[NumVertices];
    color4 colors[NumVertices];

    int Index = 0;

    void triangle(const point4 &a, const point4 &b, const point4 &c)
    {

        points[Index] = a;
        colors[Index] = VERTEX_COLORS[Index % 8];
        Index++;

        points[Index] = b;
        colors[Index] = VERTEX_COLORS[Index % 8];

        Index++;

        points[Index] = c;
        colors[Index] = VERTEX_COLORS[Index % 8];
        Index++;
    }

    point4 unit(const point4 &p)
    {
        float len = p.x * p.x + p.y * p.y + p.z * p.z;

        point4 t;
        if (len > DivideByZeroTolerance)
        {
            t = p / sqrt(len);
            t.w = 1.0;
        }

        return t;
    }

    void divide_triangle(const point4 &a, const point4 &b,
                         const point4 &c, int count)
    {
        if (count > 0)
        {
            point4 v1 = unit(a + b);
            point4 v2 = unit(a + c);
            point4 v3 = unit(b + c);

            divide_triangle(a, v1, v2, count - 1);
            divide_triangle(c, v2, v3, count - 1);
            divide_triangle(b, v3, v1, count - 1);
            divide_triangle(v1, v3, v2, count - 1);
        }
        else
        {
            triangle(a, b, c);
        }
    }

    void tetrahedron(int count)
    {
        point4 v[4] = {
            vec4(0.0, 0.0, 1.0, 1.0),
            vec4(0.0, 0.942809, -0.333333, 1.0),
            vec4(-0.816497, -0.471405, -0.333333, 1.0),
            vec4(0.816497, -0.471405, -0.333333, 1.0)};

        divide_triangle(v[0], v[1], v[2], count);
        divide_triangle(v[3], v[2], v[1], count);
        divide_triangle(v[0], v[3], v[1], count);
        divide_triangle(v[0], v[2], v[3], count);
    }
}

enum BallShape
{
    SPHERE,
    CUBE,
    NUM_SHAPES
};

enum DrawMode
{
    SOLID,
    WIREFRAME,
    NUM_DRAW_MODES
};

BallShape curBallShape = SPHERE;
DrawMode curDrawMode = SOLID;

GLuint vao[NUM_SHAPES];

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

void setProjectionMatrix()
{
    mat4 projection;

    GLfloat aspect = (GLfloat)curWidth / (GLfloat)curHeight;

    if (is3D)
    {
        projection = Perspective(FOV, aspect, zNear, zFar);
    }
    else
    {
        projection = curWidth <= curHeight ? Ortho(-1.0, 1.0, -1.0 / aspect,
                                                   1.0 / aspect, zNear, zFar)
                                           : Ortho(-1.0 * aspect, 1.0 * aspect, -1.0, 1.0, zNear, zFar);
    }

    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
}

// OpenGL initialization
void init()
{
    cubeContext::colorcube();
    sphereContext::tetrahedron(sphereContext::NumTimesToSubdivide);

    // Load shaders and use the resulting shader program
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");

    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    GLuint vColor = glGetAttribLocation(program, "vColor");

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");

    mat4 projection;
    projection = Perspective(FOV, 1.0, zNear, zFar);

    // Create a vertex array object
    glGenVertexArrays(2, vao);
    glBindVertexArray(vao[0]);

    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(vColor);

    // Create and initialize a buffer object
    glGenBuffers(1, &cubeContext::buffer);
    glBindBuffer(GL_ARRAY_BUFFER, cubeContext::buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeContext::points) + sizeof(cubeContext::colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cubeContext::points), cubeContext::points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(cubeContext::points), sizeof(cubeContext::colors), cubeContext::colors);

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(cubeContext::points)));

    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    glBindVertexArray(vao[1]);

    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(vColor);

    glGenBuffers(1, &sphereContext::buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphereContext::buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphereContext::points) + sizeof(sphereContext::colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sphereContext::points), sphereContext::points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphereContext::points), sizeof(sphereContext::colors), sphereContext::colors);

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(sphereContext::points)));

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
    mat4 model_view = (Translate(displacement) * Scale(SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR));

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

    switch (curBallShape)
    {
    case CUBE:
        glBindVertexArray(vao[0]);
        glDrawArrays(GL_TRIANGLES, 0, cubeContext::NumVertices);
        break;
    case SPHERE:
        glBindVertexArray(vao[1]);
        glDrawArrays(GL_TRIANGLES, 0, sphereContext::NumVertices);
        break;
    }

    glFlush();
    glutSwapBuffers();
}

//---------------------------------------------------------------------
//
// reshape
//

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);

    curWidth = w;
    curHeight = h;

    GLfloat aspect = (GLfloat)curWidth / (GLfloat)curHeight;

    if (curWidth <= curHeight)
    {
        bottomWallBoundary = -1.0 / aspect;
        topWallBoundary = 1.0 / aspect;
    }
    else
    {
        leftWallBoundary = -1.0 * aspect;
        rightWallBoundary = 1.0 * aspect;
    }

    setProjectionMatrix();
}

//----------------------------------------------------------------------------

void idle(void)
{
    if (displacement.x + curHorizontalSpeed <= leftWallBoundary + BALL_RADIUS)
    {
        displacement.x = leftWallBoundary + BALL_RADIUS;
        curHorizontalSpeed = -curHorizontalSpeed;
    }
    else if (displacement.x + curHorizontalSpeed >= rightWallBoundary - BALL_RADIUS)
    {

        displacement.x = rightWallBoundary - BALL_RADIUS;
        curHorizontalSpeed = -curHorizontalSpeed;
    }

    if (displacement.y + curVerticalSpeed <= bottomWallBoundary + BALL_RADIUS)
    {
        displacement.y = bottomWallBoundary + BALL_RADIUS;
        curVerticalSpeed = -curVerticalSpeed;
    }
    else if (displacement.y + curVerticalSpeed >= topWallBoundary - BALL_RADIUS)
    {
        displacement.y = topWallBoundary - BALL_RADIUS;
        curVerticalSpeed = -curVerticalSpeed;
    }

    if (is3D)
    {
        if (displacement.z + curZSpeed <= backWallBoundary + BALL_RADIUS)
        {
            displacement.z = backWallBoundary + BALL_RADIUS;
            curZSpeed = -curZSpeed;
        }
        else if (displacement.z + curZSpeed >= frontWallBoundary - BALL_RADIUS)
        {
            displacement.z = frontWallBoundary - BALL_RADIUS;
            curZSpeed = -curZSpeed;
        }
    }

    displacement += vec3(curHorizontalSpeed, curVerticalSpeed, curZSpeed);

    glutPostRedisplay();
}

//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    if (key == 'D' | key == 'd')
    {
        curDrawMode = DrawMode((curDrawMode + 1) % NUM_DRAW_MODES);

        if (curDrawMode == SOLID)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else if (curDrawMode == WIREFRAME)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
    }

    if (key == 'I' | key == 'i')
    {
        displacement = TOP_LEFT_CORNER;
        curHorizontalSpeed = INITIAL_HORIZONTAL_SPEED;
        curVerticalSpeed = INITIAL_VERTICAL_SPEED;
    }

    if (key == 'V' | key == 'v')
    {
        is3D = !is3D;
        curZSpeed = is3D ? INITIAL_Z_SPEED : 0;

        setProjectionMatrix();
    }

    if (key == 'Q' | key == 'q')
    {
        exit(0);
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
            curBallShape = BallShape((curBallShape + 1) % NUM_SHAPES);
            break;
        }
    }
}

//----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1024, 1024);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Color Cube");

    glewExperimental = GL_TRUE;
    glewInit();

    init();

    glutDisplayFunc(display); // set display callback function
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
