#ifndef _INCLUDED_TEXTURE_REPO
#define _INCLUDED_TEXTURE_REPO

/*
Global texture repository
*/

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

class Texture {
private:
    int refcnt;
    unsigned int texID;
    int width, height;
    double inverse_width, inverse_height;
    std::string tex_name;
    unsigned char *data;
    bool nearest_filter;
    
    void load_image_file(const std::string& name);
    unsigned int openGL_load_texture();
    
public:
    Texture(const std::string& name);
    ~Texture();
    
    void link() { refcnt++; }
    void unlink();
    int getWidth() { return width; }
    int getHeight() { return height; }
    float getTexX(int px) { return (float)(px * inverse_width); }
    float getTexY(int py) { return (float)(py * inverse_height); }

    int getID() { 
        if (!texID) texID = openGL_load_texture();
        return texID; 
    }
    
    const std::string& getName() { return tex_name; }
    
    void use(int texture_unit);
};

class TextureLibrary {
public:
    static TextureLibrary instance;
    
private:
    std::vector<Texture *> texture_list;
    std::unordered_map<std::string, int> texture_map;
    int load_texture(const std::string& name);
    
public:
    TextureLibrary() {}
    ~TextureLibrary();
    
    int lookupIndex(const std::string& name);
    Texture *lookupTexture(const std::string& name);
    
    Texture *getTexture(int index) {
        if (index < 0) {
            std::cout << "Invalid texture index " << index << std::endl;
            exit(0);
        }
        return texture_list[index];
    }
    
    size_t numTextures() {
        return texture_list.size();
    }
    
    // void free(const std::string& name);
    
    // std::unordered_map<std::string, Texture *>& getMap() {
    //     return texture_map;
    // }
    
    std::vector<Texture *>& getTextureList() {
        return texture_list;
    }
};


#endif
