#include <vector>
#include <set>

#include "Angel.h"

typedef vec4 color4;
typedef vec4 point4;

GLuint PROGRAM;

const int RUBICS_CUBE_DIM = 3;
const int NUM_CUBES = RUBICS_CUBE_DIM * RUBICS_CUBE_DIM * RUBICS_CUBE_DIM;
const int NUM_CUBE_FACES = 6;

const GLfloat CUBE_WIDTH = 0.45;
const GLfloat CUBE_HEIGHT = 0.45;
const GLfloat CUBE_DEPTH = 0.45;
const GLfloat BORDER_WIDTH = 0.025;

const GLfloat START_COORD = -0.7;
const GLfloat END_COORD = 0.7;

const GLfloat FOV = 90.0;
const GLfloat zNear = 0.5;
const GLfloat zFar = 5.0;

const vec4 camera_pos = vec4(-3.0, 1.5, 0.0, 1.0);

GLfloat rotation_increment = 3.0;

mat4 globalModelView;

enum
{
    Xaxis = 0,
    Yaxis = 1,
    Zaxis = 2,
    NumAxes = 3
};

int Axis = Xaxis;
GLfloat Theta[NumAxes] = {0.0, 0.0, 0.0};

color4 VERTEX_COLORS[7] = {
    color4(1.0, 1.0, 1.0, 1.0), // white
    color4(1.0, 0.0, 0.0, 1.0), // red
    color4(0.0, 0.0, 1.0, 1.0), // blue
    color4(1.0, 0.5, 0.0, 1.0), // orange
    color4(0.0, 1.0, 0.0, 1.0), // green
    color4(1.0, 1.0, 0.0, 1.0), // yellow
    color4(0.0, 0.0, 0.0, 1.0), // yellow
};

enum FaceColor
{
    WHITE,
    RED,
    BLUE,
    ORANGE,
    GREEN,
    YELLOW,
    BLACK
};

enum FacePosition
{
    LEFT,
    RIGHT,
    BOTTOM,
    TOP,
    BACK,
    FRONT,
    NUM_POSITIONS
};

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

//----------------------------------------------------------------------------

namespace RubicsCubeContext
{
    const int NUM_VERTICES_PER_CUBE = 36;
    const int NUM_VERTICES_PER_FACE = 6;

    // Contains a set of cube indices for each afce
    std::vector<std::set<int>> face_to_cube_set;

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

        face_to_cube_set.resize(NUM_CUBE_FACES);

        for (size_t cube_idx = 0; cube_idx < NUM_CUBES; cube_idx++)
        {
            points[cube_idx].resize(NUM_VERTICES_PER_CUBE);
            colors[cube_idx].resize(NUM_VERTICES_PER_CUBE);

            int x_idx = cube_idx / (RUBICS_CUBE_DIM * RUBICS_CUBE_DIM);
            int y_idx = (cube_idx / RUBICS_CUBE_DIM) % RUBICS_CUBE_DIM;
            int z_idx = cube_idx % RUBICS_CUBE_DIM;

            // Right Face
            if (z_idx == (RUBICS_CUBE_DIM - 1))
            {
                face_to_cube_set[static_cast<int>(RIGHT)].insert(cube_idx);
            }
            // Left Face
            else if (z_idx == 0)
            {
                face_to_cube_set[static_cast<int>(LEFT)].insert(cube_idx);
            }

            // Top Face
            if (y_idx == (RUBICS_CUBE_DIM - 1))
            {
                face_to_cube_set[static_cast<int>(TOP)].insert(cube_idx);
            }
            // Bottom Face
            else if (y_idx == 0)
            {
                face_to_cube_set[static_cast<int>(BOTTOM)].insert(cube_idx);
            }

            // Back Face
            if (x_idx == (RUBICS_CUBE_DIM - 1))
            {
                face_to_cube_set[static_cast<int>(BACK)].insert(cube_idx);
            }
            // Front Face
            else if (x_idx == 0)
            {
                face_to_cube_set[static_cast<int>(FRONT)].insert(cube_idx);
            }
        }

        for (size_t cube_idx = 0; cube_idx < NUM_CUBES; cube_idx++)
        {
            int x_idx = cube_idx / (RUBICS_CUBE_DIM * RUBICS_CUBE_DIM);
            int y_idx = (cube_idx / RUBICS_CUBE_DIM) % RUBICS_CUBE_DIM;
            int z_idx = cube_idx % RUBICS_CUBE_DIM;

            GLfloat start_x_coord = START_COORD + x_idx * CUBE_WIDTH + x_idx * BORDER_WIDTH;
            GLfloat end_x_coord = start_x_coord + CUBE_WIDTH;

            GLfloat start_y_coord = START_COORD + y_idx * CUBE_HEIGHT + y_idx * BORDER_WIDTH;
            GLfloat end_y_coord = start_y_coord + CUBE_HEIGHT;

            GLfloat start_z_coord = START_COORD + z_idx * CUBE_DEPTH + z_idx * BORDER_WIDTH;
            GLfloat end_z_coord = start_z_coord + CUBE_DEPTH;

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

            FaceColor color_left = BLACK;
            FaceColor color_right = BLACK;
            FaceColor color_bottom = BLACK;
            FaceColor color_top = BLACK;
            FaceColor color_back = BLACK;
            FaceColor color_front = BLACK;

            if (face_to_cube_set[static_cast<int>(LEFT)].count(cube_idx))
            {
                color_left = ORANGE;
            }
            else if (face_to_cube_set[static_cast<int>(RIGHT)].count(cube_idx))
            {
                color_right = RED;
            }

            if (face_to_cube_set[static_cast<int>(BOTTOM)].count(cube_idx))
            {
                color_bottom = YELLOW;
            }
            else if (face_to_cube_set[static_cast<int>(TOP)].count(cube_idx))
            {
                color_top = WHITE;
            }

            if (face_to_cube_set[static_cast<int>(BACK)].count(cube_idx))
            {
                color_back = BLUE;
            }
            else if (face_to_cube_set[static_cast<int>(FRONT)].count(cube_idx))
            {
                color_front = GREEN;
            }

            // Right
            quad(1, 0, 3, 2, cube_idx, 0, color_right, base_vertices);

            // Back
            quad(2, 3, 7, 6, cube_idx, 1, color_back, base_vertices);

            // Bottom
            quad(3, 0, 4, 7, cube_idx, 2, color_bottom, base_vertices);

            // Top
            quad(6, 5, 1, 2, cube_idx, 3, color_top, base_vertices);

            // Left
            quad(4, 5, 6, 7, cube_idx, 4, color_left, base_vertices);

            // Front
            quad(5, 4, 0, 1, cube_idx, 5, color_front, base_vertices);
        }

        for (size_t i = 0; i < NUM_CUBES; i++)
        {
            GLuint cur_buffer = vertex_buffers[i];

            glBindVertexArray(vaos[i]);
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
    }

    void initModelViewMatrices()
    {
        for (size_t i = 0; i < NUM_CUBES; i++)
        {
            model_view_matrices[i] = globalModelView;
        }
    }

    void render()
    {
        for (size_t i = 0; i < NUM_CUBES; i++)
        {
            glBindVertexArray(vaos[i]);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[i]);

            GLuint vPosition = glGetAttribLocation(PROGRAM, "vPosition");
            glEnableVertexAttribArray(vPosition);
            glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

            GLuint vColor = glGetAttribLocation(PROGRAM, "vColor");
            glEnableVertexAttribArray(vColor);
            glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(points[i].size() * sizeof(point4)));

            mat4 new_model_view = model_view_matrices[i] * RotateX(Theta[Xaxis]) *
                                  RotateY(Theta[Yaxis]) *
                                  RotateZ(Theta[Zaxis]);

            glUniformMatrix4fv(ModelView, 1, GL_TRUE, new_model_view);

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

    vec4 at = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 eye = camera_pos;
    vec4 up = vec4(0.0, 1.0, 0.0, 1.0);

    mat4 model_view = LookAt(eye, at, up);

    globalModelView = model_view;

    RubicsCubeContext::initModelViewMatrices();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

//----------------------------------------------------------------------------

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RubicsCubeContext::render();

    glutSwapBuffers();
}
//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    bool isShiftActive = glutGetModifiers() && GLUT_ACTIVE_SHIFT;
    int modifier = isShiftActive ? -1 : 1;

    if (key == 'H' || key == 'h')
    {
        Theta[Xaxis] += modifier * rotation_increment;
    }
    else if (key == 'J' || key == 'j')
    {
        Theta[Yaxis] += modifier * rotation_increment;
    }
    else if (key == 'K' || key == 'k')
    {
        Theta[Zaxis] += modifier * rotation_increment;
    }
    else if (key == 'q' || key == 'Q' || key == 033)
    {
        exit(EXIT_SUCCESS);
    }
}

//----------------------------------------------------------------------------

void mouse(int button, int state, int x, int y)
{
}

//----------------------------------------------------------------------------

void idle(void)
{
    if (Theta[Axis] > 360.0)
    {
        Theta[Axis] -= 360.0;
    }
    else if (Theta[Axis] < 0.0)
    {
        Theta[Axis] += 360.0;
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
