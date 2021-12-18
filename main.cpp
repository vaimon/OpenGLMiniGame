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


// В C и C++ есть оператор #, который позволяет превращать параметры макроса в строку
#define TO_STRING(x) #x

glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

struct ShaderInformation {
    // Переменные с индентификаторами ID
    // ID шейдерной программы
    GLuint shaderProgram;
    // ID атрибута вершин
    GLint attribVertex;
    // ID атрибута текстурных координат
    GLint attribTexture;

    GLint attribNormale;
    // ID юниформа текстуры
    GLint unifTexture;
    // ID юниформа сдвига
    GLint unifShift;
    GLint unifProjection;
    GLint unifView;
    GLint unifModel;
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
    // Величина сдвига
    GLfloat shift[3];
};

std::vector <GameObject> gameObjects;
std::vector <GameObject> road;

ShaderInformation shaderInformation;
glm::mat4 identityMatrix = glm::mat4(1.0f);
// Массив VBO что бы их потом удалить
std::vector <GLuint> VBOArray;


const char* VertexShaderSource = TO_STRING(
    #version 330 core\n

    uniform vec3 shift;
    uniform mat4 modelMatrix;
    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;

    in vec3 vertexPosition;
    in vec3 vertexNormale;
    in vec2 vertexTextureCoords;

    out vec2 vTextureCoordinate;
    out vec3 vNormale;

    void main() {
        float x_angle = -0.5;
        float y_angle = 0.5;
        
        vTextureCoordinate = vec2(vertexTextureCoords.x, 1.0 - vertexTextureCoords.y);
        vNormale = vertexNormale;
        gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition + shift, 1.0);
    }
);

const char* FragShaderSource = TO_STRING(
    #version 330 core\n

    uniform sampler2D textureData;

    in vec2 vTextureCoordinate;
    in vec3 vNormale;

    out vec4 color;

    void main() {
        color = texture(textureData, vTextureCoordinate) + ((vec4(vNormale, 1.0) + vec4(vTextureCoordinate * 0.0001, 0.0, 0.0)) * 0.001);
        //color = vec4(vNormale, 1.0) + vec4(vTextureCoordinate * 0.0001, 0.0, 0.0);
    }
);


void Init();
void GameTick(int tick);
void Draw(GameObject gameObject);
void Release();


int main() {
    sf::Window window(sf::VideoMode(800, 600), "Merry Christmas, Oleg!!", sf::Style::Default, sf::ContextSettings(24));
    window.setVerticalSyncEnabled(true);

    window.setActive(true);

    glewInit();

    Init();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f,0.749f,1.0f, 1.0f);
    // Счётчик кадров
    int tickCounter = 0;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::Resized) {
                glViewport(0, 0, event.size.width, event.size.height);
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GameTick(tickCounter);
        // Отрисовываем все объекты сцены
        for (GameObject& object : road)
            Draw(object);

        for (GameObject& object: gameObjects)
            Draw(object);

        tickCounter++;
        window.display();
    }

    Release();
    return 0;
}

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

void loadTexture(sf::Texture& textureData, const char* filename)
{
    if (!textureData.loadFromFile(filename))
    {
        std::cout << "could not load texture" << std::endl;
    }
}

void initBus() {
    GLuint VAO;
    GLuint VBO;
    auto vertices = parseFile(".\\objects\\bus2.obj");
    sf::Texture textureData;
    loadTexture(textureData, ".\\objects\\bus2.png");

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

    gameObjects.push_back(GameObject{
                vertices.size(),
                VAO,
                textureData,
                glm::mat4(1.0f),
                {0, 0, 0} });
        
}

void initRoadPiece(glm::mat4 modelMatrix) {
    GLuint VAO;
    GLuint VBO;
    auto vertices = parseFile(".\\objects\\road.obj");
    sf::Texture textureData;
    loadTexture(textureData, ".\\objects\\road.png");

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

    road.push_back(GameObject{
                vertices.size(),
                VAO,
                textureData,
                modelMatrix,
                {0, 0, 0} });

}

void InitObjects()
{
    //glm::mat4 identity = glm::mat4(1.0f);
    //initBus();
    initRoadPiece(glm::translate(identityMatrix, glm::vec3(0.0f, 0.0f, -50.0f)));
    initRoadPiece(glm::translate(identityMatrix, glm::vec3(0.0f, 0.0f, -250.0f)));
    initRoadPiece(glm::translate(identityMatrix, glm::vec3(0.0f, 0.0f, -450.0f)));
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
    
    shaderInformation.unifShift = glGetUniformLocation(shaderInformation.shaderProgram, "shift");
    if (shaderInformation.unifShift == -1)
    {
        std::cout << "could not bind uniform shift" << std::endl;
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
    
    checkOpenGLerror();
}

void initCamera() {
    viewMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::mat4(1.0f);
    viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, -15.0f, -50.0f));
    projectionMatrix = glm::perspective(45.0f, (GLfloat)800.0f / (GLfloat)600.0f, 0.1f, 300.0f);
}

void Init() {
    InitShader();
    InitObjects();
    initCamera();
}

// Обработка шага игрового цикла
void GameTick(int tick) {
    for (int i = 0; i < road.size(); ++i) {
        road[i].modelMatrix = glm::translate(identityMatrix, glm::vec3(0.0f, 0.0f, -50.0f - i * 200.0f + (tick % 200)));
    }
        //road[i].modelMatrix = 1.5f - ((tick + (frequency * i)) % (gameObjects.size() * frequency)) / (float)frequency;
}


void Draw(GameObject gameObject) {
    glUseProgram(shaderInformation.shaderProgram);
    glUniform3fv(shaderInformation.unifShift, 1, gameObject.shift);
    glUniformMatrix4fv(shaderInformation.unifModel, 1, GL_FALSE, glm::value_ptr(gameObject.modelMatrix));
    glUniformMatrix4fv(shaderInformation.unifView, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(shaderInformation.unifProjection, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

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
