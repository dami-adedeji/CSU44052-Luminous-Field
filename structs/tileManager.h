#ifndef _TILE_MANAGER_H_
#define _TILE_MANAGER_H_

#include <glm/glm.hpp>
#include "shader.h"
#include "tile.h"
#include <map>

struct TileManager
{
    std::map<std::pair<int,int>, Tile> tiles;
    int tileDistance = 3;
    const char* texture_path;

    const float tileSize = Tile::tileSize;

    bool runFirstUpdate = true; // to gen tiles on first loading app

    int currentTile_X; // x value of the tile camera was on in last frame
    int currentTile_Z; // z value of the tile camera was on in last frame

    std::map<std::pair<int,int>, bool> tileActiveStatus;

    void initialise();

    void updateTiles(glm::vec3 playerPosition, Shader &program);

    void renderTiles(glm::mat4 cameraMatrix, Shader &program);

    void cleanup();
};

#endif