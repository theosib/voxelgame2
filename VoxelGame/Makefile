
CXX = clang++
CFLAGS = -Ofast -g -std=c++17 -I../glm/ -I../stb/
CXXFLAGS = $(CFLAGS)
LDFLAGS = $(CFLAGS) -lglfw -framework OpenGL

HEADERS = \
block.hpp            cameracontroller.hpp chunkview.hpp        datacontainer.hpp    gamewindow.hpp       position.hpp         spinlock.hpp         uielements.hpp       worldview.hpp \
blocklibrary.hpp     cameramodel.hpp      compat.hpp           facing.hpp           geometry.hpp         render.hpp           texture.hpp          window.hpp \
blocktype.hpp        chunk.hpp            constants.hpp        filelocator.hpp      mesh.hpp             shader.hpp           time.hpp             world.hpp \
entity.hpp spline.hpp

SOURCES = \
cameramodel.cpp       datacontainer.cpp     geometry.cpp          mesh_parser.cpp       shader.cpp            texture.cpp           window.cpp            filelocator.cpp \
blocklibrary.cpp      chunk.cpp             facing.cpp            main.cpp              position.cpp          static_cube_block.cpp time.cpp              world.cpp \
cameracontroller.cpp  chunkview.cpp         gamewindow.cpp        mesh.cpp              render.cpp            stb.cpp               uielements.cpp        worldview.cpp \
blocktype.cpp  entity.cpp rotation_stuff.cpp dirt_block.cpp spline.cpp

OBJECTS = $(SOURCES:.cpp=.o)

all: game
    
%.o : %.cpp $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $< -o $@

game: $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJECTS) game

# longconcurrentmap.hpp longconcurrentmap_impl.hpp
