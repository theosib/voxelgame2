
#include "filelocator.hpp"
#include <iostream>

#if defined _WIN32 || defined _WIN64
//#include <windows.h>
//#include <winbase.h>
//#include <fileapi.h>
#include <Shlobj.h>
#endif
#ifdef __APPLE__
#include <sys/stat.h>
#endif

#if defined _WIN32 || defined _WIN64
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
//#define FILE_ATTRIBUTE_DIRECTORY 0x10
BOOL FileExists(LPCSTR szPath)
{
    DWORD dwAttrib = GetFileAttributesA(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
#endif

#ifdef __APPLE__
bool FileExists(const char* fname)
{
    struct stat buffer;
    return (stat(fname, &buffer) == 0);
}
#endif

FileLocator::FileLocator()
{
#if defined _WIN32 || defined _WIN64
    CHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        strcat_s(path, MAX_PATH, "/voxelgame");
        setConfigDir(path);
    }
    std::cout << "Home dir: " << base_config_dir << std::endl;
#endif
}

std::string FileLocator::texture(const std::string& tex_name)
{
    return base_config_dir + "/textures/" + tex_name + ".png";
}

std::string FileLocator::mesh(const std::string& mesh_name)
{
    return base_config_dir + "/blocks/" + mesh_name + ".dat";
}

std::string FileLocator::shader(const std::string& shader_name)
{
    return base_config_dir + "/shaders/" + shader_name;
}

std::string FileLocator::chunk(const std::string& chunk_name)
{
    if (chunk_name.size() == 0) {
        return base_config_dir + "/storage";        
    } else {
        return base_config_dir + "/storage/" + chunk_name;
    }
}

bool FileLocator::file_exists(const std::string& fname)
{
    return FileExists(fname.c_str());
}

FileLocator FileLocator::instance;
