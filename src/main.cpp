#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "ComputeShader.h"
#include "demoShaderLoader.h"
#include "Camera.h"

const unsigned int SCR_WIDTH = 1920; //was 1024
const unsigned int SCR_HEIGHT = 1080; //was 576

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Quad vertices
float quadVertices[] = {
    // positions        // texture coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
};

// Camera data structure matching std140 layout
struct CameraData {
    glm::vec4 position;
    glm::vec4 front;
    glm::vec4 up;
    glm::vec4 right;
    glm::vec2 fovAndAspect;
    glm::vec2 padding;
};

struct AccumulationData {
    GLuint frameCount;
    GLuint padding[3];  // For std140 layout padding
};

GLuint accumulationTexture;
GLuint accumulationUBO;
AccumulationData accumulationData = { 0 };
bool shouldResetAccumulation = false;

// Mouse callback function
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

    camera.ProcessMouseMovement(xoffset, yoffset);
    shouldResetAccumulation = true;
}

// Scroll callback function
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    //camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool cameraChanged = false;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
        cameraChanged = true;
    }
        
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
        cameraChanged = true;
    }
        
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
        cameraChanged = true;
    }
        
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
        cameraChanged = true;
    }

    if (cameraChanged) {
        shouldResetAccumulation = true;
    }
        
}

void resetAccumulation() {
    accumulationData.frameCount = 0;
    glBindBuffer(GL_UNIFORM_BUFFER, accumulationUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(AccumulationData), &accumulationData);

    // Clear accumulation texture
    /*glBindTexture(GL_TEXTURE_2D, accumulationTexture);
    std::vector<float> clearColor(SCR_WIDTH * SCR_HEIGHT * 4, 0.0f);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_FLOAT, clearColor.data());*/
}

int main() {
    // Initialize GLFW and create window
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Ray Tracer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //Compute shader
    ComputeShader computeShader(RESOURCES_PATH"raytracer.cs");

    //Quad shader
    Shader quadShader;
    quadShader.loadShaderProgramFromFile(RESOURCES_PATH "vert.vert", RESOURCES_PATH "frag.frag");
    quadShader.use();

    // Create VAO for quad
    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Create texture for compute shader output
    GLuint outputTexture;
    glGenTextures(1, &outputTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


    // Create accumulation texture
    glGenTextures(1, &accumulationTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, accumulationTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindImageTexture(1, accumulationTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    // Create and setup camera UBO
    GLuint cameraUBO;
    glGenBuffers(1, &cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), nullptr, GL_DYNAMIC_DRAW);

    // Create accumulation UBO
    glGenBuffers(1, &accumulationUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, accumulationUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(AccumulationData), &accumulationData, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, accumulationUBO);

    // Get the uniform block index and bind it explicitly
    GLuint blockIndex = glGetUniformBlockIndex(computeShader.ID, "CameraBlock");
    if (blockIndex == GL_INVALID_INDEX) {
        std::cout << "Failed to find CameraBlock uniform block" << std::endl;
    }
    else {
        std::cout << "Found CameraBlock at index: " << blockIndex << std::endl;
        glUniformBlockBinding(computeShader.ID, blockIndex, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
    }

    float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window);

        // Update camera data
        CameraData cameraData;
        cameraData.position = glm::vec4(camera.Position, 1.0f);
        cameraData.front = glm::vec4(camera.Front, 0.0f);
        cameraData.up = glm::vec4(camera.Up, 0.0f);
        cameraData.right = glm::vec4(camera.Right, 0.0f);
        cameraData.fovAndAspect = glm::vec2(glm::radians(camera.Zoom), aspect);
        cameraData.padding = glm::vec2(0.0f);

        // Update camera UBO
        glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraData), &cameraData);

        if (shouldResetAccumulation) {
            resetAccumulation();
            shouldResetAccumulation = false;
        }

        accumulationData.frameCount++;
        glBindBuffer(GL_UNIFORM_BUFFER, accumulationUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(AccumulationData), &accumulationData);

        // Dispatch compute shader
        computeShader.use();
        glDispatchCompute((SCR_WIDTH + 15) / 16, (SCR_HEIGHT + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Render quad with computed texture
        glClear(GL_COLOR_BUFFER_BIT);
        quadShader.use();
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTexture);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteTextures(1, &outputTexture);
    glDeleteBuffers(1, &cameraUBO);
    glDeleteTextures(1, &accumulationTexture);
    glDeleteBuffers(1, &accumulationUBO);

    glfwTerminate();
    return 0;
}