#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>
#include <box.h>
#include <tileManager.h>

#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

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

// for lighting
glm::vec3 lightPos(0,80,-100);
glm::vec3 lightColour(0.9f, 1.0f,0.9f);
float cutoff = glm::cos(glm::radians(12.5f)); // for spotlight

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

	/*glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);*/

	// Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	// Background
	glClearColor(0.003f, 0.0025f, 0.05f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// shaders, objects, set up rest here
	glm::float32 FoV = 60;
	glm::float32 zNear = 0.1f;
	glm::float32 zFar = 1000.0f;
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, zNear, zFar);

	Shader objectShader;
	objectShader.initialise("../shaders/object.vert", "../shaders/object.frag");

	Shader lightSourceShader;
	lightSourceShader.initialise("../shaders/lightSourceBox.vert", "../shaders/lightSourceBox.frag");

	lightSourceShader.use();
	lightSourceShader.setVec3("lightColour", lightColour.r, lightColour.g, lightColour.b);

	objectShader.use();
	objectShader.setVec3("lightPos", lightPos.x, lightPos.y, lightPos.z);

	objectShader.use();
	objectShader.setFloat("cutoff", cutoff);

	objectShader.use();
	objectShader.setVec3("lightFacing", 0, -1, 0);

	objectShader.use();
	objectShader.setFloat("constant", 1.0f);

	objectShader.use();
	objectShader.setFloat("linear", 0.07f);

	objectShader.use();
	objectShader.setFloat( "quadratic", 0.017f);

	glUseProgram(0);

	TileManager t;
	t.initialise();

	Box b, b2;
	b.initialize(glm::vec3(10,10,10),lightPos,lightSourceShader);
	b2.initialize(glm::vec3(10,10,10), glm::vec3(0,0,-40), objectShader);

	float prevDeltaTime = 0.016f; // 60 fpsshader.use();

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

		glm::vec3 forwardLook = glm::normalize(front) * t.tileSize * 0.5f; // to ensure tiles in distancee are created when we get there
		glm::vec3 updatePos = camera_target + forwardLook;
		//std::cout << "deltaTime: " << deltaTime << std::endl;

		t.updateTiles(updatePos, objectShader);

		glDepthMask(GL_TRUE);
		// render stuff here
		t.renderTiles(vp, objectShader);
		b.render(vp,lightSourceShader);
		b2.render(vp, objectShader);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Clean up
	t.cleanup();
	b.cleanup();
	b2.cleanup();
	objectShader.remove();
	lightSourceShader.remove();
	// Close OpenGL window and terminate GLFW
	glfwTerminate();
    return 0;
}

// Is called whenever a key is pressed/released via GLFW
void processInput(GLFWwindow *window)
{
	float cameraSpeed = 15.0f * deltaTime;
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
	std::cout << "X-pos: " << xpos << std::endl;
	std::cout << "Y-pos: " << ypos << std::endl;

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