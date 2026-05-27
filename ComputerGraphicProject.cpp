#define GLFW_DLL
#define GLEW_DLL
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <iostream>
#include <cmath> // Нужен для cosf и sinf

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int drawEllipse(float cx, float cy, float rx, float ry, int segments, GLuint& VAO, GLuint& VBO)
{
    // Общее количество вершин: центр (1) + сегменты + замыкающая вершина (1)
    int vertexCount = segments + 2;

    int totalCoordinates = vertexCount * 3;

    // Выделяем память под массив координат на куче
    float* points = new float[totalCoordinates];

    int index = 0;

    // Центральная точка веера треугольников
    points[index++] = cx;
    points[index++] = cy;
    points[index++] = 0.0f;

    // Генерация точек по окружности
    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * (float)M_PI * float(i) / float(segments);
        points[index++] = cx + rx * cosf(theta); // X
        points[index++] = cy + ry * sinf(theta); // Y
        points[index++] = 0.0f;                  // Z
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Загрузка массива в видеопамять
    glBufferData(GL_ARRAY_BUFFER, totalCoordinates * sizeof(float), points, GL_STATIC_DRAW);

    // Описание формата вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    delete[] points;

    return vertexCount;
}


int main()
{
    if (!glfwInit())
    {
        fprintf(stderr, "Error: couldn't start GLFW3.\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* MyWindow = glfwCreateWindow(512, 512, "Modern OpenGL Ellipse Function", NULL, NULL);

    if (!MyWindow)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(MyWindow);
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Error initializing GLEW\n");
        glfwTerminate();
        return 1;
    }

    GLuint ellipseVAO, ellipseVBO;

    // Вызов функции создания эллипса (центр 0.0, 0.0, радиусы 0.5 и 0.25, 200 сегментов)
    int vertexCount = drawEllipse(0.0f, 0.0f, 0.5f, 0.25f, 200, ellipseVAO, ellipseVBO);

    // Шейдеры
    const char* vert_shader =
        "#version 410 core\n"
        "layout (location=0) in vec3 vp;"
        "void main() {"
        "   gl_Position = vec4(vp, 1.0);"
        "}";

    const char* frag_shader =
        "#version 410 core\n"
        "uniform vec4 ourColor;"
        "out vec4 frag_colour;"
        "void main() {"
        "   frag_colour = ourColor;"
        "}";

    GLuint vert_shaders = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shaders, 1, &vert_shader, NULL);
    glCompileShader(vert_shaders);

    GLuint frag_shaders = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shaders, 1, &frag_shader, NULL);
    glCompileShader(frag_shaders);

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, frag_shaders);
    glAttachShader(shader_program, vert_shaders);
    glLinkProgram(shader_program);

    glDeleteShader(vert_shaders);
    glDeleteShader(frag_shaders);

    GLint vertexColorLocation = glGetUniformLocation(shader_program, "ourColor");

    while (!glfwWindowShouldClose(MyWindow))
    {
        glfwPollEvents();

        glClearColor(0.3f, 1.0f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glUniform4f(vertexColorLocation, 1.0f, 0.2f, 1.0f, 1.0f); // розовый цвет

        // Отрисовка сгенерированного эллипса
        glBindVertexArray(ellipseVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
        glBindVertexArray(0);

        glfwSwapBuffers(MyWindow);
    }

    // Очистка ресурсов
    glDeleteVertexArrays(1, &ellipseVAO);
    glDeleteBuffers(1, &ellipseVBO);
    glDeleteProgram(shader_program);

    glfwTerminate();
    return 0;
}