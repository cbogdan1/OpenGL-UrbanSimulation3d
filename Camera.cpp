#include "Camera.hpp"
#include <iostream> 
namespace gps {

    //Camera constructor
    ///              c(pnct pozitie)           l(priveste spre l)      u'(directie sus)
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
        //vectorul U din formula U=RxV

        //TODO - Update the rest of camera parameters
        glm::vec3 cameraFront = glm::normalize(cameraTarget - cameraPosition); //directia inainte/spate pe camera target pe l
        //vectorul  V din formula V=c-l/||c-l||
        this->cameraFrontDirection = cameraFront;


        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUpDirection)); //directia dreapta
        //vectorul R din formula R=-(VxU')/||VXU'||
        this->cameraRightDirection = cameraRight;

    }
    glm::vec3 Camera::getCameraPosition() {
        return cameraPosition;
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        //TODO
        switch (direction) {
        case MOVE_FORWARD:
            cameraPosition += cameraFrontDirection * speed;
            cameraTarget += cameraFrontDirection * speed;
            break;
        case MOVE_BACKWARD:
            cameraPosition -= cameraFrontDirection * speed;
            cameraTarget -= cameraFrontDirection * speed;
            break;
        case MOVE_LEFT:
            cameraPosition -= cameraRightDirection * speed;
            cameraTarget -= cameraRightDirection * speed;
            break;
        case MOVE_RIGHT:
            cameraPosition += cameraRightDirection * speed;
            cameraTarget += cameraRightDirection * speed;
            break;
        }
        cameraPosition.y = std::max(cameraPosition.y, 0.8f);
        std::cout << "X: " << cameraPosition.x << " | Y: " << cameraPosition.y << " | Z: " << cameraPosition.z << std::endl;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        // Converte?te gradele în radiani
        float radPitch = glm::radians(pitch);
        float radYaw = glm::radians(yaw);
        glm::vec3 front;
        front.x = cos(radYaw) * cos(radPitch);
        front.y = sin(radPitch);
        front.z = sin(radYaw) * cos(radPitch);


        cameraFrontDirection = glm::normalize(front);

        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));

        cameraTarget = cameraPosition + cameraFrontDirection;
    }

}