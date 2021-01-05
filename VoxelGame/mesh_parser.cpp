
#include "mesh.hpp"
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <string.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "filelocator.hpp"
#include "texture.hpp"

static bool startsWith(const char *line, const char *prefix)
{
    while (*prefix) {
        if (*prefix != *line) return false;
        prefix++;
        line++;
    }
    return true;
}

static bool startsWith(const std::string& line, const char *prefix)
{
    return startsWith(line.c_str(), prefix);
}

static bool delimiter(char c)
{
    if (c == ' ') return true;
    if (c == '\t') return true;
    return false;
}

static const char *lineValue(const std::string& line)
{
    const char *p = line.c_str();
    while (*p && !delimiter(*p)) p++;
    if (!*p) return 0;
    while (*p && delimiter(*p)) p++;
    if (!*p) return 0;
    return p;
}

enum Key {
    NONE,
    TEX,
    BOX,
    FACE
};

static void parseDataLine(const std::string& line, std::vector<float>& values)
{
    const char *p = line.c_str();    
    while (*p) {
        while (*p && isspace(*p)) p++;
        if (!*p) break;
        
        double val = atof(p);
        values.push_back((float)val);
        
        while (*p && !isspace(*p)) p++;
    }
}

void Mesh::addVertices(Face *face, std::vector<float>& values, int key, bool int_tex_coords, int texture_index)
{
    if (key == TEX && int_tex_coords) {
        Texture *tex = TextureLibrary::instance.getTexture(texture_index);
        for (int i=0; i<values.size(); i+=2) {
            values[i] /= tex->getWidth();
            values[i+1] /= tex->getHeight();
        }
    }
    
    int group_size;
    switch (key) {
    case TEX:
        group_size = 2;
        break;
    case FACE:
        group_size = 3;
        break;
    case BOX:
        group_size = 6;
        break;
    }
    
    for (int i=0; i<values.size(); i+=group_size) {
        switch (key) {
        case TEX:
            face->addTexCoord(glm::make_vec2(&values[i]));
            break;
        case FACE:
            face->addVertex(glm::make_vec3(&values[i]));
            break;
        case BOX:
            collision.emplace_back(&values[i]);
            break;
        }
    }    
}

static int parseSolidFaces(const char *p)
{
    int solid_faces = 0;
    
    while (*p) {
        while (*p && isspace(*p)) p++;
        
        int face = facing::faceFromName(p);
        solid_faces |= facing::bitmask(face);
        
        while (*p && !isspace(*p)) p++;
    }
    
    return solid_faces;
}


void Mesh::loadMesh(const std::string& name)
{
    std::string fname = FileLocator::instance.mesh(name);
    std::ifstream infile(fname);
    if (!infile.is_open()) {
        std::cout << "Unable to open " << name << std::endl;
        return;
    }
    
    std::string line;
    int face = -1, key = NONE;
    std::vector<float> values;
    bool int_tex_coords = false;
    
    while (std::getline(infile, line)) {
         //std::cout << line << std::endl;
        if (line[0] == '#') continue;
        
        if (line[0]==' ' || line[0] == '\t') {
            parseDataLine(line, values);
        } else {
            if (values.size()>0) {
                Face *f = getFace(face);
                addVertices(f, values, key, int_tex_coords, texture_index);
                values.clear();
            }
            
            if (startsWith(line, "tex:")) {
                key = TEX;
                face = facing::faceFromName(line.c_str()+4);
            } else if (startsWith(line, "face:")) {
                key = FACE;
                face = facing::faceFromName(line.c_str()+5);
            } else if (startsWith(line, "include:")) {
                const char *v = lineValue(line);
                loadMesh(v);
            } else if (startsWith(line, "texture:")) {
                const char *v = lineValue(line);
                texture_index = TextureLibrary::instance.lookupIndex(v);
            } else if (startsWith(line, "texture-scale:")) {
                const char *v = lineValue(line);
                if (startsWith(v, "float")) {
                    int_tex_coords = false;
                } else if (startsWith(v, "int")) {
                    int_tex_coords = true;
                }
            } else if (startsWith(line, "translucent:")) {
                const char *v = lineValue(line);
                translucent = (v[0] == 't');
            } else if (startsWith(line, "solid-faces:")) {
                const char *v = lineValue(line);
                solid_faces = parseSolidFaces(v);
            } else if (startsWith(line, "box:")) {
                key = BOX;
            }
        }
    }
    
    if (values.size()>0) {
        Face *f = getFace(face);
        addVertices(f, values, key, int_tex_coords, texture_index);
    }    
}

