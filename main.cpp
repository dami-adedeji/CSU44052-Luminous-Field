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
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// OpenGL camera view parameters
static glm::vec3 eye_center;
static glm::vec3 lookat;
static glm::vec3 up(0, 1, 0);

constexpr float tileSize = 100.0f;

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

struct Plane
{
	glm::vec3 position, scale;
	const char* texture_path;
	//GLuint shaderID;

	GLfloat vertices[12] = {
		0.0f, 0.0f, 0.0f,		// front left
		0.0f, 0.0f, -1.0f,		// back right
		1.0f, 0.0f, -1.0f,		// back left
		1.0f, 0.0f, 0.0f		// front right
	};

	GLfloat colours[12] = {
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f
	};

	GLuint indices[6] = {
		0, 2, 1,
		0, 3, 2
	};


	GLuint VAO, VBO, CBO, EBO, mvpMatrixID;

	void initialise(glm::vec3 position, glm::vec3 scale, GLuint shaderID)
	{
		this->position = position;
		this->scale = scale;
		//this->shaderID = shaderID;

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glGenBuffers(1, &CBO);
		glBindBuffer(GL_ARRAY_BUFFER, CBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		mvpMatrixID = glGetUniformLocation(shaderID, "MVP");

		if (shaderID == 0)
			std::cerr << "Failed to load plane shaders." << std::endl;
	}

	void render(glm::mat4 cameraMatrix, GLuint shaderID)
	{
		glUseProgram(shaderID);
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, CBO);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);  // Apply translation
		modelMatrix = glm::scale(modelMatrix, scale);         // Apply scaling
		// Calculate MVP matrix
		glm::mat4 MVP = cameraMatrix * modelMatrix;

		// Send MVP matrix to the shader
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &MVP[0][0]);

		//glEnableVertexAttribArray(2);
		//glBindBuffer(GL_ARRAY_BUFFER, UVBO);
		//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// set texture sampler to use texture unit 0
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, textureID);
		//shader->setUniform1i("textureSampler", 0);
		//glUniform1i(textureSamplerID, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		//glDisableVertexAttribArray(2);
	}

	void cleanup() {
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &CBO);
		glDeleteBuffers(1, &EBO);
		//glDeleteBuffers(1, &UVBO);
		glDeleteVertexArrays(1,&VAO);
		//glDeleteProgram(shaderID);
	}
};

struct Box {
	glm::vec3 position;			// Position of the box
	glm::vec3 scale;			// Size of the box in each axis
	GLuint shaderID;

	GLfloat vertex_buffer_data[72] = {	// Vertex definition for a canonical box
		// Front face
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,

		// Back face
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		// Left face
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		// Right face
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,

		// Top face
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		// Bottom face
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
	};

	GLfloat color_buffer_data[72] = {
		// Front, red
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Back, yellow
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		// Left, green
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Right, cyan
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,

		// Top, blue
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		// Bottom, magenta
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
	};

	GLuint index_buffer_data[36] = {		// 12 triangle faces of a box
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		16, 17, 18,
		16, 18, 19,

		20, 21, 22,
		20, 22, 23,
	};

	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint colorBufferID;

	// Shader variable IDs
	GLuint mvpMatrixID;

	void initialize( glm::vec3 scale, glm::vec3 position, GLuint shaderID) {
		// Define scale of the box geometry
		this->scale = scale;
		this->position = position + glm::vec3(0.0f, scale.y, 0.0f); // to always have base be at position
		this->shaderID = shaderID;

		// Create a vertex array object
		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		// Create a vertex buffer object to store the vertex data
		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		// Create a vertex buffer object to store the color data
		glGenBuffers(1, &colorBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

		// Create an index buffer object to store the index data that defines triangle faces
		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);


		// Get a handle for our "MVP" uniform
		mvpMatrixID = glGetUniformLocation(shaderID, "MVP");
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(shaderID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		// TODO: Model transform
		// ------------------------------------
		glm::mat4 modelMatrix = glm::mat4();
		// Translate the box to its position
		modelMatrix = glm::translate(modelMatrix, position);
		// Scale the box along each axis
		modelMatrix = glm::scale(modelMatrix, scale);
		// rotate around the axis
		//modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime(),rotateAxis);

		// TODO: Set model-view-projection matrix
		glm::mat4 mvp = cameraMatrix * modelMatrix;
		// ------------------------------------
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			36,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
	}
};

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

	GLuint indices[6] = {
		0, 1, 2,
		0, 2, 3
	};


	GLuint VAO, VBO, EBO, CBO, mvpMatrixID;
	void initialise(glm::vec3 position, GLuint shaderID)
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

		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
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

		//glEnableVertexAttribArray(2);
		//glBindBuffer(GL_ARRAY_BUFFER, UVBO);
		//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// set texture sampler to use texture unit 0
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, textureID);
		//shader->setUniform1i("textureSampler", 0);
		//glUniform1i(textureSamplerID, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		//glDisableVertexAttribArray(2);
	}

	void cleanup()
	{
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &CBO);
		glDeleteBuffers(1, &EBO);
		//glDeleteBuffers(1, &UVBO);
		glDeleteVertexArrays(1,&VAO);
	}
};

struct TileManager
{
	std::map<std::pair<int,int>, Tile> tiles;
	int tileDistance = 3;
	void initialise()
	{
		// add textures here too
	}

	void updateTiles(glm::vec3 playerPosition, GLuint shaderID)
	{
		std::cout << "Box position: " << playerPosition.x << ", " << playerPosition.z << std::endl;
		std::cout << "Tile size: " << tileSize << std::endl;

		int playerTileX = std::floor(playerPosition.x / tileSize);
		int playerTileZ = std::floor(playerPosition.z / tileSize);

		std::cout << "Player tile coords: " << playerTileX << ", " << playerTileZ << std::endl;

		std::map<std::pair<int,int>, Tile> tilesToRemove;

		for (int x = playerTileX - tileDistance; x <= playerTileX + tileDistance; ++x)
		{
			for (int z = playerTileZ - tileDistance; z <= playerTileZ + tileDistance; ++z)
			{
				// for curr tile at x,z
				std::pair<int, int> tileKey = std::make_pair(x, z);
				std::cout << "Checking tile: " << x/tileSize << ", " << z/tileSize << std::endl;

				if (tiles.find(tileKey) == tiles.end()) // not found-> make the tile
				{
					std::cout << "Spawning tile at " << x/tileSize << ", " << z/tileSize << std::endl;

					Tile tile;
					tile.initialise(glm::vec3(x * tileSize, 0.0f, z * tileSize), shaderID);
					tiles[tileKey] = tile;

					std::cout << "Spawned tile at " << x/tileSize << ", " << z/tileSize << std::endl;
				}
			}
		}

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

	void renderTiles(glm::mat4 cameraMatrix)
	{
			for (auto& [pos, tile] : tiles)
			{
				tile.render(cameraMatrix);
				std::cout << "Rendering tile at " << tile.position.x/tileSize << ", " << tile.position.z/tileSize << std::endl;
			}


	}

	void cleanup()
	{
		for (auto& [pos, tile] : tiles)
			tile.cleanup();

		tiles.clear();
	}
};

Box b;
//Plane p;

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
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	// Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	GLuint objectShader = LoadShadersFromFile("../object.vert", "../object.frag");
	if (objectShader == 0) std::cerr << "Failed to load object shader." << std::endl;

	// Background
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// shaders, objects, set up rest here
	glm::float32 FoV = 60;
	glm::float32 zNear = 0.1f;
	glm::float32 zFar = 1000.0f;
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, zNear, zFar);

	//p.initialise(glm::vec3(0,0,0), glm::vec3(100,1,100), objectShader);
	TileManager t;
	t.initialise();
	b.initialize(glm::vec3(5,5,5), glm::vec3(0,0,0), objectShader);

	//Tile tile;
	//tile.initialise(glm::vec3(0,0,0));
// render
		eye_center = b.position + glm::vec3(-10, 20, 70);
		lookat = b.position;

	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// calculate viewMatrix and vp
		glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
		glm::mat4 vp = projectionMatrix * viewMatrix;

		t.updateTiles(b.position, objectShader);

		glDepthMask(GL_TRUE);
		// render stuff here
		//p.render(vp, objectShader);
		b.render(vp);
		//tile.render(vp, objectShader);
		t.renderTiles(vp);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Clean up
	//p.cleanup();
	b.cleanup();
	//tile.cleanup();
	t.cleanup();

	glDeleteProgram(objectShader);
	// Close OpenGL window and terminate GLFW
	glfwTerminate();
    return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		if (b.position.z > -100.0f+b.scale.z)
			b.position.z -= 1.0f;
	}

	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		if (b.position.z < (100.0f-b.scale.z))
			b.position.z += 1.0f;
	}

	if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		if (b.position.x > -100.0f+b.scale.x)
			b.position.x -= 1.0f;
	}
	if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		if (b.position.x < (100.0f-b.scale.x))
			b.position.x += 1.0f;
	}
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}