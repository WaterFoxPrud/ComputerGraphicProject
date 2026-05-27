// ComputerGraphicProject.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define GLFW_DLL
#define GLEW_DLL
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979
#endif
void drawEllipse(float cx, float cy, float rx, float ry, int segments = 100)
{
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < segments; ++i)
    {
        float theta = 2.0f * M_PI * float(i)/float(segments);
        float x = cx + rx * cosf(theta);
        float y = cy + ry * sinf(theta);
        glVertex2f(x, y);
    }
    glEnd();
}

int main()
{
    std::cout << "Hello World!\n";
    glfwInit();
    if (!glfwInit())
    {
        fprintf(stderr, "Error: couldn't start GLFW3.\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
   // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
   // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* MyWindow = glfwCreateWindow(512, 512, "MyWindow", NULL, NULL);

    if (!MyWindow)
    {
        glfwTerminate;
        return 1;
    }

    glewExperimental = GL_TRUE;
    glfwMakeContextCurrent(MyWindow);
    
    GLenum ret = glewInit();
    if (GLEW_OK != ret)
    {
        fprintf(stderr, "Error: %s \n", glewGetErrorString(ret));
        return 1;
    }

    while (!glfwWindowShouldClose(MyWindow))
    {
        glfwPollEvents();
        glClearColor(0.3, 1.0, 0.4, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(1.0, 0.2, 1.0);
        drawEllipse(0.0f, 0.0f, 0.5f, 0.25f, 200);
        glEnd();

        glfwSwapBuffers(MyWindow);

    }
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
