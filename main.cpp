// Вращающийся треугольник с текстурой (можно вращать стрелочками)

#include <gl/glew.h>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <random>
#include <functional>

// В C и C++ есть оператор #, который позволяет превращать параметры макроса в строку
#define TO_STRING(x) #x

//================================================================================================================================
// ========================================================= ↓СТРУКТУРЫ↓ ==========================================================
struct ShaderInformation {
    GLuint shaderProgram;

    GLint attribVertex;
    GLint attribTexture;
    GLint attribNormale;

    GLint unifTexture;

    GLint unifCameraPosition;

    GLint unifProjection;
    GLint unifView;
    GLint unifModel;
    GLint unifnormaleRotation;

    GLint unifLightDirection;
    GLint unifLightAmbient;
    GLint unifLightDiffuse;
    GLint unifLightSpecular;
    GLint unifLightAttenuation;

    GLint unifSpotPosition;
    GLint unifSpotDirection;
    GLint unifSpotLight;
    GLint unifSpotAttenuation;
    GLint unifSpotCutOffCos;

    GLint unifMaterialAmbient;
    GLint unifMaterialDiffuse;
    GLint unifMaterialSpecular;
    GLint unifMaterialEmission;
    GLint unifMaterialShininess;
};

struct Light {
   glm::vec3 lightDirection;
   glm::vec4 lightAmbient;
   glm::vec4 lightDiffuse;
   glm::vec4 lightSpecular;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    GLfloat cutOffCos;
    glm::vec3 attenuation;
    glm::vec3 light;
};

struct Material {
    glm::vec4 materialAmbient;
    glm::vec4 materialDiffuse;
    glm::vec4 materialSpecular;
    glm::vec4 materialEmission;
    GLfloat materialshininess;
};

struct GameObject {
    // Количество вершин в буферах
    GLuint buffer_size;
    // ID буфера вершин
    GLuint vertexVAO;
    // ID текстуры
    sf::Texture textureData;
    //Модельная матрица для объекта (сюда пихаем все сдвиги и тд и тп)
    glm::mat4 modelMatrix;
    //Матрица для поворотов нормалей
    glm::mat4 normaleRotationMatrix;
    //Всякие коэффициенты для материала
    Material material;
};

struct Hurdle : GameObject {
    float xShift;
    float zShift;
    float radius;

    Hurdle(GLuint buffer_size, GLuint vertexVAO, sf::Texture textureData, glm::mat4 modelMatrix, glm::mat4 normaleRotationMatrix, Material material, float zShift, float radius) {
        this->buffer_size = buffer_size;
        this->vertexVAO = vertexVAO;
        this->textureData = textureData;
        this->modelMatrix = modelMatrix;
        this->normaleRotationMatrix = normaleRotationMatrix;
        this->material = material;
        this->zShift = zShift;
        this->radius = radius;
    }

    Hurdle(){}
};
// ========================================================= ↑СТРУКТУРЫ↑ ==========================================================
//================================================================================================================================
// ================================================= ↓ОБЪЯВЛЕНИЕ ВСЯКОГО РАЗНОГО↓ =================================================
GameObject car;
std::vector <GameObject> road;
std::vector <GameObject> grass;
std::vector <GameObject> leftlights;
std::vector <GameObject> rightlights;
GameObject background;
Hurdle moose;
Hurdle cone;
std::deque<Hurdle> currentHurdles;

SpotLight spotLight;
Light light;
bool isLightOn = true;
int lightEmergencyModeTime = 0;

glm::vec3 cameraPosition = glm::vec3(0.0f, -15.0f, -35.0f);

std::default_random_engine generator;
std::uniform_int_distribution<int> distribution(1, 6);

int carYawState = 0;
float carYawAngle = 0.0f;
float carYawPosition = 0.0f;
const float carRadius = 3.0f;

glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

ShaderInformation shaderInformation;
glm::mat4 identityMatrix = glm::mat4(1.0f);
// Массив VBO что бы их потом удалить
std::vector <GLuint> VBOArray;
// ================================================= ↑ОБЪЯВЛЕНИЕ ВСЯКОГО РАЗНОГО↑ =================================================
//================================================================================================================================
// ================================================= ↓ВСПОМОГАТЕЛЬНЫЕ ПРИКОЛЮХИ↓ ==================================================
std::vector<std::string> split(const std::string& s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
        // elems.push_back(std::move(item)); // if C++11 (based on comment from @mchiasson)
    }
    return elems;
}

float clamp(float num, float left, float right) {
    if (num < left) {
        return left;
    }
    if (num > right) {
        return right;
    }
    return num;
}

void moveLightDirection(float x, float y, float z) {
    light.lightDirection = glm::vec3(clamp(light.lightDirection.x + x, -1.0f, 1.0f), clamp(light.lightDirection.y + y, 0.0f, 1.0f), clamp(light.lightDirection.z + z, -1.0f, 1.0f));
    std::cout << "Light direction: (" << light.lightDirection.x << ", " << light.lightDirection.y << ", " << light.lightDirection.z << ")" << std::endl;
}

void switchLight() {
    isLightOn = !isLightOn;
    if (isLightOn) {
        light.lightDiffuse = glm::vec4(1.0, 1.0, 1.0, 1.0);
        light.lightSpecular = glm::vec4(1.0, 1.0, 1.0, 1.0);
    }
    else {
        light.lightDiffuse = glm::vec4(0.0, 0.0, 0.0, 1.0);
        light.lightSpecular = glm::vec4(0.0, 0.0, 0.0, 1.0);
    }
}
// ================================================= ↑ВСПОМОГАТЕЛЬНЫЕ ПРИКОЛЮХИ↑ =================================================
//================================================================================================================================
// ========================================================= ↓ШЕЙДЕРЫ↓ ===========================================================
const char* VertexShaderSource = TO_STRING(
    #version 330 core\n

    uniform mat4 modelMatrix;
    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;
    uniform mat4 normaleRotationMatrix;

    uniform vec3 cameraPosition;   

    uniform vec3 spotPosition;

    in vec3 vertexPosition;
    in vec3 vertexNormale;
    in vec2 vertexTextureCoords;

    out vec2 vTextureCoordinate;
    out vec3 vNormale;
    out vec3 viewDirection;
    out vec3 spotLightDirection;
    out float spotDistance;

    void main() {
        // Переведём координаты в мировые
        vec4 worldVertexPosition = modelMatrix * vec4(vertexPosition, 1.0);
        // Пробрасываем текстурные
        vTextureCoordinate = vec2(vertexTextureCoords.x, 1.0 - vertexTextureCoords.y);
        // Преобразуем и пробрасываем нормаль
        vec4 transNormale = normaleRotationMatrix * vec4(vertexNormale,0.0);
        vNormale = normalize(vec3(transNormale));
        // Задаём позишн
        gl_Position = projectionMatrix * viewMatrix * worldVertexPosition;
        // Пробрасываем вектор в глаз
        viewDirection = normalize(cameraPosition - vec3(worldVertexPosition));
        spotLightDirection = normalize(spotPosition - vec3(worldVertexPosition));
        spotDistance = length(spotPosition - vec3(worldVertexPosition));
    }
);

const char* FragShaderSource = TO_STRING(
    #version 330 core\n

    uniform vec3 lightDirection;
    uniform vec4 lightAmbient;
    uniform vec4 lightDiffuse;
    uniform vec4 lightSpecular;

    uniform vec3 spotDirection;
    uniform vec3 spotLight;
    uniform vec3 spotAttenuation;
    uniform float spotCutOffCos;

    uniform sampler2D textureData;
    uniform vec4 materialAmbient;
    uniform vec4 materialDiffuse;
    uniform vec4 materialSpecular;
    uniform vec4 materialEmission;
    uniform float materialShininess;

    in vec2 vTextureCoordinate;
    in vec3 vNormale;
    in vec3 viewDirection;
    in vec3 spotLightDirection;
    in float spotDistance;

    out vec4 color;

    void main() {
        float spotAngle = dot(normalize(spotLightDirection),normalize(-spotDirection));
        float attenuation = 1.0 / (spotAttenuation[0] + spotAttenuation[1] * spotDistance + spotAttenuation[2] * spotDistance * spotDistance);
        // Собственное свечение объекта
        color = materialEmission;
        // Вкладываем эмбиент
        color += materialAmbient * lightAmbient;
        // Считаем диффузное освещение через скаляр векторов
        float lightAngle = max(dot(vNormale, lightDirection), 0.0);
        color += materialDiffuse * lightDiffuse * lightAngle;
        if (spotAngle > spotCutOffCos) {
            float spotLightAngle = max(dot(vNormale, normalize(spotLightDirection)), 0.0);
            color += materialDiffuse * vec4(spotLight,1.0) * spotLightAngle * attenuation;
        }
        // Считаем блики той самой формулой
        float specularAngle = max(pow(dot(reflect(-lightDirection, vNormale), viewDirection), materialShininess), 0.0);
        color += materialSpecular * lightSpecular * specularAngle;
        // Смешиваем полученное с текстурой
        color *= texture(textureData, vTextureCoordinate);
        
    }
);

// ========================================================= ↑ШЕЙДЕРЫ↑ ===========================================================
//================================================================================================================================
// ==================================================== ↓РАБОТА С ФАЙЛАМИ↓ =======================================================
std::vector<GLfloat> parseFile(std::string fileName) {
    std::vector<GLfloat> vertices{};
    std::ifstream obj(fileName);

    if (!obj.is_open()) {
        throw std::exception("File cannot be opened");
    }

    std::vector<std::vector<float>> v{};
    std::vector<std::vector<float>> vt{};
    std::vector<std::vector<float>> vn{};

    std::vector <std::string> indexAccordance{};
    std::string line;
    while (std::getline(obj, line))
    {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "v") {
            auto vertex = split(line, ' ');
            std::vector<float> cv{};
            for (size_t j = 1; j < vertex.size(); j++)
            {
                cv.push_back(std::stof(vertex[j]));
            }
            v.push_back(cv);
        }
        else if (type == "vn") {
            auto normale = split(line, ' ');
            std::vector<float> cvn{};
            for (size_t j = 1; j < normale.size(); j++)
            {
                cvn.push_back(std::stof(normale[j]));
            }
            vn.push_back(cvn);
        }
        else if (type == "vt") {
            auto texture = split(line, ' ');
            std::vector<float> cvt{};
            for (size_t j = 1; j < texture.size(); j++)
            {
                cvt.push_back(std::stof(texture[j]));
            }
            vt.push_back(cvt);
        }
        else if (type == "f") {
            auto splitted = split(line, ' ');
            for (size_t i = 1; i < splitted.size(); i++)
            {
                auto triplet = split(splitted[i], '/');
                int positionIndex = std::stoi(triplet[0]) - 1;
                for (int j = 0; j < 3; j++) {
                    vertices.push_back(v[positionIndex][j]);
                }
                int normaleIndex = std::stoi(triplet[2]) - 1;
                for (int j = 0; j < 3; j++) {
                    vertices.push_back(vn[normaleIndex][j]);
                }
                int textureIndex = std::stoi(triplet[1]) - 1;
                for (int j = 0; j < 2; j++) {
                    vertices.push_back(vt[textureIndex][j]);
                }
            }
        }
    }
    return vertices;
}

void loadTexture(sf::Texture& textureData, const char* filename)
{
    if (!textureData.loadFromFile(filename))
    {
        std::cout << "could not load texture" << std::endl;
    }
}
// ==================================================== ↑РАБОТА С ФАЙЛАМИ↑ =======================================================
//================================================================================================================================
// =========================================================== ↓ЛОГИ↓ ============================================================
// Проверка ошибок OpenGL, если есть то вывод в консоль тип ошибки
void checkOpenGLerror() {
    GLenum errCode;
    // Коды ошибок можно смотреть тут
    // https://www.khronos.org/opengl/wiki/OpenGL_Error
    if ((errCode = glGetError()) != GL_NO_ERROR)
        std::cout << "OpenGl error!: " << errCode << std::endl;
}

// Функция печати лога шейдера
void ShaderLog(unsigned int shader)
{
    int infologLen = 0;
    int charsWritten = 0;
    char* infoLog;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1)
    {
        infoLog = new char[infologLen];
        if (infoLog == NULL)
        {
            std::cout << "ERROR: Could not allocate InfoLog buffer" << std::endl;
            exit(1);
        }
        glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
        std::cout << "InfoLog: " << infoLog << "\n\n\n";
        delete[] infoLog;
    }
}
// =========================================================== ↑ЛОГИ↑ ============================================================
//================================================================================================================================
// ======================================================= ↓ИНИЦИАЛИЗАЦИЯ↓ =======================================================
GameObject createObject(std::vector<GLfloat> vertices, const char* textureFile, glm::mat4 modelMatrix, glm::mat3 normaleRotationMatrix, Material material) {
    GLuint VAO;
    GLuint VBO;
    sf::Texture textureData;
    loadTexture(textureData, textureFile);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    VBOArray.push_back(VAO);
    VBOArray.push_back(VBO);

    glBindVertexArray(VAO);

    glEnableVertexAttribArray(shaderInformation.attribVertex);
    glEnableVertexAttribArray(shaderInformation.attribNormale);
    glEnableVertexAttribArray(shaderInformation.attribTexture);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

    // Атрибут с координатами
    glVertexAttribPointer(shaderInformation.attribVertex, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    // Атрибут с цветом
    glVertexAttribPointer(shaderInformation.attribNormale, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    // Атрибут с текстурой
    glVertexAttribPointer(shaderInformation.attribTexture, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));

    glBindVertexArray(0);
    glDisableVertexAttribArray(shaderInformation.attribVertex);
    glDisableVertexAttribArray(shaderInformation.attribNormale);
    glDisableVertexAttribArray(shaderInformation.attribTexture);
    checkOpenGLerror();

    return GameObject{
                vertices.size(),
                VAO,
                textureData,
                modelMatrix,
                normaleRotationMatrix,
                material
    };
}

Hurdle createHurdle(const char* objFile, const char* textureFile, glm::mat4 modelMatrix, glm::mat4 normaleRotationMatrix, Material material, float radius) {
    GLuint VAO;
    GLuint VBO;
    auto vertices = parseFile(objFile);
    sf::Texture textureData;
    loadTexture(textureData, textureFile);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    VBOArray.push_back(VAO);
    VBOArray.push_back(VBO);

    glBindVertexArray(VAO);

    glEnableVertexAttribArray(shaderInformation.attribVertex);
    glEnableVertexAttribArray(shaderInformation.attribNormale);
    glEnableVertexAttribArray(shaderInformation.attribTexture);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

    // Атрибут с координатами
    glVertexAttribPointer(shaderInformation.attribVertex, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    // Атрибут с цветом
    glVertexAttribPointer(shaderInformation.attribNormale, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    // Атрибут с текстурой
    glVertexAttribPointer(shaderInformation.attribTexture, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));

    glBindVertexArray(0);
    glDisableVertexAttribArray(shaderInformation.attribVertex);
    glDisableVertexAttribArray(shaderInformation.attribNormale);
    glDisableVertexAttribArray(shaderInformation.attribTexture);
    checkOpenGLerror();
    
    return Hurdle{
                vertices.size(),
                VAO,
                textureData,
                modelMatrix,
                normaleRotationMatrix,
                material,
                0.0f,
                radius
    };
}

void InitObjects()
{
    Material carMaterial = Material{
    glm::vec4(0.2,0.2,0.2,1.0),     //Ambient
    glm::vec4(0.8,0.8,0.8,1.0),     //Diffuse
    glm::vec4(0.3,0.3,0.3,1.0),     //Specular
    glm::vec4(0.0,0.0,0.0,1.0),     //Emission
    10.0f,                          //shininess
    };

    Material asphaltMaterial = Material{
    glm::vec4(0.1,0.1,0.1,1.0),
    glm::vec4(0.8,0.8,0.8,1.0),
    glm::vec4(0.5,0.5,0.5,1.0),
    glm::vec4(0.0,0.0,0.0,1.0),
    2.0f,
    };

    Material grassMaterial = Material{
    glm::vec4(0.1,0.1,0.1,1.0),
    glm::vec4(0.9,0.9,0.9,1.0),
    glm::vec4(0.1,0.1,0.1,1.0),
    glm::vec4(0.0,0.0,0.0,1.0),
    1.0f,
    };

    Material lightMaterial = Material{
    glm::vec4(0.0,0.0,0.0,1.0),     //Ambient
    glm::vec4(0.0,0.0,0.0,1.0),     //Diffuse
    glm::vec4(0.0,0.0,0.0,1.0),     //Specular
    glm::vec4(1.0,1.0,1.0,1.0),     //Emission
    1.0f,                          //shininess
    };

    auto roadVerts = parseFile(".\\objects\\road.obj");
    for (int i = 0; i < 3; i++) {
        road.push_back(createObject(roadVerts, ".\\objects\\road.png", glm::translate(identityMatrix, glm::vec3(0.0f, 0.0f, -50.0f - 200.0f * i)), identityMatrix, asphaltMaterial));
    }

    auto grassVerts = parseFile(".\\objects\\bettergrass.obj");
    for (int i = 0; i < 3; i++) {
        grass.push_back(createObject(grassVerts, ".\\objects\\field2.png", glm::translate(identityMatrix, glm::vec3(100.0f, 0.0f, -50.0f - 200.0f * i)), identityMatrix, grassMaterial));
    }

    for (int i = 0; i < 3; i++) {
        grass.push_back(createObject(grassVerts, ".\\objects\\field2.png", glm::translate(identityMatrix, glm::vec3(-100.0f, 0.0f, -50.0f - 200.0f * i)), identityMatrix, grassMaterial));
    }
    
    auto lightVerts = parseFile(".\\objects\\light.obj");
    for (int i = 0; i < 3; i++) {
        for (int i = 0; i <= 9; i++) {
            rightlights.push_back(createObject(lightVerts, (".\\objects\\light" + std::to_string(i) + ".png").c_str(), glm::translate(identityMatrix, glm::vec3(15.0f, 5.0f, 50 - 20.0f * (i-1))), identityMatrix, lightMaterial));
            leftlights.push_back(createObject(lightVerts, (".\\objects\\light" + std::to_string(i) + ".png").c_str(), glm::translate(identityMatrix, glm::vec3(15.0f, 5.0f, 50 - 20.0f * (i - 1))), identityMatrix, lightMaterial));
        }
    }
    background = createObject(parseFile(".\\objects\\background.obj"), ".\\objects\\background.png", glm::translate(identityMatrix, glm::vec3(0.0f, -10.0f, -199.0f)), identityMatrix, lightMaterial);
    car = createObject(parseFile(".\\objects\\bus2.obj"), ".\\objects\\bus2.png", glm::translate(identityMatrix, glm::vec3(0.0f, 0.0f, 0.0f)), identityMatrix, carMaterial);
    moose = createHurdle(".\\objects\\los.obj", ".\\objects\\los.png", glm::translate(identityMatrix, glm::vec3(0.0f, 0.0f, 0.0f)), identityMatrix, carMaterial, 3.0f);
    cone = createHurdle(".\\objects\\cone.obj", ".\\objects\\cone.png", glm::translate(identityMatrix, glm::vec3(0.0f, 0.0f, 0.0f)), identityMatrix, carMaterial, 1.0f);
}


void InitShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderSource, NULL);
    glCompileShader(vShader);
    std::cout << "vertex shader \n";
    ShaderLog(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    glCompileShader(fShader);
    std::cout << "fragment shader \n";
    ShaderLog(fShader);

    shaderInformation.shaderProgram = glCreateProgram();
    glAttachShader(shaderInformation.shaderProgram, vShader);
    glAttachShader(shaderInformation.shaderProgram, fShader);

    glLinkProgram(shaderInformation.shaderProgram);
    int link_status;
    glGetProgramiv(shaderInformation.shaderProgram, GL_LINK_STATUS, &link_status);
    if (!link_status)
    {
        std::cout << "error attach shaders \n";
        return;
    }

    shaderInformation.attribVertex =
        glGetAttribLocation(shaderInformation.shaderProgram, "vertexPosition");
    if (shaderInformation.attribVertex == -1)
    {
        std::cout << "could not bind attrib vertCoord" << std::endl;
        return;
    }

    shaderInformation.attribNormale =
        glGetAttribLocation(shaderInformation.shaderProgram, "vertexNormale");
    if (shaderInformation.attribNormale == -1)
    {
        std::cout << "could not bind attrib normale" << std::endl;
        return;
    }

    shaderInformation.attribTexture =
        glGetAttribLocation(shaderInformation.shaderProgram, "vertexTextureCoords");
    if (shaderInformation.attribTexture == -1)
    {
        std::cout << "could not bind attrib texureCoord" << std::endl;
        return;
    }

    shaderInformation.unifTexture =
        glGetUniformLocation(shaderInformation.shaderProgram, "textureData");
    if (shaderInformation.unifTexture == -1)
    {
        std::cout << "could not bind uniform textureData" << std::endl;
        return;
    }

    shaderInformation.unifModel = glGetUniformLocation(shaderInformation.shaderProgram, "modelMatrix");
    if (shaderInformation.unifModel == -1)
    {
        std::cout << "could not bind uniform view" << std::endl;
        return;
    }
    
    shaderInformation.unifView = glGetUniformLocation(shaderInformation.shaderProgram, "viewMatrix");
    if (shaderInformation.unifView == -1)
    {
        std::cout << "could not bind uniform view" << std::endl;
        return;
    }

    shaderInformation.unifProjection = glGetUniformLocation(shaderInformation.shaderProgram, "projectionMatrix");
    if (shaderInformation.unifProjection == -1)
    {
        std::cout << "could not bind uniform projection" << std::endl;
        return;
    }

    shaderInformation.unifnormaleRotation = glGetUniformLocation(shaderInformation.shaderProgram, "normaleRotationMatrix");
    if (shaderInformation.unifnormaleRotation == -1)
    {
        std::cout << "could not bind uniform normaleRotation" << std::endl;
        return;
    }

    shaderInformation.unifCameraPosition = glGetUniformLocation(shaderInformation.shaderProgram, "cameraPosition");
    if (shaderInformation.unifCameraPosition == -1)
    {
        std::cout << "could not bind uniform unifCameraPosition" << std::endl;
        return;
    }

    shaderInformation.unifLightAmbient = glGetUniformLocation(shaderInformation.shaderProgram, "lightAmbient");
    if (shaderInformation.unifLightAmbient == -1)
    {
        std::cout << "could not bind uniform unifLightAmbient" << std::endl;
        return;
    }
    
    shaderInformation.unifLightDiffuse = glGetUniformLocation(shaderInformation.shaderProgram, "lightDiffuse");
    if (shaderInformation.unifLightDiffuse == -1)
    {
        std::cout << "could not bind uniform unifLightDiffuse" << std::endl;
        return;
    }

    shaderInformation.unifLightDirection = glGetUniformLocation(shaderInformation.shaderProgram, "lightDirection");
    if (shaderInformation.unifLightDirection == -1)
    {
        std::cout << "could not bind uniform unifLightDirection" << std::endl;
        return;
    }

    shaderInformation.unifLightSpecular = glGetUniformLocation(shaderInformation.shaderProgram, "lightSpecular");
    if (shaderInformation.unifLightSpecular == -1)
    {
        std::cout << "could not bind uniform unifLightSpecular" << std::endl;
        return;
    }

    shaderInformation.unifMaterialAmbient = glGetUniformLocation(shaderInformation.shaderProgram, "materialAmbient");
    if (shaderInformation.unifMaterialAmbient == -1)
    {
        std::cout << "could not bind uniform unifMaterialAmbient" << std::endl;
        return;
    }

    shaderInformation.unifMaterialDiffuse = glGetUniformLocation(shaderInformation.shaderProgram, "materialDiffuse");
    if (shaderInformation.unifMaterialDiffuse == -1)
    {
        std::cout << "could not bind uniform unifMaterialDiffuse" << std::endl;
        return;
    }

    shaderInformation.unifMaterialEmission = glGetUniformLocation(shaderInformation.shaderProgram, "materialEmission");
    if (shaderInformation.unifMaterialEmission == -1)
    {
        std::cout << "could not bind uniform unifMaterialEmission" << std::endl;
        return;
    }

    shaderInformation.unifMaterialShininess = glGetUniformLocation(shaderInformation.shaderProgram, "materialShininess");
    if (shaderInformation.unifMaterialShininess == -1)
    {
        std::cout << "could not bind uniform materialShininess" << std::endl;
        return;
    }

    shaderInformation.unifMaterialSpecular = glGetUniformLocation(shaderInformation.shaderProgram, "materialSpecular");
    if (shaderInformation.unifMaterialSpecular == -1)
    {
        std::cout << "could not bind uniform unifMaterialSpecular" << std::endl;
        return;
    }

    shaderInformation.unifSpotAttenuation = glGetUniformLocation(shaderInformation.shaderProgram, "spotAttenuation");
    if (shaderInformation.unifSpotAttenuation == -1)
    {
        std::cout << "could not bind uniform unifSpotAttenuation" << std::endl;
        return;
    }

    shaderInformation.unifSpotDirection = glGetUniformLocation(shaderInformation.shaderProgram, "spotDirection");
    if (shaderInformation.unifSpotDirection == -1)
    {
        std::cout << "could not bind uniform " << std::endl;
        return;
    }

    shaderInformation.unifSpotLight = glGetUniformLocation(shaderInformation.shaderProgram, "spotLight");
    if (shaderInformation.unifSpotLight == -1)
    {
        std::cout << "could not bind uniform unifSpotLight" << std::endl;
        return;
    }

    shaderInformation.unifSpotPosition = glGetUniformLocation(shaderInformation.shaderProgram, "spotPosition");
    if (shaderInformation.unifSpotPosition == -1)
    {
        std::cout << "could not bind uniform unifSpotPosition" << std::endl;
        return;
    }

    shaderInformation.unifSpotCutOffCos = glGetUniformLocation(shaderInformation.shaderProgram, "spotCutOffCos");
    if (shaderInformation.unifSpotCutOffCos == -1)
    {
        std::cout << "could not bind uniform unifSpotCutOffCos" << std::endl;
        return;
    }

    checkOpenGLerror();
}

void initCamera() {
    viewMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::mat4(1.0f);
    viewMatrix = glm::translate(viewMatrix, cameraPosition);
    projectionMatrix = glm::perspective(45.0f, (GLfloat)800.0f / (GLfloat)600.0f, 0.1f, 300.0f);
}

void initLight() {
    light = Light{ 
        glm::vec3(0.0,1.0,0.0),
        glm::vec4(1.0, 1.0, 1.0, 1.0),
        glm::vec4(1.0, 1.0, 1.0, 1.0),
        glm::vec4(1.0, 1.0, 1.0, 1.0)
    };
    spotLight = SpotLight{
        glm::vec3(0.0,10.0,0.0),
        glm::vec3(0.0,-1.0,0.0),
        glm::cos(glm::radians(60.0f)),
        glm::vec3(1.0,0.09,0.032),
        glm::vec3(1.0,1.0,1.0)
    };
}

void Init() {
    InitShader();
    InitObjects();
    initCamera();
    initLight();
}
// ======================================================= ↑ИНИЦИАЛИЗАЦИЯ↑ =======================================================
//================================================================================================================================
// ========================================================= ↓ОТРИСОВКА↓ =========================================================

void enableEmergencyMode() {
    currentHurdles.clear();
    carYawAngle = 0.0;
    carYawPosition = 0.0;
    lightEmergencyModeTime = 100;
    background.material.materialEmission = glm::vec4(1.0, 0.0, 0.0, 1.0);
    light.lightDiffuse = glm::vec4(1.0, 0.0, 0.0, 1.0);
    light.lightSpecular = glm::vec4(1.0, 0.0, 0.0, 1.0);
}

void disableEmergencyMode() {
    lightEmergencyModeTime = 0;
    background.material.materialEmission = glm::vec4(1.0, 1.0, 1.0, 1.0);
    light.lightDiffuse = glm::vec4(1.0, 1.0, 1.0, 1.0);
    light.lightSpecular = glm::vec4(1.0, 1.0, 1.0, 1.0);
}

// Обработка шага игрового цикла
void GameTick(int tick) {
    if (lightEmergencyModeTime == 1) {
        disableEmergencyMode();
    }
    else if (lightEmergencyModeTime > 0) {
        lightEmergencyModeTime--;
    }

    if (tick % 100 == 0) {
        auto dice = distribution(generator);
        if (dice <= 2) {
            Hurdle hurdle = moose;
            int shift = (distribution(generator) / 3) - 1;
            hurdle.xShift = shift * 10.0f;
            hurdle.zShift = -300.0f;
            hurdle.modelMatrix = glm::translate(identityMatrix, glm::vec3(hurdle.xShift, 0.0f, hurdle.zShift));
            currentHurdles.push_back(hurdle);
        }
        if (dice >= 5) {
            Hurdle hurdle = cone;
            int shift = (distribution(generator) / 3) - 1;
            hurdle.xShift = shift * 10.0f;
            hurdle.zShift = -300.0f;
            hurdle.modelMatrix = glm::translate(identityMatrix, glm::vec3(hurdle.xShift, 0.0f, hurdle.zShift));
            currentHurdles.push_back(hurdle);
        }
    }
    for (auto& hurdle : currentHurdles) {
        hurdle.zShift += 1.0f;
        hurdle.modelMatrix = glm::translate(identityMatrix, glm::vec3(hurdle.xShift, 0.0f, hurdle.zShift));
    }
    if (!currentHurdles.empty()) {
        auto hurdle = currentHurdles.front();
        if (hurdle.zShift > 20) {
            currentHurdles.pop_front();
        }
        glm::vec3 hurdleVector = glm::vec3(hurdle.xShift, 0.0f, hurdle.zShift) - glm::vec3(carYawPosition,0.0f,-5.69f);
        if ((glm::length(hurdleVector) < (hurdle.radius + carRadius))) {
            enableEmergencyMode();
        }
    }
    for (int i = 0; i < 3; ++i) {
        road[i].modelMatrix = glm::translate(identityMatrix, glm::vec3(0.0f, 0.0f, -50.0f - i * 200.0f + (tick % 200)));
        grass[i].modelMatrix = glm::translate(identityMatrix, glm::vec3(100.0f, 0.0f, -50.0f - i * 200.0f + (tick % 200)));
        grass[i+3].modelMatrix = glm::translate(identityMatrix, glm::vec3(-110.0f, 0.0f, -50.0f - i * 200.0f + (tick % 200)));
        for (int j = 0; j < 10; j++) {
            rightlights[i*10 + j].modelMatrix = glm::translate(identityMatrix, glm::vec3(15.0f, 5.0f, (tick % 200)  - 200.0f * i + 50.0f - j * 20.0f));
            leftlights[i * 10 + j].modelMatrix = glm::translate(identityMatrix, glm::vec3(-15.0f, 5.0f, (tick % 200) - 200.0f * i + 50.0f - j * 20.0f));
        }
    }
    if (carYawState != 0) {
        carYawAngle = clamp(carYawAngle - carYawState, -10.0f, 10.0f);
        carYawPosition = clamp(carYawPosition + carYawState / 5.0f, -12.0f, 12.0f);
    }
    else {
        if (carYawAngle < 0) {
            carYawAngle = clamp(carYawAngle + 1.0f, -10.0f, 0.0f);
        }
        else {
            carYawAngle = clamp(carYawAngle - 1.0f, 0.0f, 10.0f);
        }
    }
    auto rotation = glm::rotate(identityMatrix, glm::radians(carYawAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    car.modelMatrix = glm::translate(rotation, glm::vec3(carYawPosition, 1.0f, 0.0f));
    car.normaleRotationMatrix = glm::transpose(glm::inverse(rotation));
}


void Draw(GameObject gameObject) {
    glUseProgram(shaderInformation.shaderProgram);
    glUniformMatrix4fv(shaderInformation.unifModel, 1, GL_FALSE, glm::value_ptr(gameObject.modelMatrix));
    glUniformMatrix4fv(shaderInformation.unifView, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(shaderInformation.unifProjection, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(shaderInformation.unifnormaleRotation, 1, GL_FALSE, glm::value_ptr(gameObject.normaleRotationMatrix));
    glUniform3fv(shaderInformation.unifCameraPosition, 1, glm::value_ptr(cameraPosition));

    glUniform4fv(shaderInformation.unifLightAmbient, 1, glm::value_ptr(light.lightAmbient));
    glUniform4fv(shaderInformation.unifLightDiffuse, 1, glm::value_ptr(light.lightDiffuse));
    glUniform3fv(shaderInformation.unifLightDirection, 1, glm::value_ptr(light.lightDirection));
    glUniform4fv(shaderInformation.unifLightSpecular, 1, glm::value_ptr(light.lightSpecular));

    glUniform1f(shaderInformation.unifMaterialShininess, gameObject.material.materialshininess);
    glUniform4fv(shaderInformation.unifMaterialAmbient, 1, glm::value_ptr(gameObject.material.materialAmbient));
    glUniform4fv(shaderInformation.unifMaterialDiffuse, 1, glm::value_ptr(gameObject.material.materialDiffuse));
    glUniform4fv(shaderInformation.unifMaterialEmission, 1, glm::value_ptr(gameObject.material.materialEmission));
    glUniform4fv(shaderInformation.unifMaterialSpecular, 1, glm::value_ptr(gameObject.material.materialSpecular));

    glUniform1f(shaderInformation.unifSpotCutOffCos, spotLight.cutOffCos);
    glUniform3fv(shaderInformation.unifSpotAttenuation, 1, glm::value_ptr(spotLight.attenuation));
    glUniform3fv(shaderInformation.unifSpotDirection, 1, glm::value_ptr(spotLight.direction));
    glUniform3fv(shaderInformation.unifSpotLight, 1, glm::value_ptr(spotLight.light));
    glUniform3fv(shaderInformation.unifSpotPosition, 1, glm::value_ptr(spotLight.position));

    glActiveTexture(GL_TEXTURE0);
    sf::Texture::bind(&gameObject.textureData);
    glUniform1i(shaderInformation.unifTexture, 0);

    // Привязываем вао
    glBindVertexArray(gameObject.vertexVAO);
    // Передаем данные на видеокарту(рисуем)
    glDrawArrays(GL_TRIANGLES, 0, gameObject.buffer_size);
    glBindVertexArray(0);
    // Отключаем шейдерную программу
    glUseProgram(0);
    checkOpenGLerror();
}
// ========================================================= ↑ОТРИСОВКА↑ =========================================================
//================================================================================================================================
// ========================================================== ↓ЧИСТКА↓ ===========================================================
void ReleaseShader() {
    glUseProgram(0);
    glDeleteProgram(shaderInformation.shaderProgram);
}

void ReleaseVBO()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Чистим все выделенные VBO
    for (GLuint VBO: VBOArray)
        glDeleteBuffers(1, &VBO);
}

void Release() {
    ReleaseShader();
    ReleaseVBO();
}
// ========================================================== ↑ЧИСТКА↑ ===========================================================
//================================================================================================================================
// =========================================================== ↓МЕЙН↓ ============================================================
int main() {
    sf::Window window(sf::VideoMode(800, 600), "Merry Christmas, Oleg!!", sf::Style::Default, sf::ContextSettings(24));
    window.setVerticalSyncEnabled(true);

    window.setActive(true);

    glewInit();

    Init();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
    // Счётчик кадров
    int tickCounter = 0;

   
    int dice_roll = distribution(generator);
    auto dice = std::bind(distribution, generator);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::Resized) {
                glViewport(0, 0, event.size.width, event.size.height);
            }
            else if (event.type == sf::Event::KeyReleased) {
                carYawState = 0;
            }
            else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case (sf::Keyboard::Left): carYawState = -1; break;
                case (sf::Keyboard::Right): carYawState = 1; break;
                case (sf::Keyboard::W): moveLightDirection(0, 0.01f, 0); break;
                case (sf::Keyboard::S): moveLightDirection(0, -0.01f, 0); break;
                case (sf::Keyboard::D): moveLightDirection(0.01f, 0, 0); break;
                case (sf::Keyboard::A): moveLightDirection(-0.01f, 0, 0); break;
                case (sf::Keyboard::E): moveLightDirection(0, 0, 0.01f); break;
                case (sf::Keyboard::Q): moveLightDirection(0, 0, -0.01f); break;
                case (sf::Keyboard::O): switchLight(); break;
                default: break; 
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GameTick(tickCounter);
        // Отрисовываем все объекты сцены
        for (GameObject& object : road)
            Draw(object);

        for (GameObject& light : leftlights)
            Draw(light);
        for (GameObject& light : rightlights)
            Draw(light);

        for (GameObject& object : grass)
            Draw(object);

        for (Hurdle& hurdle : currentHurdles)
            Draw(hurdle);

        Draw(car);
        Draw(background);


        int x = 1;
        if (carYawAngle > 10) {
           x = 1;
        }
        tickCounter+= x;
        window.display();
    }

    Release();
    return 0;
}
// =========================================================== ↑МЕЙН↑ ============================================================
//================================================================================================================================