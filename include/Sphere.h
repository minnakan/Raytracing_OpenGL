#ifndef SPHERE_H
#define SPHERE_H

#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>

#include <glm/gtc/type_ptr.hpp>

#define M_PI 3.14159265358979323846

class Sphere {
public:
    Sphere(const glm::vec3& position = glm::vec3(0.0f),
        float radius = 1.0f,
        unsigned int sectors = 36,
        unsigned int stacks = 18)
        : position(position), radius(radius), sectors(sectors), stacks(stacks) {
        generateVertices();
        setupBuffers();
        updateModelMatrix();
    }

    ~Sphere() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    void draw(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
        // Set transformation matrices in shader
        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void setPosition(const glm::vec3& newPosition) {
        position = newPosition;
        updateModelMatrix();
    }

    const glm::vec3& getPosition() const {
        return position;
    }

    void setScale(float scale) {
        radius = scale;
        updateModelMatrix();
    }

private:
    glm::vec3 position;
    float radius;
    unsigned int sectors;
    unsigned int stacks;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO;
    glm::mat4 modelMatrix;

    void updateModelMatrix() {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(radius));
    }

    void generateVertices() {
        float sectorStep = 2 * M_PI / sectors;
        float stackStep = M_PI / stacks;

        // Generate vertices
        for (unsigned int i = 0; i <= stacks; ++i) {
            float stackAngle = M_PI / 2 - i * stackStep;  // starting from pi/2 to -pi/2
            float xy = radius * cosf(stackAngle);
            float z = radius * sinf(stackAngle);

            for (unsigned int j = 0; j <= sectors; ++j) {
                float sectorAngle = j * sectorStep;  // starting from 0 to 2pi

                // Vertex position
                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);

                // Normal vector (normalized)
                vertices.push_back(x / radius);
                vertices.push_back(y / radius);
                vertices.push_back(z / radius);
            }
        }

        // Generate indices
        for (unsigned int i = 0; i < stacks; ++i) {
            unsigned int k1 = i * (sectors + 1);
            unsigned int k2 = k1 + sectors + 1;

            for (unsigned int j = 0; j < sectors; ++j, ++k1, ++k2) {
                if (i != 0) {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                if (i != (stacks - 1)) {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }
    }

    void setupBuffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // Vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }
};

#endif