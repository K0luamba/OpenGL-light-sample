#include <iostream>
#include <stdlib.h>
#include <limits>
#include <cstring>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <geometry.h>
#include "model.h"

using namespace std;

bool animation = true;
bool shift = false;
int camPosX, camPosY = 0;
bool keys[4] = {0, 0, 0, 0}; //массив для клавиш: D A S W

void key_call() {
        if (keys[0] == 1) //Влево
            camPosX -= 2;
        if (keys[1] == 1) //Вправо
            camPosX += 2; 
        if (keys[2] == 1) //Вниз
            camPosY += 2;
        if (keys[3] == 1) //Вверх
            camPosY -= 2; 
}

void key_callback(GLFWwindow*, int key, int, int action, int) {
    if ((key == GLFW_KEY_Q) && (action == GLFW_PRESS)) {
        animation = !animation;
    }
    if ((key == GLFW_KEY_LEFT_SHIFT) && (action == GLFW_PRESS)) {
        shift = !shift;
    }
    if ((key == GLFW_KEY_D) && (action == GLFW_PRESS)) {
        keys[0] = 1;
    }
    if ((key == GLFW_KEY_D) && (action == GLFW_RELEASE)) {
        keys[0] = 0;
    }
    if ((key == GLFW_KEY_A) && (action == GLFW_PRESS)) {
        keys[1] = 1;
    }
    if ((key == GLFW_KEY_A) && (action == GLFW_RELEASE)) {
        keys[1] = 0;
    }
    if ((key == GLFW_KEY_S) && (action == GLFW_PRESS)) {
        keys[2] = 1;
    }
    if ((key == GLFW_KEY_S) && (action == GLFW_RELEASE)) {
        keys[2] = 0;
    }
    if ((key == GLFW_KEY_W) && (action == GLFW_PRESS)) {
        keys[3] = 1;
    }
    if ((key == GLFW_KEY_W) && (action == GLFW_RELEASE)) {
        keys[3] = 0;
    }
    key_call();  //непосредственно действия, связанные с движением
}

//упаковка матрицы по строчно в массив
template <size_t DimRows,size_t DimCols,class T> void export_row_major(mat<DimRows,DimCols,T> m, float *arr) {
    for (size_t i=DimRows; i--; )
        for (size_t j=DimCols; j--; arr[i+j*DimCols]=m[i][j]);
}

GLuint load_texture(const string &imagepath) {
    cerr << "Загрузка текстуры(\"" << imagepath << "\");" << endl;
    stbi_set_flip_vertically_on_load(1);
    int width, height, bpp; //(последний параметр нужен просто для вызова, учитывая сигнатуру)
    unsigned char* rgb = stbi_load(imagepath.c_str(), &width, &height, &bpp, 3);
    //создание текстуры в OpenGL
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

void read_n_compile_shader(const string filename, GLuint &hdlr, GLenum shaderType) {
    cerr << "Загрузка шейдера " << filename << "   ";
    ifstream is(filename, ios::in|ios::binary|ios::ate);
    if (!is.is_open()) {
        cerr << "Не удалось открыть файл" << endl;
        return;
    }
    cerr << "ok" << endl;
    //содержание файла -> буфер
    long size = is.tellg();
    char *buffer = new char[size+1];
    is.seekg(0, ios::beg);
    is.read(buffer, size);
    is.close();
    buffer[size] = 0;
    //непосредственно создание OpenGL шейдера
    cerr << "Компилируется шейдер " << filename << "   ";
    hdlr = glCreateShader(shaderType);
    glShaderSource(hdlr, 1, (const GLchar**)&buffer, NULL);
    glCompileShader(hdlr);
    GLint success;
    glGetShaderiv(hdlr, GL_COMPILE_STATUS, &success);
    cerr << (success ? "ok" : "ошибка") << endl;
}

//внешняя функция связки шейдеров к программе
GLuint set_shaders(const string vsfile, const string fsfile) {
    GLuint vert_hdlr, frag_hdlr;
    //непосредственное указание на языке openGL ролей
    read_n_compile_shader(vsfile, vert_hdlr, GL_VERTEX_SHADER);
    read_n_compile_shader(fsfile, frag_hdlr, GL_FRAGMENT_SHADER);

    GLuint openglHandler = glCreateProgram();
    glAttachShader(openglHandler, vert_hdlr);
    glAttachShader(openglHandler, frag_hdlr);
    glLinkProgram(openglHandler);
    return openglHandler;
}

int setup_window(GLFWwindow* &window, GLuint width, GLuint height) {
    cerr << "Создание контекста" << "   ";
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    window = glfwCreateWindow(width, height, "My OpenGL object sample", NULL, NULL);
    glfwMakeContextCurrent(window); 
    if (!window) {
        cerr << "Не удалось создать контекст" << endl;
        return -2;
    }
    else cerr << "ok" << endl;
    glfwSetKeyCallback(window, key_callback);  //загоняем нашу функцию как управление
    glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Не удалось создать контекст" << endl;
        return -3;
    }

    return 0;
}

//создает стандартную матрицу поворота по заданному углу
Matrix z_rotation_matrix(const float angle) {
    Matrix R = Matrix::identity();
    R[0][0] = R[2][2] = cos(angle);
    R[2][0] = sin(angle);
    R[0][2] = -R[2][0];
    return R;
}


int main() {
    //задание файлов и констант
    int i, j, k;
    float cache[16];
    mat<3, 3, float> A;
    GLuint width = 900, height = 900;
    const float delta = 0.5; //в системе координат модели
    const float scale = 0.5;  
    string objectFile("../obj/diablo.obj");
    string normalsFile("../obj/diablo_nm_tangent.tga");
    string textureFile("../obj/diablo_diffuse.tga");
    Model model(objectFile.c_str(), scale, delta); //к этому моменту весь парсинг файлов произошел в model.cpp

    GLFWwindow* window;
    if (setup_window(window, width, height)) {
        glfwTerminate();
        return -1;
    }
    //Проверка на ошибки на данном этапе
	GLenum gl_error = glGetError();
	while (gl_error != GL_NO_ERROR)
		gl_error = glGetError();

    GLuint openglHandler = set_shaders("../src/vertex.glsl", "../src/fragment.glsl");
    //матрицы для преобразования: object_coords -> world_coords -> camera_coords -> clip_coords
    Matrix M = Matrix::identity();
    Matrix V = Matrix::identity();
    Matrix P = Matrix::identity();
    Matrix MVP = P*V*M;
    //Выделяем места для передачи данных в шейдеры ~
    GLuint LightUN = glGetUniformLocation(openglHandler, "LightPositionWorldCoords");
    GLuint MatrixUN = glGetUniformLocation(openglHandler, "MVP");
    GLuint ViewMatrixUN = glGetUniformLocation(openglHandler, "V");
    GLuint ModelMatrixUN = glGetUniformLocation(openglHandler, "M");
    GLuint Texture0UN = glGetUniformLocation(openglHandler, "tangentnm");
    GLuint Texture1UN = glGetUniformLocation(openglHandler, "diffuse");
    //память под данные из объектов
    vector<GLfloat> vertices(3*3*model.nfaces(), 0);
    vector<GLfloat> normals(3*3*model.nfaces(), 0);
    vector<GLfloat> tangents(3*3*model.nfaces(), 0);
    vector<GLfloat> bitangents(3*3*model.nfaces(), 0);
    vector<GLfloat> uvs(2*3*model.nfaces(), 0);
    //берем их из модели, проход по полигонам треуг.
    for (i=0; i<model.nfaces(); i++) {
        Vec3f v0 = model.point(model.vert(i, 0));
        Vec3f v1 = model.point(model.vert(i, 1));
        Vec3f v2 = model.point(model.vert(i, 2));
        //используем функции и классы geomerty.h для расширения/обрезания векторов
        Vec3f v01 = proj<3>(M*(embed<4>(v1 - v0)));
        Vec3f v02 = proj<3>(M*(embed<4>(v2 - v0)));
        //создание касательной и бикасательной составляющих для текущей грани
        A[0] = v01;
        A[1] = v02;
        A[2] = cross(v01, v02).normalize();
        Vec3f tangential = A.invert() * Vec3f(model.uv(i, 1).x - model.uv(i, 0).x, model.uv(i, 2).x - model.uv(i, 0).x, 0);
        Vec3f bitangential = A.invert() * Vec3f(model.uv(i, 1).y - model.uv(i, 0).y, model.uv(i, 2).y - model.uv(i, 0).y, 0);
        tangential.normalize();
        bitangential.normalize();
        //модель -> вектора данных
        for (j=0; j<3; j++) {
            for (k=0; k<3; k++) vertices[(i*3+j)*3 + k] = model.point(model.vert(i, j))[k];
            for (k=0; k<3; k++) normals[(i*3+j)*3 + k] = model.normal(i, j)[k];
            for (k=0; k<2; k++) uvs[(i*3+j)*2 + k] = model.uv(i, j)[k];
            for (k=0; k<3; k++) tangents[(i*3+j)*3 + k] = tangential[k];
            for (k=0; k<3; k++) bitangents[(i*3+j)*3 + k] = bitangential[k];
        }
    }
    //создание VAO
    GLuint vao = 0;
    glGenVertexArrays(1, &vao); //создание в контексте openGL места под него
    glBindVertexArray(vao); //привязка для использования
    //вектора данных -> буфер данных OpenGL 
    //вершины
    glEnableVertexAttribArray(0); //(каждая часть на своем месте, запоминаем для шейдера)
    GLuint vertexData = 0;
    glGenBuffers(1, &vertexData); //создание буфера данных
    glBindBuffer(GL_ARRAY_BUFFER, vertexData); //отмечаем его как активный
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vertices.size(), vertices.data(), GL_STATIC_DRAW); //непосредственно копирование информации
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); //(учитываем размерности)
    //нормали
    glEnableVertexAttribArray(1);
    GLuint normalData = 0;
    glGenBuffers(1, &normalData);
    glBindBuffer(GL_ARRAY_BUFFER, normalData);
    glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    //координаты текстур
    glEnableVertexAttribArray(2);
    GLuint uvData = 0;
    glGenBuffers(1, &uvData);
    glBindBuffer(GL_ARRAY_BUFFER, uvData);
    glBufferData(GL_ARRAY_BUFFER, uvs.size()*sizeof(GLfloat), uvs.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    //касательная и бикасательная
    glEnableVertexAttribArray(3);
    GLuint tangentData = 0;
    glGenBuffers(1, &tangentData);
    glBindBuffer(GL_ARRAY_BUFFER, tangentData);
    glBufferData(GL_ARRAY_BUFFER, tangents.size()*sizeof(GLfloat), tangents.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(4);
    GLuint bitangentData = 0;
    glGenBuffers(1, &bitangentData);
    glBindBuffer(GL_ARRAY_BUFFER, bitangentData);
    glBufferData(GL_ARRAY_BUFFER, bitangents.size()*sizeof(GLfloat), bitangents.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    //загрузка карты нормалей и диффузной текстуры
    GLuint tex_normals = load_texture(normalsFile);
    GLuint textureData = load_texture(textureFile);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_normals);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureData);

    glViewport(camPosX, camPosY, width, height); //стандартное поле зрения
    glEnable(GL_DEPTH_TEST);
    glClearDepth(0);
    glDepthFunc(GL_GREATER); //так как ось z смотрит на нас, рисуем то, у чего z-координата макс.
    glUseProgram(openglHandler); //ативируем шейдеры для нашей программы

    chrono::time_point<chrono::steady_clock> start = chrono::steady_clock::now();
    while (!glfwWindowShouldClose(window)) {
        //поддержка 25 fps => движение будет с одинаковой скоростью на любом устройстве
        chrono::time_point<chrono::steady_clock> end = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(end - start).count() < 40) { 
            this_thread::sleep_for(chrono::milliseconds(40 - chrono::duration_cast<chrono::milliseconds>(end - start).count()));
            continue;
        }
        start = end;

        glViewport(camPosX, camPosY, width, height);
        glClearColor(0.24f, 0.46f, 0.62f, 1.0f); //заливка фона (небо)
        glUniform1i(Texture0UN, 0);
        glUniform1i(Texture1UN, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //очистка буферов для подготовки к новой отрисовке
        //отправка матриц преобразований в шейдер
        //проверка условий поворота, ускорения
        float angle = 0.01;
        if (shift) angle = 0.02;
        Matrix R = z_rotation_matrix(angle);
        if (animation) M = R*M;
        MVP = P*V*M;
        //используем вспомогательный массив cache     
        export_row_major(M, cache); //(нужно, так как OpenGL не знает данный класс матриц)
        glUniformMatrix4fv(ModelMatrixUN, 1, GL_FALSE, cache);
        export_row_major(V, cache);
        glUniformMatrix4fv(ViewMatrixUN, 1, GL_FALSE, cache);
        export_row_major(MVP, cache);
        glUniformMatrix4fv(MatrixUN, 1, GL_FALSE, cache);
        //установка источника, передача информации о нем в шейдер
        float lightpos[3] = {20, 5, 20}; 
        glUniform3fv(LightUN, 1, lightpos);
        //непосредственно функция отрисовки полигонов
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        //и выгрузка на экран
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //в завершение освобождаем используемые ресурсы
    glUseProgram(0);
    glDeleteProgram(openglHandler); 
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glDeleteBuffers(1, &vertexData);
    glDeleteBuffers(1, &normalData);
    glDeleteBuffers(1, &uvData);
    glDeleteTextures(1, &textureData);
    glDeleteBuffers(1, &tangentData);
    glDeleteBuffers(1, &bitangentData);
    glDeleteVertexArrays(1, &vao);

    glfwTerminate();
    return 0;
}

