#include "Texture.h"
#include <glad/glad.h>
#include <iostream>
#include <stb_image.h>

GLuint loadTexture(std::string filePath)
{
    GLuint texID;

    // Gera o identificador da textura na memória
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Ajusta os parâmetros de wrapping e filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Carregamento dos pixels da imagem
    int width, height, nrChannels;
    std::cout << "Loading texture: " << filePath << std::endl;

    unsigned char *data = stbi_load(filePath.c_str(), &width, &height,
                                    &nrChannels, 0);

    if (data)
    {
        std::cout << "Texture loaded successfully: " << width << "x" << height << " channels: " << nrChannels << std::endl;
        if (nrChannels == 3) // jpg, bmp
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         data);
        }
        else // png
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture: " << filePath << std::endl;
        std::cout << "Error: " << stbi_failure_reason() << std::endl;
    }

    // Liberar o data e desconectar a textura
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}