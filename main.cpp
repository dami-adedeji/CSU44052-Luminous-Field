#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <map>

static GLFWwindow *window;
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// OpenGL camera view parameters
static glm::vec3 eye_center(0, 20, 0);
glm::vec3 camera_target = eye_center;
static glm::vec3 lookat;
static glm::vec3 front(0,0,-1);
glm::vec3 flatFront;
static glm::vec3 up(0, 1, 0);

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// for rotation
bool firstMouse = true;
float yaw   = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch =  0.0f;
float lastX =  800.0f / 2.0;
float lastY =  600.0 / 2.0;

constexpr float tileSize = 250.0f;


static GLuint LoadTextureTileBox(const char *texture_file_path) {
    int w, h, channels;
    uint8_t* img = stbi_load(texture_file_path, &w, &h, &channels, 3);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // To tile textures on a box, we set wrapping to repeat
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (img) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture " << texture_file_path << std::endl;
    }
    stbi_image_free(img);

    return texture;
}

struct Tile {
	glm::vec3 position;
	GLuint shaderID;

	GLfloat vertices[12] = {
		// to have position as center rather than front left:
		-0.5f, 0.0f, 0.5f, // front left
		0.5f, 0.0f, 0.5f, // front right
		0.5f, 0.0f, -0.5f, // back right
		-0.5f, 0.0f, -0.5f // back left
	};

	GLfloat colours[12] = {
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f
	};

	GLfloat uv[8] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	GLuint indices[6] = {
		0, 1, 2,
		0, 2, 3
	};


	GLuint VAO, VBO, UVBO, EBO, CBO, mvpMatrixID, textureID, textureSamplerID;
	void initialise(glm::vec3 position, GLuint shaderID, const char* texture_path)
	{
		this->position = position;
		this->shaderID = shaderID;

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glGenBuffers(1, &CBO);
		glBindBuffer(GL_ARRAY_BUFFER, CBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

		glGenBuffers(1, &UVBO);
		glBindBuffer(GL_ARRAY_BUFFER, UVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		textureID = LoadTextureTileBox(texture_path);

		textureSamplerID = glGetUniformLocation(shaderID, "textureSampler");
		if (textureSamplerID < 0)
			std::cerr << "Uniform 'textureSampler' not found in shader!" << std::endl;

		if (!glIsTexture(textureID))
			std::cerr << "Invalid texture loaded from " << texture_path << std::endl;
	}

	void render(glm::mat4 cameraMatrix)
	{
		glUseProgram(shaderID);

		glBindVertexArray(VAO);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, CBO);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		mvpMatrixID = glGetUniformLocation(shaderID, "MVP");

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);  // Apply translation
		modelMatrix = glm::scale(modelMatrix, glm::vec3(tileSize, 1, tileSize));
		// Calculate MVP matrix
		glm::mat4 MVP = cameraMatrix * modelMatrix;

		// Send MVP matrix to the shader
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &MVP[0][0]);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, UVBO);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// set texture sampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(textureSamplerID, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	void cleanup()
	{
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &CBO);
		glDeleteBuffers(1, &EBO);
		glDeleteBuffers(1, &UVBO);
		glDeleteVertexArrays(1,&VAO);
	}
};

struct TileManager
{
	std::map<std::pair<int,int>, Tile> tiles;
	int tileDistance = 3;
	const char* texture_path;

	bool runFirstUpdate = true; // to gen tiles on first loading app

	int currentTile_X; // x value of the tile camera was on in last frame
	int currentTile_Z; // z value of the tile camera was on in last frame

	std::map<std::pair<int,int>, bool> tileActiveStatus;
	void initialise()
	{
		texture_path = "../textures/grass-1000-mm-architextures.jpg";
	}

	void updateTiles(glm::vec3 playerPosition, GLuint shaderID)
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
						tile.initialise(glm::vec3(x * tileSize, 0.0f, z * tileSize), shaderID, texture_path);
						tiles[tileKey] = tile;

						//std::cout << "Spawned tile at " << x << ", " << z << std::endl;
					}
				}
			}

			/*for (auto t = tiles.begin(); t != tiles.end(); )
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
			}*/

			for (auto& [key, tile] : tiles) {
				int x = key.first;
				int z = key.second;

				bool shouldBeActive = std::abs(x - playerTileX) <= tileDistance && std::abs(z - playerTileZ) <= tileDistance;
				tileActiveStatus[key] = shouldBeActive;
			}
		}

	}

	void renderTiles(glm::mat4 cameraMatrix)
	{
			for (auto& [pos, tile] : tiles)
			{
				if (!tileActiveStatus[pos]) continue; // only active tiles
				tile.render(cameraMatrix);
				//std::cout << "Rendering tile at " << tile.position.x/tileSize << ", " << tile.position.z/tileSize << std::endl;
			}


	}

	void cleanup()
	{
		for (auto& [pos, tile] : tiles)
			tile.cleanup();

		tiles.clear();
	}
};

int main(void) {
    // Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "hi", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to open a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Ensure we can capture the escape key being pressed below
	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	// Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	GLuint objectShader = LoadShadersFromFile("../shaders/object.vert", "../shaders/object.frag");
	if (objectShader == 0) std::cerr << "Failed to load object shader." << std::endl;

	// Background
	glClearColor(0.003f, 0.0025f, 0.05f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// shaders, objects, set up rest here
	glm::float32 FoV = 60;
	glm::float32 zNear = 0.1f;
	glm::float32 zFar = 1000.0f;
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, zNear, zFar);

	TileManager t;
	t.initialise();

	float prevDeltaTime = 0.016f; // 60 fps

	do
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (deltaTime > 0.05f)
			deltaTime = 0.05f;

		deltaTime = glm::mix(prevDeltaTime, deltaTime, 0.1f);
		prevDeltaTime = deltaTime;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		processInput(window);

		float lerpSpeed = 5.0f;
		eye_center = glm::mix(eye_center, camera_target, lerpSpeed * deltaTime);

		// calculate viewMatrix and vp
		glm::mat4 viewMatrix = glm::lookAt(eye_center, eye_center + front, up);
		glm::mat4 vp = projectionMatrix * viewMatrix;

		glm::vec3 forwardLook = glm::normalize(front) * tileSize * 0.5f; // to ensure tiles in distancee are created when we get there
		glm::vec3 updatePos = camera_target + forwardLook;
		std::cout << "deltaTime: " << deltaTime << std::endl;

		t.updateTiles(updatePos, objectShader);

		glDepthMask(GL_TRUE);
		// render stuff here
		t.renderTiles(vp);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Clean up
	t.cleanup();

	glDeleteProgram(objectShader);
	// Close OpenGL window and terminate GLFW
	glfwTerminate();
    return 0;
}

// Is called whenever a key is pressed/released via GLFW
void processInput(GLFWwindow *window)
{
	float cameraSpeed = 10.0f * deltaTime;
	glm::vec3 move(0.0f);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_REPEAT)
	{
		//eye_center += cameraSpeed * front;

		/*below to avoid floating errors that is causing slow movement

		flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
		eye_center += cameraSpeed * flatFront;*/
		move += front;
	}

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_REPEAT)
	{
		//eye_center -= cameraSpeed * front;
		/*below to avoid floating errors that is causing slow movement

		flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
		eye_center -= cameraSpeed * flatFront;*/

		move -= front;
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_REPEAT)
	{
		//eye_center -= glm::normalize(glm::cross(front, up) * cameraSpeed);
		move -= glm::normalize(glm::cross(front, up));
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_REPEAT)
	{
		move += glm::normalize(glm::cross(front, up));
	}
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (glm::length(move) > 0.0f)
		camera_target += glm::normalize(move) * cameraSpeed;

	//std::cout << "Movement applied: (" << move.x << ", " << move.z << ")" << std::endl;
	//std::cout << "New camera position: (" << eye_center.x << ", " << eye_center.z << ")" << std::endl;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);
}