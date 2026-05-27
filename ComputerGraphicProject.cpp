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
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

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

    Model(std::string const& path) {
        loadModel(path);
    }

    void Draw(unsigned int shaderProgram) {
        GLint matAmbLoc = glGetUniformLocation(shaderProgram, "material.ambient");
        GLint matDiffLoc = glGetUniformLocation(shaderProgram, "material.diffuse");

        for (unsigned int i = 0; i < meshes.size(); i++) {
            // Красим каждый меш в зависимости от его индекса
            if (i == 0) {
                // Синий куб базы
                glUniform3f(matAmbLoc, 0.0f, 0.05f, 0.3f);
                glUniform3f(matDiffLoc, 0.0f, 0.1f, 0.6f);
            }
            else if (i == 1) {
                // Фиолетовая штука
                glUniform3f(matAmbLoc, 0.15f, 0.05f, 0.25f);
                glUniform3f(matDiffLoc, 0.45f, 0.1f, 0.65f);
            }
            else if (i == 2) {
                // Зелёная штанга
                glUniform3f(matAmbLoc, 0.05f, 0.25f, 0.05f);
                glUniform3f(matDiffLoc, 0.1f, 0.6f, 0.15f);
            }
            else {
                // Оранжевый блок
                glUniform3f(matAmbLoc, 1.0f, 0.5f, 0.0f);
                glUniform3f(matDiffLoc, 1.0f, 0.5f, 0.0f);
            }

            // Отрисовываем текущий меш с уже примененным цветом
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
        "out vec3 FragPos;\n" // Позиция фрагмента для расчета затухания и бликов
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "uniform mat3 normalMatrix;\n" // Нормальная матрица 3x3
        "void main() {\n"
        "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "   Normal = normalMatrix * aNormal;\n" // Корректная трансформация нормали
        "   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
        "}\0";

    const char* frag_shader =
        "#version 330 core\n"
        "out vec4 frag_colour;\n"
        "in vec3 Normal;\n"
        "in vec3 FragPos;\n"
        "struct Material {\n" // Описание свойств материала
        "   vec3 ambient;\n"
        "   vec3 diffuse;\n"
        "   vec3 specular;\n"
        "   float shininess;\n"
        "};\n"
        "struct Light {\n" // Описание свойств источника света
        "   vec3 position;\n"
        "   vec3 ambient;\n"
        "   vec3 diffuse;\n"
        "   vec3 specular;\n"
        "};\n"
        "uniform Material material;\n"
        "uniform Light light;\n"
        "uniform vec3 viewPos;\n" // Позиция камеры в пространстве
        "void main() {\n"
        "   // Фоновое освещение (Ambient, Стр. 7)\n"
        "   vec3 ambient = light.ambient * material.ambient;\n"
        "   // Диффузное освещение (Diffuse, Стр. 7)\n"
        "   vec3 norm = normalize(Normal);\n"
        "   vec3 lightDir = normalize(light.position - FragPos);\n"
        "   float diff = max(dot(norm, lightDir), 0.0);\n"
        "   vec3 diffuse = light.diffuse * (diff * material.diffuse);\n"
        "   // Зеркальные блики (Specular, Стр. 7)\n"
        "   vec3 viewDir = normalize(viewPos - FragPos);\n"
        "   vec3 reflectDir = reflect(-lightDir, norm);\n"
        "   float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
        "   vec3 specular = light.specular * (spec * material.specular);\n"
        "   // Результирующий цвет Фонга (Стр. 7)\n"
        "   vec3 result = ambient + diffuse + specular;\n"
        "   frag_colour = vec4(result, 1.0);\n"
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

    // Локации матриц и базовых переменных
    GLint modelLoc = glGetUniformLocation(shader_program, "model");
    GLint viewLoc = glGetUniformLocation(shader_program, "view");
    GLint projLoc = glGetUniformLocation(shader_program, "projection");
    GLint normMatLoc = glGetUniformLocation(shader_program, "normalMatrix");
    GLint viewPosLoc = glGetUniformLocation(shader_program, "viewPos");

    // Локации структуры Материала
    GLint matAmbLoc = glGetUniformLocation(shader_program, "material.ambient");
    GLint matDiffLoc = glGetUniformLocation(shader_program, "material.diffuse");
    GLint matSpecLoc = glGetUniformLocation(shader_program, "material.specular");
    GLint matShinLoc = glGetUniformLocation(shader_program, "material.shininess");

    // Локации структуры Света
    GLint lightPosLoc = glGetUniformLocation(shader_program, "light.position");
    GLint lightAmbLoc = glGetUniformLocation(shader_program, "light.ambient");
    GLint lightDiffLoc = glGetUniformLocation(shader_program, "light.diffuse");
    GLint lightSpecLoc = glGetUniformLocation(shader_program, "light.specular");

    Model ourModel("Lab3ModelVar27.obj");
    while (!glfwWindowShouldClose(MyWindow))
    {
        glfwPollEvents();
        processInput(MyWindow);

        // Зеленый фон
        glClearColor(0.3f, 1.0f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader_program);

        // 1. Передача настроек материала
        glUniform3f(matAmbLoc, 1.0f, 0.5f, 0.31f);
        glUniform3f(matDiffLoc, 1.0f, 0.5f, 0.31f);
        glUniform3f(matSpecLoc, 0.5f, 0.5f, 0.5f);
        glUniform1f(matShinLoc, 32.0f);

        // 2. Передача настроек интенсивности источника света
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(lightAmbLoc, 0.1f, 0.1f, 0.1f);  // Слабый фоновый свет
        glUniform3f(lightDiffLoc, 0.8f, 0.8f, 0.8f);  // Яркий белый диффузный
        glUniform3f(lightSpecLoc, 1.0f, 1.0f, 1.0f);  // Полный зеркальный свет

        // Передача текущей позиции камеры для расчета вектора взгляда
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);

        // Матрицы трансформаций камеры
        float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Матрица модели манипулятора
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.5f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Расчет и передача Нормальной матрицы
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        glUniformMatrix3fv(normMatLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        ourModel.Draw(shader_program);

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