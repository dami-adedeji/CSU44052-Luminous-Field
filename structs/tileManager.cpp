#include "tileManager.h"
#include <glm/glm.hpp>
#include <iostream>

void TileManager::initialise()
{
	texture_path = "../textures/coast_sand_rocks_02/coast_sand_rocks_02_diff_1k.jpg";
	//normal_path = "../textures/coast_sand_rocks_02/coast_sand_rocks_02_nor_gl_1k.jpg";
}

void TileManager::updateTiles(glm::vec3 playerPosition, Shader &program)
	{
		//std::cout << "Camera position: " << playerPosition.x << ", " << playerPosition.z << std::endl;
		//std::cout << "Tile size: " << tileSize << std::endl;

		int playerTileX = static_cast<int>(std::round(playerPosition.x / tileSize));
		int playerTileZ = static_cast<int>(std::round(playerPosition.z / tileSize));

		//std::cout << "Player tile coords: " << playerTileX << ", " << playerTileZ << std::endl;

		//std::map<std::pair<int,int>, Tile> tilesToRemove;
		if (runFirstUpdate || currentTile_X != playerTileX || currentTile_Z != playerTileZ) // if firstUpdate true for run OR crossing tile boundary then update tiles
		{
			currentTile_X = playerTileX;
			currentTile_Z = playerTileZ;
			runFirstUpdate = false;

			for (int x = playerTileX - tileDistance; x <= playerTileX + tileDistance; ++x)
			{
				for (int z = playerTileZ - tileDistance; z <= playerTileZ + tileDistance; ++z)
				{
					// for curr tile at x,z
					std::pair<int, int> tileKey = std::make_pair(x, z);
					//std::cout << "Checking tile: " << x << ", " << z << std::endl;

					if (tiles.find(tileKey) == tiles.end()) // not found-> make the tile
					{
						//std::cout << "Spawning tile at " << x << ", " << z << std::endl;

						Tile tile;
						tile.initialise(glm::vec3(x * tileSize, 0.0f, z * tileSize), program, texture_path);
						tiles[tileKey] = tile;

						//std::cout << "Spawned tile at " << x << ", " << z << std::endl;
					}
				}
			}

			if (frameCounter % cleanupInterval == 0)
			{
				for (auto t = tiles.begin(); t != tiles.end(); )
				{
					int x = t->first.first; // x in x,z of tile to delete
					int z = t->first.second; // z of same

					if (std::abs(x - playerTileX) > tileDistance || std::abs(z - playerTileZ) > tileDistance)
					{
						// Clean up the tile if needed (e.g., GPU cleanup)
						t->second.cleanup();
						t = tiles.erase(t); // erase returns the next iterator
					}
					else ++t;
				}
			}

			for (auto& [key, tile] : tiles) {
				int x = key.first;
				int z = key.second;

				bool shouldBeActive = std::abs(x - playerTileX) <= renderDistance && std::abs(z - playerTileZ) <= renderDistance;
				tileActiveStatus[key] = shouldBeActive;
			}
		}
	frameCounter++;
	}

void TileManager::renderTiles(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Shader &program)
{
    for (auto& [pos, tile] : tiles)
    {
        if (!tileActiveStatus[pos]) continue; // only active tiles
        tile.render(viewMatrix, projectionMatrix, program);
        //std::cout << "Rendering tile at " << tile.position.x/tileSize << ", " << tile.position.z/tileSize << std::endl;
    }
}

void TileManager::cleanup()
{
    for (auto& [pos, tile] : tiles)
        tile.cleanup();

    tiles.clear();
}