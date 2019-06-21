#version 330 core

//все, что нам нужно от фрагментного шейдера
out vec3 color;

//Интерполированные значения из вершинного шейдера
in vec3 NormalCamCoords;
in vec3 LightDirectionCamCoords;
in vec3 tangentCamCoords;
in vec3 bitangentCamCoords;
in vec2 UV;

//Значения, которые остаются константными для данного шага
uniform sampler2D tangentnm;
uniform sampler2D diffuse;

void main() {
    vec3 n = normalize(NormalCamCoords);  //нормаль данного сегмента в camera_coords

    mat3 D = mat3(normalize(tangentCamCoords), normalize(bitangentCamCoords), n); //Darboux frame
    n = normalize((D*normalize(texture(tangentnm, UV).rgb * 2 - 1))); //применение карты нормалей в касательном базисе

    vec3 l = normalize(LightDirectionCamCoords); //направление на свет (от фрагмента к источнику)
    float cos;
    //проверка на попадание луча с другой стороны
    if (dot(n,l) < 0) {
        cos = 0;
    }
    else cos = dot(n,l); //в силу нормализованности не может быть больше 1
    vec3 base_color = texture(diffuse, UV).xyz;
    color = base_color*(0.07 + 1.45*cos);  //формула освещения: ambient + diffuse
}

