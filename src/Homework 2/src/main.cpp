#include <vector>
#include <algorithm>
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

vec4 eye;
vec4 up;
vec4 at;

mat4 globalModelView;

float angle = 0.0;
float axis[3];

bool trackballMove = false;

float lastPos[3] = {0.0, 0.0, 0.0};

int curX, curY;
int startX, startY;

int curWidth;
int curHeight;

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

float radians(float degrees)
{
    return degrees * M_PI / 180.0;
}

mat4 rotate(float angle, vec3 axis)
{
    vec3 v = normalize(axis);

    float x = v[0];
    float y = v[1];
    float z = v[2];

    float c = cos(radians(angle));
    float omc = 1.0 - c;
    float s = sin(radians(angle));

    mat4 result = mat4(
        vec4(x * x * omc + c, x * y * omc - z * s, x * z * omc + y * s, 0.0),
        vec4(x * y * omc + z * s, y * y * omc + c, y * z * omc - x * s, 0.0),
        vec4(x * z * omc - y * s, y * z * omc + x * s, z * z * omc + c, 0.0),
        vec4());

    return result;
}

void set_trackball_vector(int x, int y, float v[3])
{
    float d;
    float a;

    v[0] = (2.0 * x - curWidth) / curWidth;
    v[1] = (curHeight - 2.0F * y) / curHeight;

    d = sqrt(v[0] * v[0] + v[1] * v[1]);

    v[2] = cos((M_PI / 2.0) * ((d < 1.0) ? d : 1.0));

    a = 1.0 / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= a;
    v[1] *= a;
    v[2] *= a;
}

void startMotion(int x, int y)
{
    startX = x;
    startY = y;

    curX = x;
    curY = y;

    set_trackball_vector(x, y, lastPos);

    trackballMove = true;
}

void stopMotion(int x, int y)
{
    trackballMove = false;
    angle = 0.0;
}

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

        for (size_t i = 0; i < NUM_CUBES; i++)
        {
            model_view_matrices[i] = mat4();
        }

        loadData();
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

            mat4 new_model_view = globalModelView * model_view_matrices[i];

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

    at = vec4(0.0, 0.0, 0.0, 1.0);
    eye = camera_pos;
    up = vec4(0.0, 1.0, 0.0, 1.0);

    mat4 model_view = LookAt(eye, at, up);

    globalModelView = model_view;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

//----------------------------------------------------------------------------

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (trackballMove && (axis[0] || axis[1] || axis[2]))
    {
        mat4 rotationMatrix = rotate(angle, vec3(axis[0], axis[1], axis[2]));

        eye = rotationMatrix * eye;
        up = rotationMatrix * up;
        at = rotationMatrix * at;

        globalModelView = LookAt(eye, at, up);
    }

    RubicsCubeContext::render();

    glutSwapBuffers();
}
//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    if (key == 'F' || key == 'f')
    {
        std::set<int> &front = RubicsCubeContext::face_to_cube_set[static_cast<int>(FRONT)];
        std::set<int> &top = RubicsCubeContext::face_to_cube_set[static_cast<int>(TOP)];
        std::set<int> &bottom = RubicsCubeContext::face_to_cube_set[static_cast<int>(BOTTOM)];
        std::set<int> &right = RubicsCubeContext::face_to_cube_set[static_cast<int>(RIGHT)];
        std::set<int> &left = RubicsCubeContext::face_to_cube_set[static_cast<int>(LEFT)];

        std::vector<int> front_top_intersec(RUBICS_CUBE_DIM);
        std::vector<int> front_right_intersec(RUBICS_CUBE_DIM);
        std::vector<int> front_bottom_intersec(RUBICS_CUBE_DIM);
        std::vector<int> front_left_intersec(RUBICS_CUBE_DIM);

        for (int cubeIdx : front)
        {
            RubicsCubeContext::model_view_matrices[cubeIdx] *= RotateX(90.0);
        }

        std::set_intersection(front.begin(), front.end(), top.begin(), top.end(), front_top_intersec.begin());
        std::set_intersection(front.begin(), front.end(), right.begin(), right.end(), front_right_intersec.begin());
        std::set_intersection(front.begin(), front.end(), bottom.begin(), bottom.end(), front_bottom_intersec.begin());
        std::set_intersection(front.begin(), front.end(), left.begin(), left.end(), front_left_intersec.begin());

        for (int idx = 0; idx < RUBICS_CUBE_DIM; idx++)
        {
            printf("Erasing %d from top\n", front_top_intersec[idx]);
            printf("Inserting %d to top\n", front_left_intersec[idx]);

            top.erase(front_top_intersec[idx]);
            top.insert(front_left_intersec[idx]);

            right.erase(front_right_intersec[idx]);
            right.insert(front_top_intersec[idx]);

            bottom.erase(front_bottom_intersec[idx]);
            bottom.insert(front_right_intersec[idx]);

            left.erase(front_left_intersec[idx]);
            left.insert(front_bottom_intersec[idx]);
        }
    }
    else if (key == 'B' || key == 'b')
    {
        std::set<int> &back = RubicsCubeContext::face_to_cube_set[static_cast<int>(BACK)];
        std::set<int> &top = RubicsCubeContext::face_to_cube_set[static_cast<int>(TOP)];
        std::set<int> &bottom = RubicsCubeContext::face_to_cube_set[static_cast<int>(BOTTOM)];
        std::set<int> &right = RubicsCubeContext::face_to_cube_set[static_cast<int>(RIGHT)];
        std::set<int> &left = RubicsCubeContext::face_to_cube_set[static_cast<int>(LEFT)];

        std::vector<int> back_top_intersec(RUBICS_CUBE_DIM);
        std::vector<int> back_right_intersec(RUBICS_CUBE_DIM);
        std::vector<int> back_bottom_intersec(RUBICS_CUBE_DIM);
        std::vector<int> back_left_intersec(RUBICS_CUBE_DIM);

        for (int cubeIdx : back)
        {
            RubicsCubeContext::model_view_matrices[cubeIdx] *= RotateX(90.0);
        }

        std::set_intersection(back.begin(), back.end(), top.begin(), top.end(), back_top_intersec.begin());
        std::set_intersection(back.begin(), back.end(), right.begin(), right.end(), back_right_intersec.begin());
        std::set_intersection(back.begin(), back.end(), bottom.begin(), bottom.end(), back_bottom_intersec.begin());
        std::set_intersection(back.begin(), back.end(), left.begin(), left.end(), back_left_intersec.begin());

        for (int idx = 0; idx < RUBICS_CUBE_DIM; idx++)
        {
            top.erase(back_top_intersec[idx]);
            top.insert(back_left_intersec[idx]);

            right.erase(back_right_intersec[idx]);
            right.insert(back_top_intersec[idx]);

            bottom.erase(back_bottom_intersec[idx]);
            bottom.insert(back_right_intersec[idx]);

            left.erase(back_left_intersec[idx]);
            left.insert(back_bottom_intersec[idx]);
        }
    }
    else if (key == 'U' || key == 'u')
    {
        for (int cubeIdx : RubicsCubeContext::face_to_cube_set[static_cast<int>(TOP)])
        {
            RubicsCubeContext::model_view_matrices[cubeIdx] *= RotateY(90.0);
        }
    }
    else if (key == 'D' || key == 'd')
    {
        for (int cubeIdx : RubicsCubeContext::face_to_cube_set[static_cast<int>(BOTTOM)])
        {
            RubicsCubeContext::model_view_matrices[cubeIdx] = RotateY(45.0);
        }
    }
    else if (key == 'R' || key == 'r')
    {
        for (int cubeIdx : RubicsCubeContext::face_to_cube_set[static_cast<int>(RIGHT)])
        {
            RubicsCubeContext::model_view_matrices[cubeIdx] = RotateZ(45.0);
        }
    }
    else if (key == 'L' || key == 'l')
    {
        for (int cubeIdx : RubicsCubeContext::face_to_cube_set[static_cast<int>(LEFT)])
        {
            RubicsCubeContext::model_view_matrices[cubeIdx] = RotateZ(45.0);
        }
    }
}

//----------------------------------------------------------------------------

void mouse(int button, int state, int x, int y)
{

    if (button == GLUT_LEFT_BUTTON)
    {
        switch (state)
        {
        case GLUT_DOWN:
            startMotion(x, y);
            break;
        case GLUT_UP:
            stopMotion(x, y);
            break;
        }
    }
}

//----------------------------------------------------------------------------

void mouseMotion(int x, int y)
{
    float curPos[3];

    float dx;
    float dy;
    float dz;

    set_trackball_vector(x, y, curPos);

    if (trackballMove)
    {
        dx = curPos[0] - lastPos[0];
        dy = curPos[1] - lastPos[1];
        dz = curPos[2] - lastPos[2];

        if (dx || dy || dz)
        {
            angle = 90.0 * sqrt(dx * dx + dy * dy + dz * dz);
            axis[0] = lastPos[1] * curPos[2] - lastPos[2] * curPos[1];
            axis[1] = lastPos[2] * curPos[0] - lastPos[0] * curPos[2];
            axis[2] = lastPos[0] * curPos[1] - lastPos[1] * curPos[0];

            lastPos[0] = curPos[0];
            lastPos[1] = curPos[1];
            lastPos[2] = curPos[2];
        }
    }

    glutPostRedisplay();
}

//----------------------------------------------------------------------------

void reshape(int w, int h)
{
    curWidth = w;
    curHeight = h;

    glViewport(0, 0, w, h);
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
    glutReshapeFunc(reshape);
    glutMotionFunc(mouseMotion);
    glutIdleFunc(idle);

    glutMainLoop();

    return 0;
}
