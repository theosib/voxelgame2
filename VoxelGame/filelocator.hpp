#ifndef INCLUDED_FILE_LOCATOR_HPP
#define INCLUDED_FILE_LOCATOR_HPP

#include <string>

class FileLocator {
public:
    static FileLocator instance;
    
private:
    std::string base_config_dir;
    
public:
    void setConfigDir(const std::string& dir) {
        base_config_dir = dir;
    }
    
    //FileLocator(const std::string& dir) {
    //    setConfigDir(dir);
    //}

    FileLocator();
    
    std::string texture(const std::string& tex_name);
    std::string mesh(const std::string& mesh_name);
    std::string shader(const std::string& shader_name);
    std::string chunk(const std::string& chunk_name);
    
    static bool file_exists(const std::string& fname);
};

#endif
