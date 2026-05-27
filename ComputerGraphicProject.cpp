#include <iostream>
#include <cmath>
#include "glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <glad/glad.h> 
#include <GLFW/glfw3.h> 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Глобальные переменные для камеры
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f); // Камера на 3 единицы по Z
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Обработка мыши
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float yaw = -90.0f; // Смотрим в сторону -Z
float pitch = 0.0f;

// Прототипы функций управления
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
// Функция отрисовки эллипса
int drawEllipse(float cx, float cy, float rx, float ry, int segments, GLuint& VAO, GLuint& VBO)
{
    int vertexCount = segments + 2;
    int totalCoordinates = vertexCount * 3;
    float* points = new float[totalCoordinates];
    int index = 0;

    points[index++] = cx;
    points[index++] = cy;
    points[index++] = 0.0f;

    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * (float)M_PI * float(i) / float(segments);
        points[index++] = cx + rx * cosf(theta);
        points[index++] = cy + ry * sinf(theta);
        points[index++] = 0.0f;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, totalCoordinates * sizeof(float), points, GL_STATIC_DRAW);

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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Создаем окно
    GLFWwindow* MyWindow = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MyWindow", NULL, NULL);

    if (!MyWindow)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(MyWindow);

    // Инициализация GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Error initializing GLAD\n");
        glfwTerminate();
        return 1;
    }

    // Настройки захвата мыши
    glfwSetInputMode(MyWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(MyWindow, mouse_callback);

    // Включаем тест глубины для 3D сцены
    glEnable(GL_DEPTH_TEST);

    GLuint ellipseVAO, ellipseVBO;
    int vertexCount = drawEllipse(0.0f, 0.0f, 0.5f, 0.25f, 200, ellipseVAO, ellipseVBO);

    const char* vert_shader =
        "#version 330 core\n"
        "layout (location=0) in vec3 vp;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * view * model * vec4(vp, 1.0);\n"
        "}\0";

    const char* frag_shader =
        "#version 330 core\n"
        "uniform vec4 ourColor;\n"
        "out vec4 frag_colour;\n"
        "void main() {\n"
        "   frag_colour = ourColor;\n"
        "}\0";

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

    // Получаем локации матриц из шейдера
    GLint modelLoc = glGetUniformLocation(shader_program, "model");
    GLint viewLoc = glGetUniformLocation(shader_program, "view");
    GLint projLoc = glGetUniformLocation(shader_program, "projection");

    while (!glfwWindowShouldClose(MyWindow))
    {
        glfwPollEvents();

        // Обработка WASD ввода каждый кадр
        processInput(MyWindow);

        glClearColor(0.3f, 1.0f, 0.4f, 1.0f); //Зелёный
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader_program);
        glUniform4f(vertexColorLocation, 1.0f, 0.2f, 1.0f, 1.0f); // Розовый

        // Матрица проекции
        float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Отрисовка
        glBindVertexArray(ellipseVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
        glBindVertexArray(0);

        glfwSwapBuffers(MyWindow);
    }

    glDeleteVertexArrays(1, &ellipseVAO);
    glDeleteBuffers(1, &ellipseVBO);
    glDeleteProgram(shader_program);

    glfwTerminate();
    return 0;
}

// Реализация WASD движения
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const float cameraSpeed = 0.05f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// Реализация обзора мышью
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
}