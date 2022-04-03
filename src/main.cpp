#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

#include <cstdlib>
#include <cmath>
#include <random>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

unsigned int loadTexture(char const * path);

unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = true;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    bool blinn = false;
    PointLight pointLight;
    SpotLight spotLight;
    bool frogShot=false;
    bool dogShot=false;
    bool catsShot=false;
    bool parrotShot=false;
    int animalsShot=0;
    int cameraEnd=0;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 0.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    /*std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';*/
}

void ProgramState::LoadFromFile(std::string filename) {
    /*std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }*/
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);
void renderCube();
void renderQuad();
float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}
int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window,mouse_button_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    //programState->LoadFromFile("resources/program_state.txt");
    /*if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }*/
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader lightShader("resources/shaders/light.vs", "resources/shaders/light.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader transpShader("resources/shaders/transparentcameraobj.vs", "resources/shaders/transparentcameraobj.fs");
    //SSAO shader
    Shader shaderGeometryPass("resources/shaders/ssao_geometry.vs", "resources/shaders/ssao_geometry.fs");
    Shader shaderLightingPass("resources/shaders/ssao.vs", "resources/shaders/ssao_lighting.fs");
    Shader shaderSSAO("resources/shaders/ssao.vs", "resources/shaders/ssao.fs");
    Shader shaderSSAOBlur("resources/shaders/ssao.vs", "resources/shaders/ssao_blur.fs");
    // load models
    // -----------
    Model parrotModel("resources/objects/parrot/parrot_HP.obj");
    parrotModel.SetShaderTextureNamePrefix("material.");
    Model frogModel("resources/objects/frog/Ceramic-frog_low-poly.obj");
    frogModel.SetShaderTextureNamePrefix("material.");
    Model dogModel("resources/objects/dog/DogPR.obj");
    dogModel.SetShaderTextureNamePrefix("material.");
    Model catsModel("resources/objects/cats/Cats.obj");
    catsModel.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 10.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.6f;
    pointLight.linear = 0.04f;
    pointLight.quadratic = 0.05f;

    SpotLight& spotLight = programState->spotLight;
    spotLight.position = programState->camera.Position;
    spotLight.direction = programState->camera.Front;
    spotLight.ambient = glm::vec3(0.0f, 0.0f, 0.0f);
    spotLight.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);

    spotLight.constant = 0.5f;
    spotLight.linear = 0.0f;
    spotLight.quadratic = 0.0f;
    spotLight.cutOff = glm::cos(glm::radians(5.0f));
    spotLight.outerCutOff = glm::cos(glm::radians(7.0f));

    float transparentVertices[] = {
            0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  0.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  0.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  1.0f
    };

    //skybox
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    unsigned int transparentCameraVAO, transparentCameraVBO;
    glGenVertexArrays(1, &transparentCameraVAO);
    glGenBuffers(1, &transparentCameraVBO);
    glBindVertexArray(transparentCameraVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentCameraVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    stbi_set_flip_vertically_on_load(true);
    unsigned int transparentCameraTexture = loadTexture(FileSystem::getPath("resources/textures/camera.png").c_str());
    unsigned int transparentCheckmarkTexture = loadTexture(FileSystem::getPath("resources/textures/checkmark.png").c_str());
    transpShader.use();
    transpShader.setInt("texture1",0);

    vector<std::string> skyboxSides = {
            FileSystem::getPath("resources/textures/cubemap/posx.jpg"),
            FileSystem::getPath("resources/textures/cubemap/negx.jpg"),
            FileSystem::getPath("resources/textures/cubemap/posy.jpg"),
            FileSystem::getPath("resources/textures/cubemap/negy.jpg"),
            FileSystem::getPath("resources/textures/cubemap/posz.jpg"),
            FileSystem::getPath("resources/textures/cubemap/negz.jpg"),
            FileSystem::getPath("resources/textures/cubemap/negz.jpg"),
    };

    stbi_set_flip_vertically_on_load(false);
    unsigned int cubemapTexture = loadCubemap(skyboxSides);
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedo;
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // also create framebuffer to hold SSAO processing stage
    // -----------------------------------------------------
    unsigned int ssaoFBO, ssaoBlurFBO;
    glGenFramebuffers(1, &ssaoFBO);  glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    unsigned int ssaoColorBuffer, ssaoColorBufferBlur;
    // SSAO color buffer
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    // and blur stage
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0;

        // scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }
    // generate noise texture
    // ----------------------
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
    unsigned int noiseTexture; glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // lighting info
    // -------------
    glm::vec3 lightPos = glm::vec3(2.0, 4.0, -2.0);
    glm::vec3 lightColor = glm::vec3(0.86, 0.3f, 0.2f);

    // shader configuration
    // --------------------
    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedo", 2);
    shaderLightingPass.setInt("ssao", 3);
    shaderSSAO.use();
    shaderSSAO.setInt("gPosition", 0);
    shaderSSAO.setInt("gNormal", 1);
    shaderSSAO.setInt("texNoise", 2);
    shaderSSAOBlur.use();
    shaderSSAOBlur.setInt("ssaoInput", 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model;
        if(programState->animalsShot==4){
            if(programState->cameraEnd==0){
                programState->camera.Zoom=38;
                programState->camera.Position=glm::vec3(0.0f,0.0f,-2.0f);
                programState->camera.Yaw=90.0f;
                programState->camera.Pitch=6.00f;
                programState->camera.Front = glm::vec3(-0.015, 0.106, 0.994);
                programState->camera.ProcessMouseMovement(0.0f,0.0f);
                programState->cameraEnd=1;
            }
            // 1. geometry pass: render scene's geometry/color data into gbuffer
            // -----------------------------------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            projection = glm::perspective(glm::radians(programState->camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT,
                                                    0.1f, 100.0f);
            view = programState->camera.GetViewMatrix();
            model = glm::mat4(1.0f);
            shaderGeometryPass.use();
            shaderGeometryPass.setMat4("projection", projection);
            shaderGeometryPass.setMat4("view", view);

            // room cube
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0, 0.0f, -1.5f));
            model = glm::scale(model, glm::vec3(1.0f));
            shaderGeometryPass.setMat4("model", model);
            shaderGeometryPass.setInt("invertedNormals", 1); // invert normals as we're inside the cube
            renderCube();
            shaderGeometryPass.setInt("invertedNormals", 0);

            // parrot
            if(programState->parrotShot){
                model = glm::mat4(1.0f);
                model = glm::translate(model,glm::vec3(-0.15,0.012,-1.5));
                model = glm ::rotate (model, currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.03f));
                shaderGeometryPass.setMat4("model", model);
                parrotModel.Draw(shaderGeometryPass);
            }
            //frog
            if(programState->frogShot){
                model = glm::mat4(1.0f);
                model = glm::translate(model,glm::vec3(-0.05f,0.012f,-1.5));
                model = glm ::rotate (model, currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.25f));
                shaderGeometryPass.setMat4("model", model);
                frogModel.Draw(shaderGeometryPass);
            }
            //dog
            if(programState->dogShot){
                model = glm::mat4(1.0f);
                model = glm::translate(model,glm::vec3(0.05f,0.012f,-1.5f));
                model = glm ::rotate (model, currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.04f));
                shaderGeometryPass.setMat4("model", model);
                dogModel.Draw(shaderGeometryPass);
            }
            //cats
            if(programState->catsShot){
                model = glm::mat4(1.0f);
                model = glm::translate(model,glm::vec3(0.15f,0.012f,-1.5f));
                model = glm ::rotate (model, currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.0002f));
                shaderGeometryPass.setMat4("model", model);
                catsModel.Draw(shaderGeometryPass);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 2. generate SSAO texture
            // ------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderSSAO.use();
            // Send kernel + rotation
            for (unsigned int i = 0; i < 64; ++i)
                shaderSSAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
            shaderSSAO.setMat4("projection", projection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, noiseTexture);
            renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);


            // 3. blur SSAO texture to remove noise
            // ------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderSSAOBlur.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
            renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);


            // 4. lighting pass: traditional deferred Blinn-Phong lighting with added screen-space ambient occlusion
            // -----------------------------------------------------------------------------------------------------
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            shaderLightingPass.use();
            // send light relevant uniforms
            glm::vec3 lightPosView = glm::vec3(programState->camera.GetViewMatrix() * glm::vec4(lightPos, 1.0));
            shaderLightingPass.setVec3("light.Position", lightPosView);
            shaderLightingPass.setVec3("light.Color", lightColor);
            // Update attenuation parameters
            const float linear = 0.09;
            const float quadratic = 0.032;
            shaderLightingPass.setFloat("light.Linear", linear);
            shaderLightingPass.setFloat("light.Quadratic", quadratic);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gAlbedo);
            glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
            glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
            renderQuad();
        }
        else{
            lightShader.use();
            //pointlight setup
            pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
            lightShader.setBool("blinn", programState->blinn);
            lightShader.setVec3("pointLight.position", pointLight.position);
            lightShader.setVec3("pointLight.ambient", pointLight.ambient);
            lightShader.setVec3("pointLight.diffuse", pointLight.diffuse);
            lightShader.setVec3("pointLight.specular", pointLight.specular);
            lightShader.setFloat("pointLight.constant", pointLight.constant);
            lightShader.setFloat("pointLight.linear", pointLight.linear);
            lightShader.setFloat("pointLight.quadratic", pointLight.quadratic);
            lightShader.setVec3("viewPosition", programState->camera.Position);
            lightShader.setFloat("material.shininess", 32.0f);

            //spotlight setup
            spotLight.position = programState->camera.Position;
            spotLight.direction = programState->camera.Front;
            lightShader.setVec3("spotLight.position", spotLight.position);
            lightShader.setVec3("spotLight.direction", spotLight.direction);
            lightShader.setVec3("spotLight.ambient", spotLight.ambient);
            lightShader.setVec3("spotLight.diffuse", spotLight.diffuse);
            lightShader.setVec3("spotLight.specular", spotLight.specular);
            lightShader.setFloat("spotLight.constant", spotLight.constant);
            lightShader.setFloat("spotLight.linear", spotLight.linear);
            lightShader.setFloat("spotLight.quadratic", spotLight.quadratic);
            lightShader.setFloat("spotLight.cutOff", spotLight.cutOff);
            lightShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

            // view/projection transformations
            glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                    (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
            glm::mat4 view = programState->camera.GetViewMatrix();
            lightShader.setMat4("projection", projection);
            lightShader.setMat4("view", view);

            glm::mat4 model = glm::mat4(1.0f);
            // render models
            lightShader.use();

            if(!programState->parrotShot){
                model = glm::mat4(1.0f);
                model = glm::translate(model,glm::vec3(0.40f,0.57f,0.69f));
                model = glm ::rotate (model, 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm ::rotate (model, -0.55f, glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm ::rotate (model, 0.1f, glm::vec3(0.0f, 0.0f, 1.0f));
                model = glm::scale(model, glm::vec3(0.03f));
                lightShader.setMat4("model", model);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                parrotModel.Draw(lightShader);
                glDisable(GL_CULL_FACE);
            }

            if(!programState->frogShot){
                model = glm::mat4(1.0f);
                model = glm::translate(model,glm::vec3(0.8f,-0.53f,-0.06f));
                model = glm ::rotate (model, -1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.25f));
                lightShader.setMat4("model", model);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                frogModel.Draw(lightShader);
                glDisable(GL_CULL_FACE);
            }

            if(!programState->dogShot){
                model = glm::mat4(1.0f);
                model = glm::translate(model,glm::vec3(-0.9f,-0.4f,-0.41f));
                model = glm ::rotate (model, 1.5f, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm ::rotate (model, -0.2f, glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.04f));
                lightShader.setMat4("model", model);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                dogModel.Draw(lightShader);
                glDisable(GL_CULL_FACE);
            }

            if(!programState->catsShot){
                model = glm::mat4(1.0f);
                model = glm::translate(model,glm::vec3(0.22f,0.37f,-0.9f));
                model = glm ::rotate (model, 2.5f, glm::vec3(0.0f, 1.0f, 0.0f));
                //model = glm ::rotate (model, -0.4f, glm::vec3(1.0f, 0.0f, 0.0f));
                //model = glm ::rotate (model, 0.2f, glm::vec3(0.0f, 0.0f, 1.0f));
                model = glm::scale(model, glm::vec3(0.0002f));
                lightShader.setMat4("model", model);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                catsModel.Draw(lightShader);
                glDisable(GL_CULL_FACE);
            }

        }

        //render camera
        if(programState->animalsShot!=4){
            transpShader.use();
            projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                          (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.5f,0.0f,-0.8));
            model = glm::scale(model, glm::vec3(1.0f));

            transpShader.setMat4("model", model);

            glBindVertexArray(transparentCameraVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, transparentCameraTexture);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDisable(GL_CULL_FACE);
        }

        //render checkmark
        if(programState->animalsShot!=4){
            transpShader.use();
            projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                          (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.3f,0.55f,-0.8));
            model = glm::scale(model, glm::vec3(0.2f));

            transpShader.setMat4("model", model);

            glBindVertexArray(transparentCameraVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, transparentCheckmarkTexture);
        }

        double xpos = programState->camera.Front.x;
        double ypos = programState->camera.Front.y;
        double zpos = programState->camera.Front.z;
        if(!programState->parrotShot){
            if((xpos-0.392)*(xpos-0.392) + (ypos-0.618)*(ypos-0.618) + (zpos-0.680)*(zpos-0.680) < 0.002
               && (programState->camera.Zoom<25 && programState->camera.Zoom>15)){
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glDisable(GL_CULL_FACE);
            }
        }
        if(!programState->frogShot){
            if((xpos-0.836)*(xpos-0.836) + (ypos+0.545)*(ypos+0.545) + (zpos+0.064)*(zpos+0.064) < 0.002
               && (programState->camera.Zoom<15 && programState->camera.Zoom>7)){
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glDisable(GL_CULL_FACE);
            }
        }
        if(!programState->dogShot){
            if((xpos+0.852)*(xpos+0.852) + (ypos+0.340)*(ypos+0.340) + (zpos+0.397)*(zpos+0.397) < 0.002
               && (programState->camera.Zoom<24 && programState->camera.Zoom>13)){
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glDisable(GL_CULL_FACE);
            }
        }
        if(!programState->catsShot){
            if((xpos-0.216)*(xpos-0.216) + (ypos-0.392)*(ypos-0.392) + (zpos+0.894)*(zpos+0.894) < 0.002
               && (programState->camera.Zoom<15 && programState->camera.Zoom>8)){
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glDisable(GL_CULL_FACE);
            }
        }

        // skybox shader setup
        // -----------
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        // render skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //de-allocate
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteVertexArrays(1,&transparentCameraVAO);
    glDeleteBuffers(1,&transparentCameraVBO);

    //programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RED;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if(programState->animalsShot!=4){
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        if (programState->CameraMouseMovementUpdateEnabled)
            programState->camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    /*{
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }*/

    /*{
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::End();
    }*/

    {
        if(!(programState->parrotShot||programState->frogShot
                ||programState->dogShot||programState->catsShot)){
            ImGui::Begin("GAME INFO:");
            ImGui::Text("Take picture of all animal totems.");
            ImGui::Text("Use scroll wheel to zoom in on totems and check mark will appear.");
            ImGui::Text("Left mouse button to take a picture.");
            ImGui::End();
        }
        if(programState->animalsShot==4){
            ImGui::Begin("END:");
            ImGui::Text("Congratulations!");
            ImGui::Text("You successfully took pictures of all animal totems!");
            ImGui::Text("Press ESC to exit.");
            ImGui::End();
        }

    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    /*if (key == GLFW_KEY_I && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            programState->CameraMouseMovementUpdateEnabled = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }*/
    if (key == GLFW_KEY_B && action == GLFW_PRESS && programState->animalsShot!=4){
        programState->blinn=!programState->blinn;
        if(programState->blinn){
            cout << "Blinn-Phong"<< endl;
        }
        else{
            cout << "Phong" << endl;
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && programState->animalsShot!=4){
        double xpos = programState->camera.Front.x;
        double ypos = programState->camera.Front.y;
        double zpos = programState->camera.Front.z;
        if(!programState->parrotShot){
            if((xpos-0.392)*(xpos-0.392) + (ypos-0.618)*(ypos-0.618) + (zpos-0.680)*(zpos-0.680) < 0.002
                && (programState->camera.Zoom<25 && programState->camera.Zoom>15)){
                programState->parrotShot=true;
                programState->animalsShot++;
            }
        }
        if(!programState->frogShot){
            if((xpos-0.836)*(xpos-0.836) + (ypos+0.545)*(ypos+0.545) + (zpos+0.064)*(zpos+0.064) < 0.002
               && (programState->camera.Zoom<15 && programState->camera.Zoom>7)){
                programState->frogShot=true;
                programState->animalsShot++;
            }
        }
        if(!programState->dogShot){
            if((xpos+0.852)*(xpos+0.852) + (ypos+0.340)*(ypos+0.340) + (zpos+0.397)*(zpos+0.397) < 0.002
               && (programState->camera.Zoom<24 && programState->camera.Zoom>13)){
                programState->dogShot=true;
                programState->animalsShot++;
            }
        }
        if(!programState->catsShot){
            if((xpos-0.216)*(xpos-0.216) + (ypos-0.392)*(ypos-0.392) + (zpos+0.894)*(zpos+0.894) < 0.002
               && (programState->camera.Zoom<15 && programState->camera.Zoom>8)){
                programState->catsShot=true;
                programState->animalsShot++;
            }
        }
    }

}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}