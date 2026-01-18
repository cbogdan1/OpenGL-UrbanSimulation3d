#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types
#include <cstdlib> // pentru rand()
#include <glm/gtc/constants.hpp> // pentru glm::half_pi

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"
#include <iostream>
// skybox
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;
gps::Shader particleShader;
gps::Shader reflectionShader;


// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir = glm::vec3(0.0f, 0.0f, 0.0f); // Soarele
glm::vec3 lightColorDir = glm::vec3(0.0f, 0.0f, 0.0f); // Lumina gri-alb (mai slaba)

glm::vec3 lightPos = glm::vec3(0.0f, 1.0f, 0.0f); // Becul
glm::vec3 lightColorPoint = glm::vec3(1.0f, 0.6f, 0.0f); // Lumina portocalie (puternica)
// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc, lightColorDirLoc;

// camera
gps::Camera myCamera(
    glm::vec3(10.0f, 10.0f, 10.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));


GLboolean pressedKeys[1024];

// models
gps::Model3D teapot;
gps::Model3D car;
gps::Model3D building;
gps::Model3D house;
gps::Model3D road;
gps::Model3D grass;
gps::Model3D fence;
gps::Model3D bench;
gps::Model3D garage;
gps::Model3D garage_door;
gps::Model3D wheel;
gps::Model3D lamp;
gps::Model3D tree;
gps::Model3D straight_street;
gps::Model3D bridge;
GLfloat angle;

// shaders
gps::Shader myBasicShader;

GLuint floorVAO, floorVBO;
GLint objectColorLoc;


// mouse control
bool firstMouse = true;
float lastX = 512.0f; // jumatate din latimea ferestrei (1024/2)
float lastY = 384.0f; // jumatate din inaltimea ferestrei (768/2)
float yaw = -90.0f;   // initializat la -90 ca sa privim spre -Z
float pitch = 0.0f;
float sensitivity = 0.1f;


GLuint floorTexture;
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096; // Rezolutie umbre

gps::Shader depthMapShader;



bool inCarMode = true;       // Pornim direct in masina (modifica pe false daca vrei invers)

glm::vec3 carPosition = glm::vec3(0.0f, 0.0f, 10.0f); // Pozitia initiala a masinii
float carAngle = 90.0f;     // Rotatia masinii (0 grade = spre -Z)

float currentCarSpeed = 0.0f; // Viteza actuala (poate fi pozitiva sau negativa)
float maxSpeed = 50.0f;       // Viteza maxima
float acceleration = 40.0f;   // Cat de repede porneste
float deceleration = 30.0f;   // Cat de repede se opreste singura (Frecare)
float breakPower = 80.0f;     // Cat de repede franeaza cand pui contra (optional)


// Setari pentru camera cand urmareste masina
float distantaSpate = 23.0f;
float inaltimeCamera = 15.0f;

float angleAroundCar = 0.0f; // Unghiul orizontal fata de spatele masinii
float pitchCar = 20.0f;      // Unghiul vertical (cat de sus ne uitam)

float wheelAngle = 0.0f;      // Invartirea rotii (spin)
float steeringAngle = 0.0f;   // Unghiul de viraj al rotilor fata (stanga-dreapta)
float maxSteeringAngle = 35.0f; // Cat de mult poti vira maxim

bool isGarageOpen = false;
float garageDoorAngle = 0.0f;

// Timpul ultimului cadru
double lastTimeStamp = 0.0;
// Variabila globala pe care o vor folosi toate functiile (viteza, rotatie)
float deltaTime = 0.0f;
void updateDelta(double elapsedSeconds) {
    // Aici poti inmulti cu o constanta daca vrei slow-motion sau fast-forward
    deltaTime = (float)elapsedSeconds;
}


bool presentationMode = false; // Daca suntem in modul animatie
float presentationTime = 0.0f; // Cronometrul animatiei

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};
std::vector<BoundingBox> obstacles;
BoundingBox garageDoorBox;
bool showBoundingBoxes = false;
GLuint debugVAO = 0;
GLuint debugVBO = 0;

struct Particle {
    glm::vec3 position; // Unde se afla
    glm::vec3 velocity; // Directia in care merge (sus si in spate)
    float life;         // Cat timp mai traieste (1.0 = noua, 0.0 = moarta)
    float size;         // Cat de mare e
};

std::vector<Particle> smokeParticles;
GLuint smokeTexture;
GLuint smokeVAO, smokeVBO;
float smokeSpawnTimer = 0.0f;

GLuint ReadTextureFromFile(const char* file_name) {

    int x, y, n;
    int force_channels = 4;
    unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);

    if (!image_data) {
        fprintf(stderr, "ERROR: could not load %s\n", file_name);
        return false;
    }
    // NPOT check
    if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
        fprintf(
            stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name
        );
    }

    int width_in_bytes = x * 4;
    unsigned char* top = NULL;
    unsigned char* bottom = NULL;
    unsigned char temp = 0;
    int half_height = y / 2;

    for (int row = 0; row < half_height; row++) {

        top = image_data + row * width_in_bytes;
        bottom = image_data + (y - row - 1) * width_in_bytes;

        for (int col = 0; col < width_in_bytes; col++) {

            temp = *top;
            *top = *bottom;
            *bottom = temp;
            top++;
            bottom++;
        }
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_SRGB, //GL_SRGB,//GL_RGBA,
        x,
        y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image_data
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}
void initShadowMap() {
    glGenFramebuffers(1, &shadowMapFBO);

    // Cream textura de adancime
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Culoarea bordurii (alb) pentru a nu avea umbre in afara hartii
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Atasam textura la FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    // Nu avem nevoie de culori, doar adancime
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void initSkyBox() {
    std::vector<const GLchar*> faces;
    // Ordinea este CRITICA: Right, Left, Top, Bottom, Back, Front
 /*   faces.push_back("skybox/skybox1/right.tga");
    faces.push_back("skybox/skybox1/left.tga");
    faces.push_back("skybox/skybox1/top.tga");
    faces.push_back("skybox/skybox1/bottom.tga");
    faces.push_back("skybox/skybox1/back.tga");
    faces.push_back("skybox/skybox1/front.tga");*/

    faces.push_back("skybox/skybox2/px.png"); // Right
    faces.push_back("skybox/skybox2/nx.png"); // Left
    faces.push_back("skybox/skybox2/py.png"); // Top
    faces.push_back("skybox/skybox2/ny.png"); // Bottom
    faces.push_back("skybox/skybox2/pz.png"); // Back
    faces.push_back("skybox/skybox2/nz.png"); // Front


    mySkyBox.Load(faces);
}
// Functie simpla de intersectie AABB (Cutie cu Cutie)
bool checkCollision(BoundingBox a, BoundingBox b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
        (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
        (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// Helper ca sa cream o cutie doar stiind centrul si marimea (latime/inaltime/adancime)
BoundingBox createBox(glm::vec3 center, glm::vec3 size) {
    BoundingBox box;
    box.min = center - size * 0.5f; // Jumatate din marime in stanga/jos/spate
    box.max = center + size * 0.5f; // Jumatate din marime in dreapta/sus/fata
    return box;
}
GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
    glViewport(0, 0, width, height);
    if (height == 0) height = 1; // evitare impartire 0 daca se minimizeaza

    projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 5500.0f);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        // Modul Wireframe (doar liniile)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        // Modul Poligonal / Puncte (doar varfurile)
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        // marime puncte
        glPointSize(3.0f);
    }

    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        // solid
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        inCarMode = !inCarMode; // Inversam starea (true -> false sau false -> true)

        if (inCarMode) {
            std::cout << "Mod: CAR CONTROL" << std::endl;
        }
        else {
            std::cout << "Mod: FREE CAMERA" << std::endl;
        }
    }
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        presentationMode = !presentationMode; // Start/Stop
        if (presentationMode) {
            presentationTime = 0.0f; // Resetam cronometrul cand pornim
            inCarMode = false; // Iesim din masina ca sa nu facem accident
            std::cout << "Mod: PRESENTATION START" << std::endl;
        }
        else {
            std::cout << "Mod: PRESENTATION STOP" << std::endl;
        }
    }
    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        isGarageOpen = !isGarageOpen; // Comuta intre deschis/inchis
    }
    if (key == GLFW_KEY_B && action == GLFW_PRESS) {
        showBoundingBoxes = !showBoundingBoxes;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    if (inCarMode) {
        // control masina
        // Mouse Stanga/Dreapta roteste in jurul masinii
        angleAroundCar -= xoffset;

        // Mouse Sus/Jos ridica sau coboara camera
        pitchCar -= yoffset;

        // Limitari pentru a nu intra sub pamant sau a nu da peste cap camera 
        // Axa de deasupra masinii
        if (pitchCar > 60.0f) pitchCar = 60.0f;
        if (pitchCar < 5.0f)  pitchCar = 5.0f; // Minim 5 grade sa nu intre in asfalt
    }
    else {
        // control free-cam
        yaw += xoffset;
        pitch += yoffset;
        //blocare sa nu se dea peste cap
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        myCamera.rotate(pitch, yaw);
    }
}

void processMovement() {
    //******************
    //ANIMATIE USA GARAJ
    //******************
    float doorSpeed = 100.0f * deltaTime;
    if (isGarageOpen) {
        if (garageDoorAngle < 90.0f) garageDoorAngle += doorSpeed;  //deschidere
    }
    else {
        if (garageDoorAngle > 0.0f) garageDoorAngle -= doorSpeed; //inchidere
    }
    //*****************************
    //  ANIMATIE PREZENTARE SCENA
    //*****************************
    if (presentationMode) {
        presentationTime += deltaTime;

        // Definire puncte din prezentare
        glm::vec3 centerHouse = glm::vec3(40.0f, 5.0f, -70.0f);
        glm::vec3 centerBuildings = glm::vec3(0.0f, 5.0f, 0.0f);
        glm::vec3 centerPark = glm::vec3(18.0f, 5.0f, 70.0f);

        glm::vec3 camPos, camTarget;

        if (presentationTime < 8.0f) { //rotire casa mare
            float radius = 45.0f; float speed = 0.5f;
            camPos.x = centerHouse.x + sin(presentationTime * speed) * radius;
            camPos.z = centerHouse.z + cos(presentationTime * speed) * radius;
            camPos.y = 20.0f;
            camTarget = centerHouse;
        }
        else if (presentationTime < 12.0f) { //trage camera in spre urmatorul obiect de prezentat
            float t = (presentationTime - 8.0f) / 4.0f;
            glm::vec3 startPos = centerHouse + glm::vec3(0, 5, 25);
            glm::vec3 endPos = centerBuildings + glm::vec3(0, 15, 30);
            camPos = glm::mix(startPos, endPos, t); //interploare din cele 3 puncte
            camTarget = glm::mix(centerHouse, centerBuildings, t);//interploare din cele 3 puncte
        }
        else if (presentationTime < 20.0f) { //prezentare rotire urmatorul set obiecte(3 blocuri)
            float radius = 45.0f; float speed = 0.6f; float localTime = presentationTime - 12.0f;
            camPos.x = centerBuildings.x + sin(localTime * speed) * radius;
            camPos.z = centerBuildings.z + cos(localTime * speed) * radius;
            camPos.y = 20.0f;
            camTarget = centerBuildings;
        }
        else if (presentationTime < 24.0f) { //mutare camera din nou
            float t = (presentationTime - 20.0f) / 4.0f;
            glm::vec3 startPos = centerBuildings + glm::vec3(0, 15, 45);
            glm::vec3 endPos = centerPark + glm::vec3(15, 5, 0);
            camPos = glm::mix(startPos, endPos, t);
            camTarget = glm::mix(centerBuildings, centerPark, t);
        }
        else if (presentationTime < 32.0f) { //rotire din nou (jurul parcului)
            float radius = 30.0f; float speed = 0.5f; float localTime = presentationTime - 24.0f;
            camPos.x = centerPark.x + cos(localTime * speed) * radius;
            camPos.z = centerPark.z + sin(localTime * speed) * radius;
            camPos.y = 10.0f;
            camTarget = centerPark;
        }
        else {
            presentationMode = false; // Stop
        }
        //formeaza coordonatele camerei cu look at si trimite prin uniform la shader
        view = glm::lookAt(camPos, camTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        return; // Iesim, nu procesam control manual
    }

    //************************************************
    //  LOGICA FIZICA (CONTROL UTILIZATOR + INERTIE)
    //************************************************

    // Variabile locale pentru inputs
    float turnSpeed = 4.0f * deltaTime;
    float steerInputSpeed = 200.0f * deltaTime;

    if (inCarMode) {
        // directia volan
        if (pressedKeys[GLFW_KEY_A]) {
            steeringAngle += steerInputSpeed;
            if (steeringAngle > maxSteeringAngle) steeringAngle = maxSteeringAngle;
        }
        else if (pressedKeys[GLFW_KEY_D]) {
            steeringAngle -= steerInputSpeed;
            if (steeringAngle < -maxSteeringAngle) steeringAngle = -maxSteeringAngle;
        }
        else {
            // Revenire automata la centru dupa ce se elibereaza tastele
            if (steeringAngle > 1.0f) steeringAngle -= steerInputSpeed;
            else if (steeringAngle < -1.0f) steeringAngle += steerInputSpeed;
            else steeringAngle = 0.0f;
        }

        // logica de aceleratie /inertie

        // calcul viteza Inputs
        if (pressedKeys[GLFW_KEY_W]) {
            // Daca mergem cu spatele si apasam W ==> franare
            if (currentCarSpeed < -0.1f) currentCarSpeed += breakPower * deltaTime;
            else currentCarSpeed += acceleration * deltaTime;
        }
        else if (pressedKeys[GLFW_KEY_S]) {
            // Daca mergem cu fata si apasam S -> Franare puternica
            if (currentCarSpeed > 0.1f) currentCarSpeed -= breakPower * deltaTime;
            else currentCarSpeed -= acceleration * deltaTime;
        }
        else {
            // Nicio tasta apasata -> Frecare / Inertie
            if (currentCarSpeed > 0.1f) currentCarSpeed -= deceleration * deltaTime;
            else if (currentCarSpeed < -0.1f) currentCarSpeed += deceleration * deltaTime;
            else currentCarSpeed = 0.0f;
        }

        // limitare viteza maxima
        if (currentCarSpeed > maxSpeed) currentCarSpeed = maxSpeed;
        if (currentCarSpeed < -maxSpeed) currentCarSpeed = -maxSpeed;

        // miscare doar daca viteza e suficient de mare
        if (abs(currentCarSpeed) > 0.01f) {

            // calcul cat se misca in acest frame
            float moveStep = currentCarSpeed * deltaTime;

            // calcul pt pozitia viitoare
            glm::vec3 nextPos = carPosition;
            nextPos.x += sin(glm::radians(carAngle)) * moveStep;
            nextPos.z += cos(glm::radians(carAngle)) * moveStep;

            // verificare coliziuni
            bool collision = false;

            // limitele hartii
            if (nextPos.x > 200.0f || nextPos.x < -200.0f || nextPos.z > 200.0f || nextPos.z < -200.0f) {
                collision = true;
            }

            // alte obstacole
            if (!collision) {
                // Dimensiunea bounding box-ului masinii (conform discutiei)
                glm::vec3 carSize = glm::vec3(2.5f, 2.8f, 4.0f);
                BoundingBox nextCarBox = createBox(nextPos, carSize);

                // Obstacole statice
                for (const auto& obstacle : obstacles) {
                    if (checkCollision(nextCarBox, obstacle)) {
                        collision = true;
                        break;
                    }
                }
                // Usa Garaj (Doar daca nu e complet deschisa)
                if (!collision && garageDoorAngle < 70.0f) {
                    if (checkCollision(nextCarBox, garageDoorBox)) {
                        collision = true;
                    }
                }
            }

            // schimbare pozitie daca nu sunt coliziuni
            if (!collision) {
                // E liber, actualizam pozitia
                carPosition = nextPos;

                // Rotim rotile (vizual) in functie de viteza
                float wheelRotationFactor = 25.0f;
                if (currentCarSpeed > 0) wheelAngle += currentCarSpeed * wheelRotationFactor * deltaTime;
                else wheelAngle += currentCarSpeed * wheelRotationFactor * deltaTime;

                // Rotim masina (Heading) doar daca avem viteza
                float turnFactor = (currentCarSpeed / maxSpeed);
                carAngle += steeringAngle * turnSpeed * turnFactor;

            }
            else {
                // coliziune ==> oprire instanta
                currentCarSpeed = 0.0f;
            }
        }

        // update camera third persone
        float horizDistance = distantaSpate * cos(glm::radians(pitchCar));
        float vertDistance = distantaSpate * sin(glm::radians(pitchCar));
        float theta = glm::radians(carAngle + 180.0f + angleAroundCar);

        glm::vec3 cameraPos;
        cameraPos.x = carPosition.x - horizDistance * sin(theta);
        cameraPos.z = carPosition.z - horizDistance * cos(theta);
        cameraPos.y = carPosition.y + vertDistance;

        glm::vec3 cameraTarget = carPosition + glm::vec3(0.0f, 1.5f, 0.0f);
        view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    }
    else {
        // free-cam mode
        float currentCameraSpeed = 50.0f * deltaTime;
        if (pressedKeys[GLFW_KEY_W]) myCamera.move(gps::MOVE_FORWARD, currentCameraSpeed);
        if (pressedKeys[GLFW_KEY_S]) myCamera.move(gps::MOVE_BACKWARD, currentCameraSpeed);
        if (pressedKeys[GLFW_KEY_A]) myCamera.move(gps::MOVE_LEFT, currentCameraSpeed);
        if (pressedKeys[GLFW_KEY_D]) myCamera.move(gps::MOVE_RIGHT, currentCameraSpeed);
        view = myCamera.getViewMatrix();
    }

    // ceainic
    if (pressedKeys[GLFW_KEY_Q]) angle -= 60.0f * deltaTime;
    if (pressedKeys[GLFW_KEY_E]) angle += 60.0f * deltaTime;

    // Trimitem matricele actualizate
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
}



void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
    glfwSetFramebufferSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.1f, 0.5f, 0.5f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}
void initFloor() {
    // Pozitie (x,y,z), Normala (nx,ny,nz), Coordonate Textura (u,v)
GLfloat floorVertices[] = {
        // --- TRIUNGHIUL 1 ---
        // Stanga-Jos
        -1000.0f, 0.0f,  1000.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        // Dreapta-Jos
         1000.0f, 0.0f,  1000.0f,  0.0f, 1.0f, 0.0f,  1000.0f, 0.0f,
         // Dreapta-Sus
          1000.0f, 0.0f, -1000.0f,  0.0f, 1.0f, 0.0f,  1000.0f, 1000.0f,

          // --- TRIUNGHIUL 2 ---
          // Dreapta-Sus
           1000.0f, 0.0f, -1000.0f,  0.0f, 1.0f, 0.0f,  1000.0f, 1000.0f,
           // Stanga-Sus
           -100.0f, 0.0f, -1000.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1000.0f,
           // Stanga-Jos
           -1000.0f, 0.0f,  1000.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f
    };
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

    // Atributele (trebuie sa se potriveasca cu layout-ul din basic.vert)
    // 1. Pozitia
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 2. Normala
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 3. Coordonate Textura (UV) 
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void initModels() {
    teapot.LoadModel("models/teapot/teapot20segUT.obj");
	car.LoadModel("models/car/car_body.obj");
	building.LoadModel("models/building/13940_New_York_City_Brownstone_Building_v1_l2.obj");
	house.LoadModel("models/house/two_story_villa_house.obj");
	road.LoadModel("models/road/10561_RoadSection90_v1-L3.obj");
	grass.LoadModel("models/grass/10450_Rectangular_Grass_Patch_v1_iterations-2.obj");
    fence.LoadModel("models/fence/fence01.obj");
    bench.LoadModel("models/bench/Bench.obj");
	wheel.LoadModel("models/car/wheel.obj");
	lamp.LoadModel("models/lamp/Street_Lamp_7.obj");
	tree.LoadModel("models/tree/Tree.obj");
    straight_street.LoadModel("models/street1/rua para blender/untitled.obj");
	garage.LoadModel("models/garage/garaj_fara_usa.obj");
    garage_door.LoadModel("models/garage/usa_garaj.obj");
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert","shaders/basic.frag");
    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
	skyboxShader.loadShader("shaders/skyBox.vert", "shaders/skyBox.frag");
    reflectionShader.loadShader("shaders/reflection.vert", "shaders/reflection.frag");
    particleShader.loadShader("shaders/particle.vert", "shaders/particle.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 500.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(5.0f, 10.0f, 5.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    glm::vec3 lightColorDir = glm::vec3(0.1f, 0.1f, 0.1f);
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    lightColorDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColorDir");
    glUniform3fv(lightColorDirLoc, 1, glm::value_ptr(lightColorDir));

    // LUMINI POZITIONALE
   // pozitie becuri
    glm::vec3 pointLightPositions[] = {
        glm::vec3(-4.0f, 15.0f, 0.0f),
        glm::vec3(-4.0f,  10.0f, 0.0f),
        glm::vec3(-4.0f, 15.0f,  -20.0f), 
        glm::vec3(-4.0f,   10.0f,    20.0f),   
        glm::vec3(40.5f, 11.6f, -67.15f),  
        glm::vec3(17.0f, 6.2f, 70.0f) 
    };

    // culori pentru becuri
    glm::vec3 pointLightColors[] = {
        glm::vec3(.9f, 0.6f, 0.0f), // Portocaliu (original)
        glm::vec3(.9f, 0.6f, 0.0f),
        glm::vec3(.9f, 0.6f, 0.0f),
        glm::vec3(.9f, 0.6f, 0.0f),
        glm::vec3(.9f, 0.6f, 0.0f),
        glm::vec3(.9f, 0.6f, 0.0f)
    };

    // trimitere pos+color prin uniform la shader
    for (unsigned int i = 0; i < 6; i++) {
        std::string number = std::to_string(i);
        // Pozitia
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, ("lightPos[" + number + "]").c_str()),1, glm::value_ptr(pointLightPositions[i]));
        // Culoarea
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, ("lightColorPoint[" + number + "]").c_str()),1, glm::value_ptr(pointLightColors[i]));
    }


    initFloor();
    floorTexture = ReadTextureFromFile("models/floor_texture/natural-stone-aged-paviment.jpg");
}
void RenderModel(gps::Model3D& modelObj, gps::Shader shader, glm::vec3 position, glm::vec3 scale = glm::vec3(1.0f), float rotationAngle = 0.0f, glm::vec3 rotate = glm::vec3(1.0f)) {
    shader.useShaderProgram();

    // 1. Matricea Model
    glm::mat4 model = glm::mat4(1.0f);

    // Translatie 
    model = glm::translate(model, position);

    // Rotatie pe axa Y (optionala)
    model = glm::rotate(model, glm::radians(rotationAngle), rotate);

    // Scalare (optional)
    model = glm::scale(model, scale);

    // 2. Trimitem matricea Model la shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // 3. Desenare obiect
    modelObj.Draw(shader);
}
void RenderModelWithReflection(gps::Model3D& modelObj, gps::Shader shader, glm::vec3 position, glm::vec3 scale, float rotationAngle = 0.0f, glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f)) {
    shader.useShaderProgram();

    //1.Construire Matricea Model
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationAngle), rotationAxis);
    model = glm::scale(model, scale);

    // Trimitere matricea Model la shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // 2. Trimitere view si projection
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // 3. Trimitem Pozitia Camerei
    // Reflexia depinde de unghiul din care privesti
    // Daca nu ai functia getCameraPosition(), foloseste: glm::vec3(glm::inverse(view)[3])
    glUniform3fv(glGetUniformLocation(shader.shaderProgram, "cameraPos"), 1, glm::value_ptr(myCamera.getCameraPosition()));

    // 4. Trimitere Textura Skybox
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mySkyBox.GetTextureId());
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "skybox"), 0);

    // 5. Desenare model
    modelObj.Draw(shader);
}
void renderTeapot(gps::Shader shader) {
    shader.useShaderProgram();
    glm::mat4 modelTeapot = glm::mat4(1.0f);
    modelTeapot = glm::translate(modelTeapot, glm::vec3(19.8f, 1.3f, 58.5f));
	modelTeapot = glm::rotate(modelTeapot, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelTeapot = glm::scale(modelTeapot, glm::vec3(0.5f, 0.5f, 0.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelTeapot));
    glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * modelTeapot));
    glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
    teapot.Draw(shader);
}
void renderFloor(gps::Shader shader) {
    shader.useShaderProgram();

    // 1. Matricea Model
    glm::mat4 modelFloor = glm::mat4(1.0f);

    // 2. Trimitere la shaderul curent
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelFloor));

    glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * modelFloor));
    glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Textura
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "diffuseTexture"), 0);

    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
void renderCar(gps::Shader shader) {
    shader.useShaderProgram();
    GLint specLoc = glGetUniformLocation(shader.shaderProgram, "specularStrength");
    glUniform1f(specLoc, 1.0f);
    // 1. Caroserie
    glm::mat4 modelCar = glm::mat4(1.0f);
    modelCar = glm::translate(modelCar, carPosition);
    modelCar = glm::rotate(modelCar, glm::radians(carAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelCar = glm::rotate(modelCar, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    modelCar = glm::rotate(modelCar, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    modelCar = glm::scale(modelCar, glm::vec3(0.02f));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelCar));
    glm::mat3 normalMatrixCar = glm::mat3(glm::inverseTranspose(view * modelCar));
    glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixCar));

    car.Draw(shader);

	// Setare specularitatea mai mica pentru roti (reflexie redusa fata de caroserie)
    glUniform1f(specLoc, 0.1f);
    // 2. Pozitie roti
    glm::vec3 wheelOffsets[] = {
        glm::vec3(1.5f, 0.567f,  2.158f),   // Fata Stanga
        glm::vec3(-1.5f, 0.567f,  2.158f),  // Fata Dreapta
        glm::vec3(-1.5f, 0.567f, -2.598f),  // Spate Stanga
        glm::vec3(1.5f, 0.567f, -2.598f), // Spate Dreapta
    };

    for (int i = 0; i < 4; i++) {
        glm::mat4 modelWheel = glm::mat4(1.0f);

        // A. Pozitia Masinii
        modelWheel = glm::translate(modelWheel, carPosition);

        // B. Rotatia Masinii (Heading)
        modelWheel = glm::rotate(modelWheel, glm::radians(carAngle), glm::vec3(0, 1, 0));

        // C. Offset spre pozitia rotii
        modelWheel = glm::translate(modelWheel, wheelOffsets[i]);

        // D. Viraj (Doar fata)
        if (i < 2) {
            modelWheel = glm::rotate(modelWheel, glm::radians(steeringAngle), glm::vec3(0, 1, 0));
        }

        // E. Rotatia de spin (inaintea rotatiei de asezare in plan)
        modelWheel = glm::rotate(modelWheel, glm::radians(wheelAngle), glm::vec3(1, 0, 0));

        // F. Reasezarea rotilor
        modelWheel = glm::rotate(modelWheel, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        modelWheel = glm::rotate(modelWheel, glm::radians(90.0f), glm::vec3(0, 0, 1));
        if (i == 1 || i == 2) { // Dreapta
            modelWheel = glm::rotate(modelWheel, glm::radians(-180.0f), glm::vec3(0, 0, 1));
        }

        // G. Scalare
        modelWheel = glm::scale(modelWheel, glm::vec3(0.02f));

        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelWheel));
        glm::mat3 normalMatrixWheel = glm::mat3(glm::inverseTranspose(view * modelWheel));
        glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixWheel));

        wheel.Draw(shader);
    }
}
void renderGarageDoor(gps::Shader shader) {
    glDisable(GL_CULL_FACE);

    // Valoare de la 0.0 la 1.0
    float normalizedTime = garageDoorAngle / 90.0f;
    // 1. RIDICARE (UP)
    float slideUp = normalizedTime * 4.2f;

    // 2. IESIRE IN AFARA (OUT)
    float slideOut = normalizedTime * 2.5f;

    glm::vec3 basePos = glm::vec3(98.75f, 0.9f, 4.8f);

    // CALCUL POZITIE 
    // basePos.y + slideUp  -> O ridic?m
    // basePos.z - slideOut -> O tragem în FA?A garajului (Sc?dem Z)
    glm::vec3 animPos = glm::vec3(basePos.x, basePos.y + slideUp, basePos.z - slideOut);

    // RANDARE
    // +garageDoorAngle pentru rotire spre exterior
    RenderModel(garage_door, shader, animPos, glm::vec3(2.4f), garageDoorAngle, glm::vec3(1.0f, 0.0f, 0.0f));

    glEnable(GL_CULL_FACE);
}
void RenderModelRotated(gps::Model3D& modelObj, gps::Shader shader, glm::vec3 position, glm::vec3 scale, float rotX=0.0f, float rotY= 0.0f, float rotZ= 0.0f) {
    shader.useShaderProgram();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // Aplicam rotatiile pe rand
    if (rotX != 0.0f) model = glm::rotate(model, glm::radians(rotX), glm::vec3(1.0f, 0.0f, 0.0f));
    if (rotY != 0.0f) model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
    if (rotZ != 0.0f) model = glm::rotate(model, glm::radians(rotZ), glm::vec3(0.0f, 0.0f, 1.0f));

    model = glm::scale(model, scale);

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // variabila globala 'view' pentru normal matrix
    glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    modelObj.Draw(shader);
}
std::vector<glm::vec3> treePositions;
void initRandomTrees() {
    for (int i = 0; i < 15; i++) {

        float rX = -21.0f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (15.0f - (-21.0f))));

        float rZ = 74.0f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (94.0f - 74.0f)));

        // adaugare in lista (Y e 0.0f la sol)
        treePositions.push_back(glm::vec3(rX, 0.0f, rZ));
    }
}
void initObstacles() {
    obstacles.clear();

    // 1. Copacii
    for (const auto& pos : treePositions) {
        obstacles.push_back(createBox(pos, glm::vec3(2.0f, 10.0f, 2.0f)));
    }

    // 2. Casa 
    obstacles.push_back(createBox(glm::vec3(40.0f, 5.0f, -76.0f), glm::vec3(35.0f, 30.0f, 30.0f)));
    obstacles.push_back(createBox(glm::vec3(84.0f, 5.0f, 132.0f), glm::vec3(35.0f, 30.0f, 30.0f)));

    // 3. Cladirile
    obstacles.push_back(createBox(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(8.0f, 40.0f, 8.0f)));
    obstacles.push_back(createBox(glm::vec3(0.0f, 0.0f, -20.0f), glm::vec3(8.0f, 40.0f, 8.0f)));
    obstacles.push_back(createBox(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(8.0f, 40.0f, 8.0f)));
    obstacles.push_back(createBox(glm::vec3(-60.0f, 0.0f, 0.0f), glm::vec3(8.0f, 40.0f, 8.0f)));
    obstacles.push_back(createBox(glm::vec3(-60.0f, 0.0f, -20.0f), glm::vec3(8.0f, 40.0f, 8.0f)));
    obstacles.push_back(createBox(glm::vec3(-130.0f, 0.0f, 80.0f), glm::vec3(8.0f, 40.0f, 8.0f)));
    obstacles.push_back(createBox(glm::vec3(-130.0f, 0.0f, 100.0f), glm::vec3(8.0f, 40.0f, 8.0f)));
    obstacles.push_back(createBox(glm::vec3(-130.0f, 0.0f, 120.0f), glm::vec3(8.0f, 40.0f, 8.0f)));
    // 4. Banca si Lampa
    obstacles.push_back(createBox(glm::vec3(18.0f, 0.0f, 70.0f), glm::vec3(1.0f, 10.0f, 1.0f)));
    obstacles.push_back(createBox(glm::vec3(20.0f, 0.0f, 60.0f), glm::vec3(3.0f, 2.0f, 1.0f)));

    // 5. Gardurile
    
    // 6. Garaj
    obstacles.push_back(createBox(glm::vec3(103.0f, 2.5f, 18.0f), glm::vec3(20.0f, 5.0f, 1.0f))); //perete spate
    obstacles.push_back(createBox(glm::vec3(93.3f, 2.5f, 12.2f), glm::vec3(0.1f, 5.0f, 12.0f))); //perete dreapta
    obstacles.push_back(createBox(glm::vec3(112.3f, 2.5f, 12.2f), glm::vec3(0.1f, 5.0f, 12.0f))); //perete stanga
    obstacles.push_back(createBox(glm::vec3(107.0f, 2.5f, 4.5f), glm::vec3(9.0f, 5.0f, 1.0f)));// usa inchisa
    //usa 
    garageDoorBox = createBox(glm::vec3(98.75f, 1.5f, 4.8f), glm::vec3(5.5f, 3.0f, 0.5f));

    //aceleasi loop-uri ca la desenare
    for (int i = 0; i <= 4; i++) {
        // Gardul 1 (Axa X) 
        obstacles.push_back(createBox(glm::vec3(18.5f - i * 9.0f, 2.0f, 50.0f), glm::vec3(9.0f, 5.0f, 1.0f)));

        // Gardul 2 (Axa X) 
        obstacles.push_back(createBox(glm::vec3(18.5f - i * 9.0f, 2.0f, 95.3f), glm::vec3(9.0f, 5.0f, 1.0f)));

        // Gardul 3 (Axa Z) 
        obstacles.push_back(createBox(glm::vec3(22.85f, 2.0f, 54.6f + i * 9.0f), glm::vec3(1.0f, 5.0f, 9.0f)));
    }
}
void initCube() {
    // Un cub wireframe (linii) de la (0,0,0) la (1,1,1)
    // 3 float (poz) + 3 float (normala-dummy) + 2 float (tex-dummy) = 8 floats per vertex
    float vertices[] = {
        // Baza (y=0)
        0.0f, 0.0f, 0.0f,  0,0,0, 0,0,  1.0f, 0.0f, 0.0f,  0,0,0, 0,0,
        1.0f, 0.0f, 0.0f,  0,0,0, 0,0,  1.0f, 0.0f, 1.0f,  0,0,0, 0,0,
        1.0f, 0.0f, 1.0f,  0,0,0, 0,0,  0.0f, 0.0f, 1.0f,  0,0,0, 0,0,
        0.0f, 0.0f, 1.0f,  0,0,0, 0,0,  0.0f, 0.0f, 0.0f,  0,0,0, 0,0,

        // Top (y=1)
        0.0f, 1.0f, 0.0f,  0,0,0, 0,0,  1.0f, 1.0f, 0.0f,  0,0,0, 0,0,
        1.0f, 1.0f, 0.0f,  0,0,0, 0,0,  1.0f, 1.0f, 1.0f,  0,0,0, 0,0,
        1.0f, 1.0f, 1.0f,  0,0,0, 0,0,  0.0f, 1.0f, 1.0f,  0,0,0, 0,0,
        0.0f, 1.0f, 1.0f,  0,0,0, 0,0,  0.0f, 1.0f, 0.0f,  0,0,0, 0,0,

        // Verticale
        0.0f, 0.0f, 0.0f,  0,0,0, 0,0,  0.0f, 1.0f, 0.0f,  0,0,0, 0,0,
        1.0f, 0.0f, 0.0f,  0,0,0, 0,0,  1.0f, 1.0f, 0.0f,  0,0,0, 0,0,
        1.0f, 0.0f, 1.0f,  0,0,0, 0,0,  1.0f, 1.0f, 1.0f,  0,0,0, 0,0,
        0.0f, 0.0f, 1.0f,  0,0,0, 0,0,  0.0f, 1.0f, 1.0f,  0,0,0, 0,0
    };

    glGenVertexArrays(1, &debugVAO);
    glGenBuffers(1, &debugVBO);

    glBindVertexArray(debugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Pozitie (locatia 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normale (locatia 1) 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoords (locatia 2) 
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}
void drawDebugBox(BoundingBox box, gps::Shader shader) {
    shader.useShaderProgram();

    // calcul dimensiunea si pozitia
    glm::vec3 size = box.max - box.min;
    glm::vec3 position = box.min;

    // construire matricea Model
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position); // mutam la pozitia min
    model = glm::scale(model, size);         // scalare la dimensiunea corecta

    // trimitere matrice la shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // Calcul normala (necesar pentru shader)
    glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // desenare linii
    glBindVertexArray(debugVAO);
    glDrawArrays(GL_LINES, 0, 24); // 12 linii * 2 puncte
    glBindVertexArray(0);
}
void initSmokeParticles() {


    // 2. Definim un patrat simplu (Quad)
    float particleVertices[] = {
        // Pozitie (x,y,z)   // TexCoords (u,v)
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,

         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f
    };

    glGenVertexArrays(1, &smokeVAO);
    glGenBuffers(1, &smokeVBO);

    glBindVertexArray(smokeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, smokeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particleVertices), particleVertices, GL_STATIC_DRAW);

    // Atribut 0: Pozitie
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Atribut 2: Textura 

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}
void renderSmoke() {
    particleShader.useShaderProgram();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Trimitem matricile
    glUniformMatrix4fv(glGetUniformLocation(particleShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(particleShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(smokeVAO);

    for (const auto& p : smokeParticles) {
        glm::mat4 modelSmoke = glm::mat4(1.0f);
        modelSmoke = glm::translate(modelSmoke, p.position);

        // Billboarding
        modelSmoke[0][0] = view[0][0]; modelSmoke[0][1] = view[1][0]; modelSmoke[0][2] = view[2][0];
        modelSmoke[1][0] = view[0][1]; modelSmoke[1][1] = view[1][1]; modelSmoke[1][2] = view[2][1];
        modelSmoke[2][0] = view[0][2]; modelSmoke[2][1] = view[1][2]; modelSmoke[2][2] = view[2][2];

        modelSmoke = glm::scale(modelSmoke, glm::vec3(p.size));

        glUniformMatrix4fv(glGetUniformLocation(particleShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelSmoke));
        glUniform1f(glGetUniformLocation(particleShader.shaderProgram, "opacity"), p.life);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    myBasicShader.useShaderProgram();
}
void updateSmokeParticles(float deltaTime) {
    smokeSpawnTimer += deltaTime;

    //1. LOGICA DE DENSITATE (IDLE vs MOVING)

    float spawnInterval = 0.2f;   // IDLE: Fum rar (o data la 0.2 secunde)
    float ejectionForce = 2.0f;   // IDLE: Iese incet

    // Daca apasam acceleratia W sau spatele S
    if (pressedKeys[GLFW_KEY_W] || pressedKeys[GLFW_KEY_S]) {
        spawnInterval = 0.04f;    // MOVING: Fum des (mult mai rapid)
        ejectionForce = 6.0f;     // MOVING: Iese cu presiune mai mare
    }

    // generare particule doar daca a trecut timpul necesar
    if (smokeSpawnTimer >= spawnInterval) {
        smokeSpawnTimer = 0.0f;

        // Calcula pozitie (in spate)
        glm::vec3 exhaustOffset = glm::vec3(-0.7f, 0.5f, -3.8f);
        glm::mat4 rotationMat = glm::rotate(glm::mat4(1.0f), glm::radians(carAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 rotatedOffset = glm::vec3(rotationMat * glm::vec4(exhaustOffset, 1.0f));
        glm::vec3 spawnPos = carPosition + rotatedOffset;

        Particle p;
        p.position = spawnPos;

        // CALCUL VITEZA DIAGONALA 
        float rad = glm::radians(carAngle);
        glm::vec3 backwardDir = glm::vec3(-sin(rad), 0.0f, -cos(rad));

        // Folosim 'ejectionForce' care e mai mare cand acceleram
        glm::vec3 diagonalVelocity = (backwardDir * ejectionForce) + glm::vec3(0.0f, 1.5f, 0.0f);

        //generare random
        float randX = ((rand() % 100) / 100.0f - 0.5f);
        float randY = ((rand() % 100) / 100.0f - 0.5f);
        float randZ = ((rand() % 100) / 100.0f - 0.5f);

        p.velocity = diagonalVelocity + glm::vec3(randX * 0.5f, randY * 0.5f, randZ * 0.5f);
        p.life = 1.0f;
        p.size = 0.1f;

        smokeParticles.push_back(p);
    }

    //2. UPDATE PARTICULE EXISTENTE 
    for (int i = 0; i < smokeParticles.size(); i++) {
        smokeParticles[i].position += smokeParticles[i].velocity * deltaTime;

        // Frecare cu aerul (incetinire viteza pe orizontala)
        smokeParticles[i].velocity.x *= 0.96f;
        smokeParticles[i].velocity.z *= 0.96f;

        smokeParticles[i].life -= deltaTime * 0.8f;
        smokeParticles[i].size += deltaTime * 2.5f;

        if (smokeParticles[i].life <= 0.0f) {
            smokeParticles.erase(smokeParticles.begin() + i);
            i--;
        }
    }
}
void drawObjects(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    GLint specLoc = glGetUniformLocation(shader.shaderProgram, "specularStrength");
    glUniform1f(specLoc, 0.0f);
    RenderModel(building, shader, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
    RenderModel(building, shader, glm::vec3(-60.0f, 0.0f, 0.0f), glm::vec3(.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
    RenderModel(building, shader, glm::vec3(0.0f, 0.0f, -20.0f), glm::vec3(.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
    RenderModel(building, shader, glm::vec3(-60.0f, 0.0f, -20.0f), glm::vec3(.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
    RenderModel(building, shader, glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
    RenderModelRotated(building, shader, glm::vec3(-130.0f, 0.0f, 80.0f), glm::vec3(.02f), -90.0f, 0.0f, 180);
    RenderModelRotated(building, shader, glm::vec3(-130.0f, 0.0f, 100.0f), glm::vec3(.02f), -90.0f, 0.0f, 180);
    RenderModelRotated(building, shader, glm::vec3(-130.0f, 0.0f, 120.0f), glm::vec3(.02f), -90.0f, 0.0f, 180);

    RenderModelRotated(building, shader, glm::vec3(-60.0f, 0.0f, 210.0f), glm::vec3(.02f), -90.0f, 0.0f, -90);
    RenderModelRotated(building, shader, glm::vec3(-40.0f, 0.0f, 210.0f), glm::vec3(.02f), -90.0f, 0.0f, -90);
    RenderModelRotated(building, shader, glm::vec3(-20.0f, 0.0f, 210.0f), glm::vec3(.02f), -90.0f, 0.0f, -90);

    RenderModelRotated(building, shader, glm::vec3(60.0f, 0.0f, 210.0f), glm::vec3(.02f), -90.0f, 0.0f, -90);
    RenderModelRotated(building, shader, glm::vec3(80.0f, 0.0f, 210.0f), glm::vec3(.02f), -90.0f, 0.0f, -90);
    RenderModelRotated(building, shader, glm::vec3(100.0f, 0.0f, 210.0f), glm::vec3(.02f), -90.0f, 0.0f, -90);

    RenderModel(lamp, shader, glm::vec3(18.0f, 0.0f, 70.0f), glm::vec3(3.0f));
    RenderModel(bench, shader, glm::vec3(20.0f, 0.0f, 60.0f), glm::vec3(0.08f), -90, glm::vec3(0.0f, 1.0f, 0.0f));
    RenderModel(bench, shader, glm::vec3(20.0f, 0.0f, 80.0f), glm::vec3(0.08f),-90,glm::vec3(0.0f,1.0f,0.0f));

    RenderModel(house, shader, glm::vec3(40.0f, 0.0f, -70.0f), glm::vec3(0.02f));
    RenderModel(house, shader, glm::vec3(84.0f, 0.0f, 132.0f), glm::vec3(0.02f));
    
    glDisable(GL_CULL_FACE); // sa nu randeze doar triunghiurile din fata
    RenderModel(garage, shader, glm::vec3(103.5f, 0.2f, 9.2f), glm::vec3(2.4f));
    glEnable(GL_CULL_FACE);
    renderGarageDoor(shader);
    for (int i = 0; i < treePositions.size(); i++) {
        RenderModel(tree, shader, treePositions[i], glm::vec3(3.0f));
    }


    for (int i = 0; i <= 7; i++) {
        for (int j = 0; j <= 7; j++) {
            RenderModel(grass, shader, glm::vec3(20.0f-j*5.55f, 0.0f, 52.9f + i * 5.65f), glm::vec3(.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
        }
    }
    for (int i = 0; i <= 4; i++) {
        RenderModel(fence, shader, glm::vec3(18.5f-i*9.0f, 2.0f, 50.0f), glm::vec3(0.02f), 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        RenderModel(fence, shader, glm::vec3(18.5f - i * 9.0f, 2.0f, 95.3f), glm::vec3(0.02f), 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        RenderModel(fence, shader, glm::vec3(22.85f, 2.0f, 54.6f+i*9.0f), glm::vec3(0.02f), 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    RenderModel(straight_street, shader, glm::vec3(38.5f, -3.5f, -32.2f), glm::vec3(3.2f));
    RenderModel(straight_street, shader, glm::vec3(124.2f, -3.5f, 47.5f), glm::vec3(3.2f),90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    RenderModel(straight_street, shader, glm::vec3(124.2f, -3.5f, 99.5f), glm::vec3(3.2f), 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    RenderModel(straight_street, shader, glm::vec3(38.5f, -3.5f, 178.0f), glm::vec3(3.2f)); //*
    RenderModel(straight_street, shader, glm::vec3(-31.5f, -3.5f, 178.0f), glm::vec3(3.2f)); //*
    RenderModel(straight_street, shader, glm::vec3(-97.9f, -3.5f, 105.5f), glm::vec3(3.2f), 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
   

    RenderModel(road, shader, glm::vec3(-27.0f, 0.5f, -22.0f), glm::vec3(.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
    RenderModelRotated(road, shader, glm::vec3(-50.5f, 0.5f, 15.5f), glm::vec3(.02f), -90.0f,0.0f,180);
    RenderModelRotated(road, shader, glm::vec3(110.5f, 0.5f, -22.5f), glm::vec3(.02f), -90.0f, 0.0f, -90);
    RenderModelRotated(road, shader, glm::vec3(110.5f, 0.5f, 164.2f), glm::vec3(.02f), -90.0f, 0.0f, 180); //*
    RenderModel(road, shader, glm::vec3(-88.0f, 0.5f, 39.5f), glm::vec3(.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
    RenderModelRotated(road, shader, glm::vec3(-87.8f, 0.5f, 164.5f), glm::vec3(.02f), -90.0f, 0.0f, -270.0f);
    renderFloor(shader);


    glUniform1f(specLoc, 0.5f);
    renderTeapot(shader);
    renderCar(shader);
   // RenderModel(table, shader, glm::vec3(0.0f, 0.0f, 3.0f));
    // 2. Masa (Cu logica de reflexie)
    if (depthPass) {
        // Cazul 1: randare umbre fara shader reflexie aici
        //RenderModel(table, shader, glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    else {
        // Cazul 2: Randam scena finala
        // cu shaderul de reflexie
        //RenderModelWithReflection(table, reflectionShader, glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.02f), -90, glm::vec3(1.0f, 0.0f, 0.0f));
    }
}
void renderScene() {
    int width, height;
    // Luam dimensiunile reale in pixeli (buffer) direct de la fereastra
    glfwGetFramebufferSize(myWindow.getWindow(), &width, &height);

    // evitare impartirea la 0
    if (height == 0) height = 1;

    // Setam viewport-ul pe TOATA suprafata disponibila
    glViewport(0, 0, width, height);

    // calcul matricea de proiectie cu noul aspect ratio (width / height)
    //  far plane mare pt a marii vizibilitatea
    projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
    // **** PASUL 1: Generare Harta de Umbre ****
    depthMapShader.useShaderProgram();

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // dezactivare Cull Face ca sa se prinda umbra din toate unghiurile
    glDisable(GL_CULL_FACE);

    // 1. Configuram matricea luminii
    float near_plane = 1.0f, far_plane = 150.0f; // marim far_plane
    // marim zona ortografica (-20 la 20) ca sa incapa toata scena
    glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, near_plane, far_plane);

    glm::vec3 sunOffset = glm::vec3(30.0f, 30.0f, 30.0f);

    glm::vec3 shadowCenter;

    if (inCarMode) {
        // daca masina se misca umbrele urmaresc masina
        shadowCenter = carPosition;
    }
    else {
        // aaca zburam liber, umbrele urmaresc camera
        shadowCenter = myCamera.getCameraPosition();
    }
    // pozitia actuala a camerei de umbre este Masina + Offset
    glm::vec3 currentLightPos = shadowCenter + sunOffset;

    // matricea de view se uita la MASINA (carPosition), nu la (0,0,0)
    glm::mat4 lightView = glm::lookAt(currentLightPos, shadowCenter, glm::vec3(0.0f, 1.0f, 0.0f));


    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
    // 2. Trimitere matricea la shaderul de adancime
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    // 3. Desenam obiectele in harta de umbre
    drawObjects(depthMapShader, true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);



    // **** PASUL 2: Randare Scena Finala ****
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reactivare Culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    myBasicShader.useShaderProgram();
    // CALCUL POZITIE SI DIRECTIE PENTRU FARURI (SPOT LIGHTS)
    glm::vec3 headlightOffsetLeft = glm::vec3(0.7f, 2.1f, -0.5f);
    glm::vec3 headlightOffsetRight = glm::vec3(-0.7f, 2.1f, -0.5f);

    glm::mat4 carRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(carAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 rotatedLeft = glm::vec3(carRotationMatrix * glm::vec4(headlightOffsetLeft, 1.0f));
    glm::vec3 rotatedRight = glm::vec3(carRotationMatrix * glm::vec4(headlightOffsetRight, 1.0f));

    glm::vec3 finalPosLeft = carPosition + rotatedLeft;
    glm::vec3 finalPosRight = carPosition + rotatedRight;

    // 2. calculam DIRECTIA farurilor (Vectorul Forward al masinii)
    // masina se misca pe baza lui carAngle.
    glm::vec3 carDirection;
    carDirection.x = sin(glm::radians(carAngle));
    carDirection.y = -0.1f; // le inclinam foarte putin in jos ca sa bata pe asfalt
    carDirection.z = cos(glm::radians(carAngle));
    carDirection = glm::normalize(carDirection);

    // TRIMITEM DATELE LA SHADER (STRUCTURA SPOTLIGHT)

    // --- FARUL STANGA (spotLight[0]) ---
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[0].position"), 1, glm::value_ptr(finalPosLeft));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[0].direction"), 1, glm::value_ptr(carDirection));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[0].color"), 1, glm::value_ptr(glm::vec3(1.0f, 0.6f, 0.0f))); // Alb galbui

    // Parametrii conului si atenuare
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[0].constant"), 1.0f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[0].linear"), 0.09f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[0].quadratic"), 0.032f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[0].cutOff"), glm::cos(glm::radians(12.5f))); // Unghi interior
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[0].outerCutOff"), glm::cos(glm::radians(17.5f))); // Unghi exterior

    // --- FARUL DREAPTA (spotLight[1]) ---
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[1].position"), 1, glm::value_ptr(finalPosRight));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[1].direction"), 1, glm::value_ptr(carDirection));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[1].color"), 1, glm::value_ptr(glm::vec3(1.0f, 0.6f, 0.0f)));

    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[1].constant"), 1.0f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[1].linear"), 0.09f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[1].quadratic"), 0.032f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[1].cutOff"), glm::cos(glm::radians(12.5f)));
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "spotLight[1].outerCutOff"), glm::cos(glm::radians(17.5f)));


    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    // trimitem matricea de lumina
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    // mutare Shadow Map pe Texture Unit 3
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

    // activare textura obiectului pe Texture Unit 0
    glActiveTexture(GL_TEXTURE0);

    drawObjects(myBasicShader, false);

    // DESENAREA SKYBOX
    skyboxShader.useShaderProgram();

    // oprim culling-ul ca sa vedem interiorul cubului
    // cu culling pornit nu se va vedea nimic
    glDisable(GL_CULL_FACE);

    mySkyBox.Draw(skyboxShader, view, projection);

    // repornire culling-ul la loc pentru restul scenei (urmatorul frame)
    glEnable(GL_CULL_FACE);
    renderSmoke();
    if (showBoundingBoxes) {
        myBasicShader.useShaderProgram();

        // trimitere matricile View si Projection
        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // activare modul LINE (doar contur)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        // 1. OBSTACOLELE STATICE (Pereti, copaci)
        for (const auto& box : obstacles) {
            drawDebugBox(box, myBasicShader);
        }

        // 2. USA DE GARAJ (ANIMATA)  
        float normalizedTime = garageDoorAngle / 90.0f;
        float slideUp = normalizedTime * 4.2f;
        float slideOut = normalizedTime * 2.5f;

        glm::vec3 basePos = glm::vec3(98.75f, 0.9f, 4.8f);
        glm::vec3 animPos = glm::vec3(basePos.x, basePos.y + slideUp, basePos.z - slideOut);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, animPos);
        model = glm::rotate(model, glm::radians(garageDoorAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        // centrare si scalare usa
        model = glm::translate(model, glm::vec3(-2.75f, 0.0f, -0.25f));
        model = glm::scale(model, glm::vec3(5.5f, 3.0f, 0.5f));

        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(debugVAO);
        glDrawArrays(GL_LINES, 0, 24);

        // 3. MASINA (BOUNDING BOX ROTIT) 
        glm::vec3 carSize = glm::vec3(2.5f, 2.0f, 4.0f); // aceleasi dimensiuni ca in processMovement

        glm::mat4 modelCar = glm::mat4(1.0f);

        // A. Pozitionare si Rotatie (La fel ca modelul 3D al masinii)
        modelCar = glm::translate(modelCar, carPosition);
        modelCar = glm::rotate(modelCar, glm::radians(carAngle), glm::vec3(0.0f, 1.0f, 0.0f));

        // B. Scalare la dimensiunea coliziunii
        modelCar = glm::scale(modelCar, carSize);

        // C. Centrare (Cubul debug e 0..1, noi il vrem -0.5..0.5 ca sa fie centrat pe masina)
        modelCar = glm::translate(modelCar, glm::vec3(-0.5f, 0.0f, -0.5f));

        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelCar));

        // Desenare
        // (Nu e nevoie sa dam bind din nou daca e acelasi VAO, dar e safe sa fie aici)
        glBindVertexArray(debugVAO);
        glDrawArrays(GL_LINES, 0, 24);
        glBindVertexArray(0);

        //Revenire la modul FILL
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
	initShaders();
	initUniforms();
    setWindowCallbacks();
    initShadowMap();
	initSkyBox();
	initRandomTrees();
    initObstacles();
    initSmokeParticles();
    initCube();
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        double currentTimeStamp = glfwGetTime();
        updateDelta(currentTimeStamp - lastTimeStamp);
        lastTimeStamp = currentTimeStamp;
        processMovement();
        updateSmokeParticles(deltaTime);
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
