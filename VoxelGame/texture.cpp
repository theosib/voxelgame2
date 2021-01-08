
#include "gl_includes.hpp"
#include "stb_image.h"
#include <iostream>
#include "filelocator.hpp"
#include "texture.hpp"

TextureLibrary TextureLibrary::instance;


// static std::string base_dir("/Users/millerti/tinker/voxelgame/textures/");

unsigned int Texture::openGL_load_texture()
{
    unsigned int texID;
    glGenTextures(1, &texID); // XXX check for failure
    glBindTexture(GL_TEXTURE_2D, texID); 
    
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (nearest_filter) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); // XXX check for failure
    
    stbi_image_free(data);
    data = 0;
    
    return texID;
}

void Texture::load_image_file(const std::string& name)
{
    if (name.find("_nearest") != std::string::npos) nearest_filter = true;
    
    std::string full_path(FileLocator::instance.texture(name));
    int nrChannels;
    
    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(full_path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
    if (!data) {
        // XXX Push error up to UI
        std::cerr << "Failed to load image\n";
        exit(0);
        return;
    }
    std::cout << name << " width=" << width << " height=" << height << " nc=" << nrChannels << std::endl;
    
    inverse_width = 1.0 / width;
    inverse_height = 1.0 / height;
    // texID = openGL_load_texture(data, width, height);
    // stbi_image_free(data);
}


Texture::Texture(const std::string& name) : tex_name(name)
{
    refcnt = 0;
    texID = 0;
    width = 0;
    height = 0;
    data = 0;
    inverse_width = 0;
    inverse_height = 0;
    nearest_filter = false;
    
    load_image_file(name);
}

Texture::~Texture()
{
    if (texID) glDeleteTextures(1, &texID);
    if (data) stbi_image_free(data);
}

void Texture::use(int texture_unit) {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, getID());
}


void Texture::unlink()
{
    refcnt--;
    
    // Schedule delete
    // if (refcnt == 0) {
    //     texture_library.delete(name);
    // }
}



// TextureLibrary::TextureLibrary() {}

TextureLibrary::~TextureLibrary()
{
    for (auto i = texture_list.begin(); i != texture_list.end(); ++i) {
        delete *i;
    }
}

int TextureLibrary::load_texture(const std::string& name)
{
    Texture *t = new Texture(name);
    size_t index = texture_list.size();
    texture_map[name] = (int)index;
    texture_list.push_back(t);
    return (int)index;
}

int TextureLibrary::lookupIndex(const std::string& name)
{
    auto i = texture_map.find(name);
    if (i != texture_map.end()) return i->second;
    return load_texture(name);
}

Texture *TextureLibrary::lookupTexture(const std::string& name)
{
    int index = lookupIndex(name);
    return texture_list[index];
}
    
// void TextureLibrary::free(const std::string& name)
// {
//     auto i = texture_map.find(name);
//     if (i == texture_map.end()) return;
//
//     for (int j=0; j<texture_list.size(); j++) {
//         if (texture_list[j] == i->second) {
//             texture_list[j] = 0;
//             break;
//         }
//     }
//
//     delete i->second;
//     texture_map.erase(i);
// }

