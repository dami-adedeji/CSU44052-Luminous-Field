//This work is based on "Green Alien" (https://sketchfab.com/3d-models/green-alien-9096e628a04242708126cf7b7c8008a1)
//      by Bogdan Streletskiy (https://sketchfab.com/streletskiy) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)
//This work is based on "Pine Tree - PS1 Low Poly" (https://sketchfab.com/3d-models/pine-tree-ps1-low-poly-d71ceeb303644e649d09fe8038aa5e47)
//      by Wersaus33 (https://sketchfab.com/wersaus33) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)
//This work is based on "Rustic_Cabin_fbx" (https://sketchfab.com/3d-models/rustic-cabin-fbx-00660493c25d48a1a1fab8ea15140cfc)
//      by Creater (https://sketchfab.com/7f5609904fe14de59b74533836ad9b) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)
//This work is based on "UFO Low Poly" (https://sketchfab.com/3d-models/ufo-low-poly-0fcfc381e84347fe834da4086883c9af)
//      by Jacobs Development (https://sketchfab.com/Jacobs_Development) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>
#include <box.h>
#include <tileManager.h>
#include <light.h>
#include <gltfModel.h>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <random>


// GLTF model loader
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

static GLFWwindow *window;
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// OpenGL camera view parameters
static glm::vec3 eye_center(0, 10 , 0);
glm::vec3 camera_target = eye_center;
static glm::vec3 lookat;
static glm::vec3 front(0,0,-1);
static glm::vec3 up(0, 1, 0);

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// for shadow
static int shadowMapWidth = 1024;
static int shadowMapHeight = 768;

static float depthFoV = 80.0f;
static float depthNear = 30.0f;
static float depthFar = 600.0f;

// Helper flag and function to save depth maps for debugging
static bool saveDepth = true;

// This function retrieves and stores the depth map of the default frame buffer
// or a particular frame buffer (indicated by FBO ID) to a PNG image.
static void saveDepthTexture(GLuint fbo, std::string filename) {
	int width = shadowMapWidth;
	int height = shadowMapHeight;
	if (shadowMapWidth == 0 || shadowMapHeight == 0) {
		width = 1024;
		height = 768;
	}
	int channels = 3;

	std::vector<float> depth(width * height);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glReadBuffer(GL_DEPTH_COMPONENT);
	glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::vector<unsigned char> img(width * height * 3);
	for (int i = 0; i < width * height; ++i) img[3*i] = img[3*i+1] = img[3*i+2] = depth[i] * 255;

	stbi_write_png(filename.c_str(), width, height, channels, img.data(), width * channels);
}

// for rotation
bool firstMouse = true;
float yaw   = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch =  0.0f;
float lastX =  800.0f / 2.0;
float lastY =  600.0 / 2.0;

float randomFloat(float min, float max) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<> dist(min, max);
	return dist(gen);
}

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

	Shader depthShader;
	depthShader.initialise("../shaders/depth.vert","../shaders/depth.frag");

	// making lights
	Light dirLight;
	dirLight.type = 0;
	dirLight.direction = glm::vec3(-0.5, -1.0, -0.3);
	dirLight.colour = glm::vec3(0.05f, 0.05f,0.1f);
	dirLight.constant = 1.0;
	dirLight.linear = 0.007;
	dirLight.quadratic = 0.0002;

	// for lighting - spotlight
	Light spotlight;
	spotlight.type = 2;
	spotlight.direction = glm::vec3(0, -1, 0);
	spotlight.position = glm::vec3(0,60,-40);//(0,100,-100);
	spotlight.colour = glm::vec3(0.75f, 6.25f, 0.5f)*glm::vec3(3.0f);//(0.3f, 2.5f, 0.2f);
	spotlight.constant = 1.0;
	spotlight.linear = 0.014;
	spotlight.quadratic = 0.0007;
	spotlight.cutoff = glm::cos(glm::radians(15.0f));
	spotlight.outerCutoff = glm::cos(glm::radians(20.0f));

	std::vector<Light> lights = {dirLight, spotlight};

	objectShader.use();
	objectShader.setInt("numLights", lights.size());
	for (int i = 0; i < lights.size(); ++i)
	{
		std::string base = "lights[" + std::to_string(i) + "]";
		objectShader.setInt((base + ".type"), lights[i].type);
		objectShader.setVec3((base + ".position"), lights[i].position);
		objectShader.setVec3((base + ".direction"), lights[i].direction);
		objectShader.setVec3((base + ".colour"), lights[i].colour);
		objectShader.setFloat((base + ".constant"), lights[i].constant);
		objectShader.setFloat((base + ".linear"), lights[i].linear);
		objectShader.setFloat((base + ".quadratic"), lights[i].quadratic);
		objectShader.setFloat((base + ".cutoff"), lights[i].cutoff);
		objectShader.setFloat((base + ".outerCutoff"), lights[i].outerCutoff);
	}

	//fog to fade out the horizon; based on cam pos
	glm::vec3 fogColour = glm::vec3(0.03f, 0.04f, 0.01f);
	objectShader.setVec3("fogColour", fogColour);
	objectShader.setFloat("fogStart", 50.0f);
	objectShader.setFloat("fogEnd", 150.0f);

	TileManager t;
	t.initialise();

	std::vector<GLTFModel> models;
	glm::mat4 transformMatrix(1.0f);
	transformMatrix = glm::mat4(1.0f);
	GLTFModel ufo("../assets/ufo-low-poly/scene.gltf");
	ufo.isAnimated = false;
	transformMatrix = glm::translate(transformMatrix, spotlight.position);
	transformMatrix = glm::scale(transformMatrix, glm::vec3(40.0f));
	//transformMatrix = glm::rotate(transformMatrix, glm::radians(-90.0f),glm::vec3(1,0,0));
	ufo.setTransform(transformMatrix);
	models.push_back(ufo);
	//}

	GLTFModel cabin("../assets/rustic-cabin/scene.gltf");
	transformMatrix = glm::mat4(1.0f);
	cabin.isAnimated = false;
	transformMatrix = glm::translate(transformMatrix, glm::vec3(0,8,-40));
	transformMatrix = glm::scale(transformMatrix, glm::vec3(10.0f));
	//transformMatrix = glm::rotate(transformMatrix, glm::radians(90.0f),glm::vec3(1,0,0));
	cabin.setTransform(transformMatrix);
	models.push_back(cabin);

	GLTFModel tree("../assets/pine_tree_-_ps1_low_poly/scene1.gltf");
	transformMatrix = glm::mat4(1.0f);
	tree.isAnimated = false;
	transformMatrix = glm::translate(transformMatrix, glm::vec3(20,0, -40));
	transformMatrix = glm::scale(transformMatrix, glm::vec3(2.0f));
	transformMatrix = glm::rotate(transformMatrix, glm::radians(-90.0f),glm::vec3(1,0,0));
	tree.setTransform(transformMatrix);
	models.push_back(tree);

	GLTFModel tree2("../assets/pine_tree_-_ps1_low_poly/scene1.gltf");
	transformMatrix = glm::mat4(1.0f);
	tree2.isAnimated = false;
	transformMatrix = glm::translate(transformMatrix, glm::vec3(-40,0, -60));
	transformMatrix = glm::scale(transformMatrix, glm::vec3(3.0f));
	transformMatrix = glm::rotate(transformMatrix, glm::radians(-90.0f),glm::vec3(1,0,0));
	tree2.setTransform(transformMatrix);
	models.push_back(tree2);

	float diffStrength = 0.1f;
	GLTFModel alien("../assets/green_alien/scene.gltf");
	alien.diffuseStrength = diffStrength;
	alien.isAnimated = true;
	transformMatrix = glm::mat4(1.0f);
	transformMatrix = glm::translate(transformMatrix, glm::vec3(0,0, -30));
	transformMatrix = glm::scale(transformMatrix, glm::vec3(0.0035f));
	alien.setTransform(transformMatrix);
	models.push_back(alien);

	//shadow fbo
	GLuint shadowFBO;
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	// shadow map
	GLuint depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = {1.0, 1.0, 1.0, 1.0};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Error: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	float prevDeltaTime = 0.016f; // 60 fpsshader.use();
	float fps = 0.0f;
	float fpsTimer = 0.0f;
	int fpsFrames = 0;

	do
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (deltaTime > 0.05f)
			deltaTime = 0.05f;

		deltaTime = glm::mix(prevDeltaTime, deltaTime, 0.1f);
		prevDeltaTime = deltaTime;

		//========= SHADOW RENDER ===============================
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);//perspective(glm::radians(depthFoV), (float)(shadowMapWidth/shadowMapHeight), depthNear, depthFar);
		glm::mat4 lightView = glm::lookAt(glm::vec3(0,8,-40) - dirLight.direction * 100.0f, glm::vec3(0,8,-40), glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		depthShader.use();
		depthShader.setMatrix("lightSpaceMatrix", &lightSpaceMatrix[0][0]);

		for (GLTFModel& m : models)
			m.renderDepth(lightSpaceMatrix, depthShader);

		//========= MAIN RENDER =============
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		processInput(window);
		float lerpSpeed = 5.0f;
		eye_center = glm::mix(eye_center, camera_target, lerpSpeed * deltaTime);

		// calculate viewMatrix and vp
		glm::mat4 viewMatrix = glm::lookAt(eye_center, eye_center + front, up);

		glm::vec3 forwardLook = glm::normalize(front) * t.tileSize * 0.5f; // to ensure tiles in distancee are created when we get there
		glm::vec3 updatePos = camera_target + forwardLook;

		// render stuff here
		objectShader.use();
		objectShader.setVec3("cameraPos", eye_center);
		objectShader.setMatrix("view", &viewMatrix[0][0]);
		objectShader.setMatrix("projection", &projectionMatrix[0][0]);
		objectShader.setMatrix("lightSpaceMatrix", &lightSpaceMatrix[0][0]);
		alien.updateAnimation(deltaTime);

		for (GLTFModel& m : models)
			m.render(objectShader, depthMap);
		t.updateTiles(updatePos, objectShader);
		t.renderTiles(viewMatrix, projectionMatrix, objectShader);

		if (saveDepth) {
			std::string filename = "depth_camera.png";
			saveDepthTexture(shadowFBO, filename);
			std::cout << "Depth texture saved to " << filename << std::endl;
			saveDepth = false;
		}

		fpsFrames++;
		fpsTimer += deltaTime;

		if (fpsTimer >= 1.0f) {
			fps = fpsFrames / fpsTimer;
			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << "CSU44052 Supplemental DA | Frames per second (FPS): " << fps;
			glfwSetWindowTitle(window, stream.str().c_str());

			fpsFrames = 0;
			fpsTimer = 0.0f;
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Clean up
	t.cleanup();
	objectShader.remove();
	depthShader.remove();
	// Close OpenGL window and terminate GLFW
	glfwTerminate();
    return 0;
}

// Is called whenever a key is pressed/released via GLFW
void processInput(GLFWwindow *window)
{
	float cameraSpeed = 15.0f * deltaTime;
	glm::vec3 move(0.0f);
	glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_REPEAT)
	{
		move += flatFront;
	}

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_REPEAT)
	{
		move -= flatFront;
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_REPEAT)
	{
		move -= glm::normalize(glm::cross(flatFront, up));
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_REPEAT)
	{
		move += glm::normalize(glm::cross(flatFront, up));
	}
	if ((glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_REPEAT)
		&& eye_center.y < 40.0f/*+ cameraSpeed <= 20.0f*/)
	{
		move += glm::vec3(0.0f, 1.0f, 0.0f);
	}
	if ((glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_REPEAT)
		&& eye_center.y > 5.0f/*- cameraSpeed >= 0.0f*/)
	{
		move -= glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (glm::length(move) > 0.0f)
		camera_target += glm::normalize(move) * cameraSpeed;
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

    float sensitivity = 0.1f;
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