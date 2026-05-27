#include <iostream>
#include <vector>
#include <string>
#include <cmath>

// Подключение GLM
#include "glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// Подключение графического API
#include <glad/glad.h> 
#include <GLFW/glfw3.h> 

// Подключение библиотек ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Глобальные переменные для FPS-камеры
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 6.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;

// Позиция источника света в мировом пространстве
glm::vec3 lightPos = glm::vec3(2.0f, 4.0f, 3.0f);

void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);

// Инициализация структуры Vertex с нулевыми значениями по умолчанию
struct Vertex {
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Normal = glm::vec3(0.0f);
};

class Mesh {
public:
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) {
        this->vertices = vertices;
        this->indices = indices;
        setupMesh();
    }

    void Draw(unsigned int shaderProgram) {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    unsigned int VBO, EBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        glBindVertexArray(0);
    }
};

class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;

    // Вектор для хранения автоматически вычисленных собственных центров каждого меша
    std::vector<glm::vec3> meshCenters;

    Model(std::string const& path) {
        loadModel(path);
        calculateCenters(); // Вычисляем барицентры суставов сразу после загрузки
    }

    // Автоматический расчет геометрического центра масс для каждого меша
    void calculateCenters() {
        meshCenters.resize(meshes.size(), glm::vec3(0.0f));
        for (size_t i = 0; i < meshes.size(); i++) {
            if (meshes[i].vertices.empty()) continue;

            // Берем координаты самой первой вершины для инициализации границ
            glm::vec3 minBounds = meshes[i].vertices[0].Position;
            glm::vec3 maxBounds = meshes[i].vertices[0].Position;

            for (const auto& vertex : meshes[i].vertices) {
                minBounds = glm::min(minBounds, vertex.Position);
                maxBounds = glm::max(maxBounds, vertex.Position);
            }
            // Собственный центр меша
            meshCenters[i] = (minBounds + maxBounds) * 0.5f;
        }
    }

    void Draw(unsigned int shaderProgram, glm::mat4 baseModel, float joint1, float joint2, float joint3) {
        GLint matAmbLoc = glGetUniformLocation(shaderProgram, "material.ambient");
        GLint matDiffLoc = glGetUniformLocation(shaderProgram, "material.diffuse");
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint normMatLoc = glGetUniformLocation(shaderProgram, "normalMatrix");

        if (meshes.size() < 4) return;

        // Получаем автоматические геометрические центры деталей
        glm::vec3 centerLink1 = meshCenters[1]; // Собственный центр фиолетовой штуки
        glm::vec3 centerLink2 = meshCenters[2]; // Собственный центр зеленой штанги
        glm::vec3 centerLink3 = meshCenters[3]; // Собственный центр оранжевого блока

        glm::vec3 pivotJoint2 = centerLink2 + glm::vec3(0.0f, 0.0f, 1.0f);     // Стык штуки и штанги
        glm::vec3 pivotJoint3 = centerLink3 + glm::vec3(0.0f, 2.0f, 0.0f);     // Стык штанги и блока

        // База
        glm::mat4 mBase = baseModel;

        // Фиолетовая штука
        glm::mat4 mLink1 = glm::rotate(mBase, glm::radians(joint1), glm::vec3(0.0f, 1.0f, 0.0f));

        // Зелёная штанга
        glm::mat4 mLink2 = glm::translate(mLink1, pivotJoint2);
        mLink2 = glm::rotate(mLink2, glm::radians(joint2), glm::vec3(1.0f, 0.0f, 0.0f));
        mLink2 = glm::translate(mLink2, -pivotJoint2);

        // Оранжевый блок
        glm::mat4 mLink3 = glm::translate(mLink2, pivotJoint3);
        mLink3 = glm::rotate(mLink3, glm::radians(joint3), glm::vec3(0.0f, 0.0f, 1.0f));
        mLink3 = glm::translate(mLink3, -pivotJoint3);

        // Отрисовка мешей с распределением индивидуальных матриц и цветов
        for (unsigned int i = 0; i < meshes.size(); i++) {
            glm::mat4 currentModel = glm::mat4(1.0f);
          
            switch (i) {
            case 0: // Синий куб базы
                glUniform3f(matAmbLoc, 0.0f, 0.05f, 0.3f);
                glUniform3f(matDiffLoc, 0.0f, 0.1f, 0.6f);
                currentModel = mBase;
                break;

            case 1: // Фиолетовая штука
                glUniform3f(matAmbLoc, 0.15f, 0.05f, 0.25f);
                glUniform3f(matDiffLoc, 0.45f, 0.1f, 0.65f);
                currentModel = mLink1;
                break;

            case 2: // Зеленая штанга
                glUniform3f(matAmbLoc, 0.05f, 0.25f, 0.05f);
                glUniform3f(matDiffLoc, 0.1f, 0.6f, 0.15f);
                currentModel = mLink2;
                break;

            case 3: // Оранжевый блок
            default:
                glUniform3f(matAmbLoc, 1.0f, 0.5f, 0.0f);
                glUniform3f(matDiffLoc, 1.0f, 0.5f, 0.0f);
                currentModel = mLink3;
                break;
            }

            // Отправка матриц и нормалей текущего меша в шейдерную программу
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(currentModel));
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(currentModel)));
            glUniformMatrix3fv(normMatLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

            meshes[i].Draw(shaderProgram);
        }
    }

private:
    void loadModel(std::string const& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            glm::vec3 vector;

            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            if (mesh->HasNormals()) {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }
        return Mesh(vertices, indices);
    }
};

int main()
{
    if (!glfwInit()) {
        fprintf(stderr, "Error: couldn't start GLFW3.\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* MyWindow = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MyWindow", NULL, NULL);
    if (!MyWindow) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(MyWindow);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Error initializing GLAD\n");
        glfwTerminate();
        return 1;
    }

    glfwSetInputMode(MyWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(MyWindow, mouse_callback);
    glEnable(GL_DEPTH_TEST);

    const char* vert_shader =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "out vec3 Normal;\n"
        "out vec3 FragPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "uniform mat3 normalMatrix;\n"
        "void main() {\n"
        "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "   Normal = normalMatrix * aNormal;\n"
        "   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
        "}\0";

    const char* frag_shader =
        "#version 330 core\n"
        "out vec4 frag_colour;\n"
        "in vec3 Normal;\n"
        "in vec3 FragPos;\n"
        "struct Material {\n"
        "    vec3 ambient;\n"
        "    vec3 diffuse;\n"
        "    vec3 specular;\n"
        "    float shininess;\n"
        "};\n"
        "struct Light {\n"
        "    vec3 position;\n"
        "    vec3 ambient;\n"
        "    vec3 diffuse;\n"
        "    vec3 specular;\n"
        "};\n"
        "uniform Material material;\n"
        "uniform Light light;\n"
        "uniform vec3 viewPos;\n"
        "void main() {\n"
        "    vec3 ambient = light.ambient * material.ambient;\n"
        "    vec3 norm = normalize(Normal);\n"
        "    vec3 lightDir = normalize(light.position - FragPos);\n"
        "    float diff = max(dot(norm, lightDir), 0.0);\n"
        "    vec3 diffuse = light.diffuse * (diff * material.diffuse);\n"
        "    vec3 viewDir = normalize(viewPos - FragPos);\n"
        "    vec3 reflectDir = reflect(-lightDir, norm);\n"
        "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
        "    vec3 specular = light.specular * (spec * material.specular);\n"
        "    vec3 result = ambient + diffuse + specular;\n"
        "    frag_colour = vec4(result, 1.0);\n"
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

GLint modelLoc = glGetUniformLocation(shader_program, "model");
GLint viewLoc = glGetUniformLocation(shader_program, "view");
GLint projLoc = glGetUniformLocation(shader_program, "projection");
GLint normMatLoc = glGetUniformLocation(shader_program, "normalMatrix");
GLint viewPosLoc = glGetUniformLocation(shader_program, "viewPos");
GLint matSpecLoc = glGetUniformLocation(shader_program, "material.specular");
GLint matShinLoc = glGetUniformLocation(shader_program, "material.shininess");
GLint lightPosLoc = glGetUniformLocation(shader_program, "light.position");
GLint lightAmbLoc = glGetUniformLocation(shader_program, "light.ambient");
GLint lightDiffLoc = glGetUniformLocation(shader_program, "light.diffuse");
GLint lightSpecLoc = glGetUniformLocation(shader_program, "light.specular");

Model ourModel("Lab3ModelVar27.obj");

// Инициализация переменных для анимации суставов
float robotJoint1 = 0.0f;
float robotJoint2 = 0.0f;
float robotJoint3 = 0.0f;

// Инициализация таймеров для плавного движения
float deltaTime = 0.0f;
float lastFrame = 0.0f;

while (!glfwWindowShouldClose(MyWindow))
{
    // Вычисление независимой скорости кадра deltaTime
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    glfwPollEvents();
    processInput(MyWindow);

    // Движение звеньев робота по клавишам 1-2, 3-4, 5-6
    if (glfwGetKey(MyWindow, GLFW_KEY_1) == GLFW_PRESS) robotJoint1 += deltaTime * 40.0f;
    if (glfwGetKey(MyWindow, GLFW_KEY_2) == GLFW_PRESS) robotJoint1 -= deltaTime * 40.0f;
    if (glfwGetKey(MyWindow, GLFW_KEY_3) == GLFW_PRESS) robotJoint2 += deltaTime * 40.0f;
    if (glfwGetKey(MyWindow, GLFW_KEY_4) == GLFW_PRESS) robotJoint2 -= deltaTime * 40.0f;
    if (glfwGetKey(MyWindow, GLFW_KEY_5) == GLFW_PRESS) robotJoint3 += deltaTime * 40.0f;
    if (glfwGetKey(MyWindow, GLFW_KEY_6) == GLFW_PRESS) robotJoint3 -= deltaTime * 40.0f;

    // Дополнительные проверки на крайние смещения по значению
    if (robotJoint2 > 85.0f)  robotJoint2 = 85.0f;
    if (robotJoint2 < -63.0f) robotJoint2 = -63.0f;

    // Зеленый фон
    glClearColor(0.3f, 1.0f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_program);
    glUniform3f(matSpecLoc, 0.5f, 0.5f, 0.5f);
    glUniform1f(matShinLoc, 32.0f);
    glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(lightAmbLoc, 0.1f, 0.1f, 0.1f);
    glUniform3f(lightDiffLoc, 0.8f, 0.8f, 0.8f);
    glUniform3f(lightSpecLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);

    float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Сброс и создание базовой единичной матрицы
    glm::mat4 baseModel = glm::mat4(1.0f);

    // Вызов иерархической отрисовки с углами поворота суставов
    ourModel.Draw(shader_program, baseModel, robotJoint1, robotJoint2, robotJoint3);

    glfwSwapBuffers(MyWindow);
}

glDeleteProgram(shader_program);
glfwTerminate();
return 0;
}

void processInput(GLFWwindow* window)
{
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

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
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

    if (pitch > 89.0f)
        pitch = 89.0f;

    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
}