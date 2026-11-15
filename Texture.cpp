#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#include "Texture.h"

#include <iostream>

GLuint loadTexture(const std::string& path) {
    int width, height, channels;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(
        path.c_str(), &width, &height, &channels, 3
    );

    if (!data) {
        std::cerr << "Failed to load texture: "
                  << path << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D, 0, format,
        width, height, 0,
        format, GL_UNSIGNED_BYTE, data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return textureID;
}