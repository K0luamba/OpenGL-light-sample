#version 330 core

//изменяемые данные, хранимые в виде массивов
layout(location = 0) in vec3 vertexPositionModelCoords;
layout(location = 1) in vec3 vertexNormalModelCoords;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

//выходные данные, интерполируются
out vec2 UV;
out vec3 NormalCamCoords;
out vec3 LightDirectionCamCoords;
out vec3 tangentCamCoords;
out vec3 bitangentCamCoords;

//Константные данные для каждого шага
uniform vec3 LightPositionWorldCoords;
uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;

void main() {
    gl_Position = MVP * vec4(vertexPositionModelCoords, 1); //позиция вершины преобразовывается в clip_coords
    UV = vertexUV; //текстурные координаты данной вершины

    NormalCamCoords = (transpose(inverse(V*M)) * vec4(vertexNormalModelCoords, 0)).xyz; //нормаль для вершины, в camera_coords

    vec3 vertexPositionCamCoords = (V*M*vec4(vertexPositionModelCoords,1)).xyz;
    vec3 LightPositionCamCoords = (V*vec4(LightPositionWorldCoords, 1)).xyz; //M опускаем, чтобы свет не двигался при вращении объектов
    LightDirectionCamCoords = LightPositionCamCoords - vertexPositionCamCoords;

    tangentCamCoords = (V*vec4(  tangent, 0)).xyz;
    bitangentCamCoords = (V*vec4(bitangent, 0)).xyz;
}

