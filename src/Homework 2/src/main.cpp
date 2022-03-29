#include <vector>

#include "Angel.h"

typedef vec4 color4;
typedef vec4 point4;

GLuint PROGRAM;

const int RUBICS_CUBE_DIM = 3;
const int NUM_CUBES = RUBICS_CUBE_DIM * RUBICS_CUBE_DIM * RUBICS_CUBE_DIM;

const GLfloat CUBE_WIDTH = 0.4;
const GLfloat CUBE_HEIGHT = 0.4;
const GLfloat CUBE_DEPTH = 0.4;

const GLfloat START_COORD = -0.6;
const GLfloat END_COORD = 0.6;

const GLfloat FOV = 90.0;
const GLfloat zNear = 0.5;
const GLfloat zFar = 5.0;

color4 VERTEX_COLORS[6] = {
    color4(1.0, 1.0, 1.0, 1.0), // white
    color4(1.0, 0.0, 0.0, 1.0), // red
    color4(0.0, 0.0, 1.0, 1.0), // blue
    color4(1.0, 0.5, 0.0, 1.0), // orange
    color4(0.0, 1.0, 0.0, 1.0), // green
    color4(1.0, 1.0, 0.0, 1.0), // yellow
};

enum FaceColor
{
    WHITE,
    RED,
    BLUE,
    ORANGE,
    GREEN,
    YELLOW
};

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

//----------------------------------------------------------------------------

namespace RubicsCubeContext
{
    const int NUM_VERTICES_PER_CUBE = 36;
    const int NUM_VERTICES_PER_FACE = 6;

    GLuint vertex_buffers[NUM_CUBES];
    GLuint vaos[NUM_CUBES];
    mat4 model_view_matrices[NUM_CUBES];

    std::vector<point4> points[NUM_CUBES];
    std::vector<color4> colors[NUM_CUBES];

    void quad(int a, int b, int c, int d,
              int cube_idx, int quad_num, FaceColor color,
              point4 base_vertices[8]);

    void loadData()
    {

        for (size_t i = 0; i < NUM_CUBES; i++)
        {
            points[i].resize(NUM_VERTICES_PER_CUBE);
            colors[i].resize(NUM_VERTICES_PER_CUBE);
        }

        for (size_t cube_idx = 0; cube_idx < NUM_CUBES; cube_idx++)
        {
            int x_idx = cube_idx / (RUBICS_CUBE_DIM * RUBICS_CUBE_DIM);
            int y_idx = (cube_idx / RUBICS_CUBE_DIM) % RUBICS_CUBE_DIM;
            int z_idx = cube_idx % RUBICS_CUBE_DIM;

            GLfloat start_x_coord = START_COORD + CUBE_WIDTH * x_idx;
            GLfloat end_x_coord = std::min(START_COORD + CUBE_WIDTH * (x_idx + 1), END_COORD);

            GLfloat start_y_coord = START_COORD + CUBE_HEIGHT * y_idx;
            GLfloat end_y_coord = std::min(START_COORD + CUBE_HEIGHT * (y_idx + 1), END_COORD);

            GLfloat start_z_coord = START_COORD + CUBE_DEPTH * z_idx;
            GLfloat end_z_coord = std::min(START_COORD + CUBE_DEPTH * (z_idx + 1), END_COORD);

            point4 base_vertices[8] = {
                point4(start_x_coord, start_y_coord, end_z_coord, 1.0),
                point4(start_x_coord, end_y_coord, end_z_coord, 1.0),
                point4(end_x_coord, end_y_coord, end_z_coord, 1.0),
                point4(end_x_coord, start_y_coord, end_z_coord, 1.0),
                point4(start_x_coord, start_y_coord, start_z_coord, 1.0),
                point4(start_x_coord, end_y_coord, start_z_coord, 1.0),
                point4(end_x_coord, end_y_coord, start_z_coord, 1.0),
                point4(end_x_coord, start_y_coord, start_z_coord, 1.0),
            };

            quad(1, 0, 3, 2, cube_idx, 0, RED, base_vertices);
            quad(2, 3, 7, 6, cube_idx, 1, RED, base_vertices);
            quad(3, 0, 4, 7, cube_idx, 2, RED, base_vertices);
            quad(6, 5, 1, 2, cube_idx, 3, RED, base_vertices);
            quad(4, 5, 6, 7, cube_idx, 4, RED, base_vertices);
            quad(5, 4, 0, 1, cube_idx, 5, RED, base_vertices);
        }

        for (size_t i = 0; i < NUM_CUBES; i++)
        {
            GLuint cur_buffer = vertex_buffers[i];

            glBindBuffer(GL_ARRAY_BUFFER, cur_buffer);

            glBufferData(GL_ARRAY_BUFFER, points[i].size() * sizeof(point4) + colors[i].size() * sizeof(point4), NULL, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, points[i].size() * sizeof(point4), &points[i][0]);
            glBufferSubData(GL_ARRAY_BUFFER, points[i].size() * sizeof(point4), colors[i].size() * sizeof(point4), &colors[i][0]);
        }
    }

    void init()
    {
        glGenBuffers(NUM_CUBES, vertex_buffers);
        glGenVertexArrays(NUM_CUBES, vaos);

        loadData();

        for (size_t i = 0; i < NUM_CUBES; i++)
        {
            model_view_matrices[i] = Translate(0.0, 0.0, 0.0);
        }
    }

    void render()
    {
        for (size_t i = 0; i < NUM_CUBES; i++)
        {
            glBindVertexArray(vaos[i]);

            GLuint vPosition = glGetAttribLocation(PROGRAM, "vPosition");
            glEnableVertexAttribArray(vPosition);
            glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

            GLuint vColor = glGetAttribLocation(PROGRAM, "vColor");
            glEnableVertexAttribArray(vColor);
            glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(points[i].size() * sizeof(point4)));

            // glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view_matrices[i]);

            glDrawArrays(GL_TRIANGLES, 0, NUM_VERTICES_PER_CUBE);
        }
    }

    void quad(int a, int b, int c, int d,
              int cube_idx, int quad_num, FaceColor color,
              point4 base_vertices[8])
    {
        int index = NUM_VERTICES_PER_FACE * quad_num;

        int color_idx = static_cast<int>(color);

        colors[cube_idx][index] = VERTEX_COLORS[color_idx];
        points[cube_idx][index] = base_vertices[a];
        index++;

        colors[cube_idx][index] = VERTEX_COLORS[color_idx];
        points[cube_idx][index] = base_vertices[b];
        index++;

        colors[cube_idx][index] = VERTEX_COLORS[color_idx];
        points[cube_idx][index] = base_vertices[c];
        index++;

        colors[cube_idx][index] = VERTEX_COLORS[color_idx];
        points[cube_idx][index] = base_vertices[a];
        index++;

        colors[cube_idx][index] = VERTEX_COLORS[color_idx];
        points[cube_idx][index] = base_vertices[c];
        index++;

        colors[cube_idx][index] = VERTEX_COLORS[color_idx];
        points[cube_idx][index] = base_vertices[d];
        index++;
    }

}

//----------------------------------------------------------------------------

// OpenGL initialization
void init()
{
    PROGRAM = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(PROGRAM);

    RubicsCubeContext::init();

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(PROGRAM, "ModelView");
    Projection = glGetUniformLocation(PROGRAM, "Projection");

    // Set projection matrix
    mat4 projection;
    projection = Perspective(FOV, 1.0, zNear, zFar);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0, 1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 model_view = Translate(vec3(0.0, 0.0, -2.0)) * Scale(1.0, 1.0, 1.0);
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

    RubicsCubeContext::render();

    glutSwapBuffers();
}
//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 033:
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
        }
    }
}

//----------------------------------------------------------------------------

void idle(void)
{
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
