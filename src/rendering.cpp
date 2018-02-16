static void SetMatrixUniform(GLuint programID, const char* name, glm::mat4 matrix)
{
    glUniformMatrix4fv(glGetUniformLocation(programID, name), 1, GL_FALSE, &matrix[0][0]);
}

static void SetVec3Uniform(GLuint programID, const char* name, glm::vec3 vec)
{
    glUniform3f(glGetUniformLocation(programID, name), vec.x, vec.y, vec.z);
}

static void SetVec4Uniform(GLuint programID, const char* name, glm::vec4 vec)
{
    glUniform4f(glGetUniformLocation(programID, name), vec.x, vec.y, vec.z, vec.w);
}

static void SetFloatUniform(GLuint programID, const char* name, float value)
{
    glUniform1f(glGetUniformLocation(programID, name), value);
}

void MessageCallback(GLenum source,
                     GLenum type,
                     GLuint id,
                     GLenum severity,
                     GLsizei length,
                     const GLchar* message,
                     const void* userParam)
{
    (void)userParam; // Silence unused warning
    if(type == GL_DEBUG_TYPE_ERROR)
    {
        fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s, source = %x, id = %ud, length %ud= \n",
                (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
                type, severity, message, source, id, length);
    }
}



static glm::mat4 ComputeTransformation(glm::vec3 scale = glm::vec3(1.0f), glm::quat orientation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f), glm::vec3 position = glm::vec3(0.0f))
{
    glm::mat4 t;
    t = glm::scale(glm::mat4(1.0f), scale);
    t = glm::toMat4(orientation) * t;
    t = glm::translate(t, position);
    return t;
}

static void ComputeModelTransformation(model& object)
{
    object.transform = ComputeTransformation(object.scale, object.orientation, object.position);
}



static char* LoadShaderFromFile(const char* Path)
{
    GLchar* Source = {};
    
    FILE* File = fopen(Path, "r");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        uint32_t Size = (uint32_t)ftell(File);
        fseek(File, 0, SEEK_SET);
        
        Source = (GLchar*)malloc(Size + 1 * sizeof(GLchar));
        
        fread(Source, Size, 1, File);
        Source[Size] = '\0';
        
        fclose(File);
    }
    else
    {
        fprintf(stderr, "Could not read file %s. File does not exist.\n",Path);
    }
    
    return Source;
}

static shader LoadShaders(const char* vertexFilePath, const char* fragmentFilePath)
{
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    
    auto VertexShaderCode = LoadShaderFromFile(vertexFilePath);
    auto FragmentShaderCode = LoadShaderFromFile(fragmentFilePath);
    if(!VertexShaderCode)
    {
        fprintf(stderr, "Could not load vertex shader: %s\n", vertexFilePath);
    }
    if(!FragmentShaderCode)
    {
        fprintf(stderr, "Could not load fragment shader: %s\n", fragmentFilePath);
    }
    
    GLint result = GL_FALSE;
    int infoLogLength;
    
    glShaderSource(vertexShaderID, 1, &VertexShaderCode, NULL);
    glCompileShader(vertexShaderID);
    
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0)
    {
        char* Buffer = (char*)malloc((infoLogLength + 1) * sizeof(char));
        glGetShaderInfoLog(vertexShaderID, infoLogLength, NULL, &Buffer[0]);
        printf("%s\n", &Buffer[0]);
    }
    
    
    glShaderSource(fragmentShaderID, 1, &FragmentShaderCode, NULL);
    glCompileShader(fragmentShaderID);
    
    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0)
    {
        char* Buffer = (char*)malloc((infoLogLength + 1) * sizeof(char));
        glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL, &Buffer[0]);
        printf("%s\n", &Buffer[0]);
    }
    
    printf("Linking program\n");
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);
    
    glGetProgramiv(programID, GL_LINK_STATUS, &result);
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0)
    {
        char* Buffer = (char*)malloc((infoLogLength + 1) * sizeof(char));
        glGetShaderInfoLog(programID, infoLogLength, NULL, &Buffer[0]);
        printf("Bob: %s\n", &Buffer[0]);
    }
    
    glDetachShader(programID, vertexShaderID);
    glDetachShader(programID, fragmentShaderID);
    
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
    free(VertexShaderCode);
    free(FragmentShaderCode);
    
    shader newShader = {};
    newShader.programID = programID;
    return newShader;
}

static void InitializeOpenGL(render_context& renderContext)
{
    if(!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(0);
    }
    
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 
    
    // Open a window and create its OpenGL context
    // (In the accompanying source code, this variable is global for simplicity)
    renderContext.screenWidth= 1600;
    renderContext.screenHeight = 900;
    renderContext.window = glfwCreateWindow(renderContext.screenWidth, renderContext.screenHeight, "Convex Hull", NULL, NULL);
    if(renderContext.window == NULL){
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit(0);
    }
    
    glfwMakeContextCurrent(renderContext.window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    
    glfwSetCursorPosCallback(renderContext.window, MousePositionCallback);
    glfwSetScrollCallback(renderContext.window, MouseScrollCallback);
    
    printf("%s\n", glGetString(GL_VERSION));
    printf("Shading language supported: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Glad Version: %d.%d\n", GLVersion.major, GLVersion.minor);
    
    glEnable(GL_DEPTH_TEST);
    
    //glEnable(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable debug output
    glEnable              (GL_DEBUG_OUTPUT);
    glDebugMessageCallback((GLDEBUGPROC) MessageCallback, 0);
    
    glfwSetInputMode(renderContext.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    renderContext.textureShader = LoadShaders("../shaders/texture.vert", "../shaders/texture.frag");
    renderContext.colorShader = LoadShaders("../shaders/color.vert", "../shaders/color.frag");
    renderContext.basicShader = LoadShaders("../shaders/basic.vert", "../shaders/basic.frag");
    
    // Initialize line buffer
    glGenVertexArrays(1, &renderContext.primitiveVAO);
    glBindVertexArray(renderContext.primitiveVAO);
    glGenBuffers(1, &renderContext.primitiveVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderContext.primitiveVBO);
    
    glBindVertexArray(0);
    
    // Initialize Quad buffers
    glGenVertexArrays(1, &renderContext.quadVAO);
    glBindVertexArray(renderContext.quadVAO);
    
    glGenBuffers(1, &renderContext.quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderContext.quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(renderContext.quadVertices),renderContext.quadVertices, GL_STATIC_DRAW);
    
    glGenBuffers(1, &renderContext.quadIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderContext.quadIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(renderContext.quadIndices), renderContext.quadIndices, GL_STATIC_DRAW);
    
    glUseProgram(renderContext.basicShader.programID);
    
    auto pos = glGetAttribLocation(renderContext.basicShader.programID, "position");
    
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glBindVertexArray(0);
}

static void RenderLine(render_context& renderContext, glm::vec3 start = glm::vec3(0.0f), glm::vec3 end = glm::vec3(0.0f), glm::vec4 color = glm::vec4(1.0f), float lineWidth = 2.0f)
{
    glUseProgram(renderContext.basicShader.programID);
    
    glBindVertexArray(renderContext.primitiveVAO);
    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, renderContext.primitiveVBO);
    auto width = 0.02f * lineWidth;
    
    auto normal =  glm::vec3(width/2.0f, 0.0f, width/2.0f);
    
    auto v1 = start - normal;
    auto v2 = start + normal;
    auto v3 = end - normal;
    auto v4 = end + normal;
    
    GLfloat points[18] = {v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, v3.x, v3.y, v3.z, v4.x, v4.y, v4.z, v2.x, v2.y, v2.z};
    glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), &points[0], GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    
    SetMatrixUniform(renderContext.basicShader.programID, "M", glm::mat4(1.0f));
    SetMatrixUniform(renderContext.basicShader.programID, "V", renderContext.viewMatrix);
    SetMatrixUniform(renderContext.basicShader.programID, "P", renderContext.projectionMatrix);
    SetVec4Uniform(renderContext.basicShader.programID, "c", color);
    
    glDrawArrays(GL_TRIANGLES, 0, 18);
}

static light& CreateLight(render_context& renderContext, glm::vec3 position = glm::vec3(0.0f), glm::vec3 color = glm::vec3(1.0f), float power = 2.0f)
{
    auto& l = renderContext.lights[renderContext.lightCount++];
    
    l.position = position;
    l.color = color;
    l.power = power;
    
    return l;
}

static float* BuildVertexBuffer(face* faces, int numFaces)
{
    float* vertices = (float*)malloc(sizeof(float) * numFaces * 3 * 3);
    for(int i = 0; i < numFaces; i++)
    {
        vertices[i * 9] = faces[i].vertices[0].x;
        vertices[i * 9 + 1] = faces[i].vertices[0].y;
        vertices[i * 9 + 2] = faces[i].vertices[0].z;
        vertices[i * 9 + 3] = faces[i].vertices[1].x;
        vertices[i * 9 + 4] = faces[i].vertices[1].y;
        vertices[i * 9 + 5] = faces[i].vertices[1].z;
        vertices[i * 9 + 6] = faces[i].vertices[2].x;
        vertices[i * 9 + 7] = faces[i].vertices[2].y;
        vertices[i * 9 + 8] = faces[i].vertices[2].z;
    }
    return vertices;
}

static glm::vec3 ComputeFaceNormal(face f)
{
    auto u = f.vertices[1] - f.vertices[0];
    auto v = f.vertices[2] - f.vertices[0];
    
    glm::vec3 normal = glm::vec3(0.0f);
    normal.x = (u.y * v.z) - (u.z * v.y);
    normal.y = (u.z * v.x) - (u.x * v.z);
    normal.z = (u.x * v.y) - (u.y * v.x);
    
    return normal;
}

static void AddFace(model& m, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3)
{
    if(m.numFaces + 1 >= m.facesSize)
    {
        if(m.facesSize == 0)
        {
            m.facesSize = 2;
            m.faces = (face*)malloc(sizeof(face) * m.facesSize);
        }
        else
        {
            m.faces = (face*)realloc(m.faces, m.facesSize * 2);
        }
    }
    
    face newFace = {};
    newFace.vertices[0] = v1;
    newFace.vertices[1] = v2;
    newFace.vertices[2] = v3;
    m.faces[m.numFaces++] = newFace;
}

static model& InitEmptyModel(render_context& renderContext)
{
    model& object = renderContext.models[renderContext.modelCount++];
    
    glGenVertexArrays(1, &object.VAO);
    glBindVertexArray(object.VAO);
    glGenBuffers(1, &object.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, object.VBO);
    
    auto& mat = object.material;
    mat.specularColor = glm::vec3(1.0f);
    mat.alpha = 1.0f;
    mat.type = MT_color;
    mat.diffuse.diffuseColor = glm::vec3(1.0f);
    mat.materialShader = renderContext.colorShader;
    
    object.vertexCount = 0;
    object.uvCount = 0;
    object.colorCount = 0;
    object.normalCount = 0;
    
    glBindVertexArray(0);
    
    return object;
}

static model& LoadModel(render_context& renderContext, gl_buffer vbo, gl_buffer* uvBuffer = 0, gl_buffer* colorBuffer = 0, gl_buffer* normalBuffer = 0, glm::vec3 diffuseColor = glm::vec3(1.0f))
{
    model& object = renderContext.models[renderContext.modelCount++];
    
    glGenVertexArrays(1, &object.VAO);
    glBindVertexArray(object.VAO);
    glGenBuffers(1, &object.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, object.VBO);
    glBufferData(GL_ARRAY_BUFFER, vbo.size, vbo.data, GL_STATIC_DRAW);
    
    auto& mat = object.material;
    mat.specularColor = glm::vec3(1, 1, 1);
    mat.alpha = 1.0f;
    
    object.vertexCount = vbo.count;
    object.uvCount = 0;
    object.colorCount = 0;
    object.normalCount = 0;
    
    if(uvBuffer)
    {
        glGenBuffers(1, &object.uvBufferHandle);
        glBindBuffer(GL_ARRAY_BUFFER, object.uvBufferHandle);
        glBufferData(GL_ARRAY_BUFFER, uvBuffer->size, uvBuffer->data, GL_STATIC_DRAW);
        object.hasUV = true;
        mat.type = MT_texture;
        mat.texture.tex = uvBuffer->uv.tex;
        mat.materialShader = renderContext.textureShader;
        object.uvCount = uvBuffer->count;
    }
    else
    {
        mat.type = MT_color;
        mat.diffuse.diffuseColor = diffuseColor;
        mat.materialShader = renderContext.colorShader;
    }
    
    if(colorBuffer)
    {
        glGenBuffers(1, &object.colorBufferHandle);
        glBindBuffer(GL_ARRAY_BUFFER, object.colorBufferHandle);
        glBufferData(GL_ARRAY_BUFFER, colorBuffer->size, colorBuffer->data, GL_STATIC_DRAW);
        object.hasColor = true;
        mat.type = MT_color;
        mat.diffuse.diffuseColor = glm::vec3(1, 1, 1);
        mat.materialShader = renderContext.colorShader;
        object.colorCount = colorBuffer->count;
    }
    
    if(normalBuffer)
    {
        glGenBuffers(1, &object.normalBufferHandle);
        glBindBuffer(GL_ARRAY_BUFFER, object.normalBufferHandle);
        glBufferData(GL_ARRAY_BUFFER, normalBuffer->size, normalBuffer->data, GL_STATIC_DRAW);
        object.hasNormals = true;
        object.normalCount = normalBuffer->count;
    }
    glBindVertexArray(0);
    
    return object;
}

static void UpdateBuffer(gl_buffer newBuffer, int bufferHandle)
{
    if(newBuffer.size != 0 && newBuffer.data)
    {
        glBindBuffer(GL_ARRAY_BUFFER, bufferHandle);
        glBufferData(GL_ARRAY_BUFFER, newBuffer.size, newBuffer.data, GL_STATIC_DRAW);
    }
}

static void RenderQuad(render_context& renderContext, glm::vec3 position = glm::vec3(0.0f), glm::quat orientation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f), glm::vec3 scale = glm::vec3(1.0f), glm::vec4 color = glm::vec4(1.0f))
{
    glBindVertexArray(renderContext.quadVAO);
    
    renderContext.right = glm::normalize(glm::cross(renderContext.direction, renderContext.up));
    
    auto m = ComputeTransformation(scale, orientation, position);
    
    SetMatrixUniform(renderContext.basicShader.programID, "M", m);
    SetMatrixUniform(renderContext.basicShader.programID, "V", renderContext.viewMatrix);
    SetMatrixUniform(renderContext.basicShader.programID, "P", renderContext.projectionMatrix);
    SetVec4Uniform(renderContext.basicShader.programID, "c", color);
    
    glDrawElements(GL_TRIANGLES, sizeof(renderContext.quadIndices), GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
}

static glm::vec3 ToObjectCoords()
{
}

static void RenderModel(render_context& renderContext, model& m)
{
    auto& material = m.material;
    glUseProgram(material.materialShader.programID);
    ComputeModelTransformation(m);
    SetMatrixUniform(material.materialShader.programID, "M", m.transform);
    SetMatrixUniform(material.materialShader.programID, "V", renderContext.viewMatrix);
    SetMatrixUniform(material.materialShader.programID, "P", renderContext.projectionMatrix);
    
    auto lightPos = glm::vec3(1, 1, 1);
    auto lightColor = glm::vec3(1, 1, 1);
    auto lightPower = 2.0f;
    
    if(renderContext.lightCount > 0)
    {
        auto& light = renderContext.lights[0];
        lightPos = renderContext.position; //light.position;
        lightColor = light.color;
        lightPower = light.power;
    }
    
    SetVec3Uniform(material.materialShader.programID, "lightPosWorld", lightPos);
    SetVec3Uniform(material.materialShader.programID, "lightColor", lightColor);
    SetFloatUniform(material.materialShader.programID, "lightPower", lightPower);
    SetVec3Uniform(material.materialShader.programID, "specularColor", material.specularColor);
    SetFloatUniform(material.materialShader.programID, "alpha", material.alpha);
    
    if(material.type == MT_color)
    {
        SetVec3Uniform(material.materialShader.programID, "diffuseColor", material.diffuse.diffuseColor);
    }
    
    glBindVertexArray(m.VAO);
    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    
    auto vbo = BuildVertexBuffer(m.faces, m.numFaces);
    m.vertexCount = m.numFaces * 3;
    glBufferData(GL_ARRAY_BUFFER, sizeof(vbo), vbo, GL_DYNAMIC_DRAW);
    //free(vbo);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    if(m.hasUV)
    {
        glBindTexture(GL_TEXTURE_2D, m.material.texture.tex.textureID);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, m.uvBufferHandle);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    }
    
    if(m.hasNormals)
    {
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, m.normalBufferHandle);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    }
    
    printf("%d\n", m.vertexCount);
    glDrawArrays(GL_TRIANGLES, 0, m.vertexCount * 3);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindVertexArray(0);
}

static void RenderGrid(render_context& renderContext, glm::vec4 color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), float lineWidth = 2.0f)
{
    float gridSize = 15.0f;
    glm::vec3 offset = renderContext.originOffset;
    for(int x = 0; x < gridSize; x++)
    {
        for(int y = 0; y < gridSize; y++)
        {
            RenderLine(renderContext, glm::vec3(x, 0.0f, y) - offset, glm::vec3(gridSize, 0.0f, y) - offset, color, lineWidth);
            RenderLine(renderContext, glm::vec3(x, 0.0f, y) - offset, glm::vec3(x, 0.0f, gridSize) - offset, color, lineWidth);
        }
    }
    
    RenderLine(renderContext, glm::vec3(gridSize, 0.0f, 0.0f) - offset, glm::vec3(gridSize, 0.0f, gridSize) - offset, color, lineWidth);
    RenderLine(renderContext, glm::vec3(0.0f, 0.0f, gridSize) - offset, glm::vec3(gridSize, 0.0f, gridSize) - offset, color, lineWidth);
}


static void Render(render_context& renderContext)
{
    for(int modelIndex = 0; modelIndex < renderContext.modelCount; modelIndex++)
    {
        RenderModel(renderContext, renderContext.models[modelIndex]);
    }
    RenderGrid(renderContext, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f), 1.0f);
}

static void ComputeMatrices(render_context& renderContext, double deltaTime)
{
    if(renderContext.FoV >= 1.0f && renderContext.FoV <= 45.0f)
        renderContext.FoV -= inputState.yScroll;
    if(renderContext.FoV <= 1.0f)
        renderContext.FoV = 1.0f;
    if(renderContext.FoV >= 45.0f)
        renderContext.FoV = 45.0f;
    
    renderContext.projectionMatrix = glm::perspective(glm::radians(renderContext.FoV), (float)renderContext.screenWidth / (float)renderContext.screenHeight, renderContext.near, renderContext.far);
    
    float panSpeed = 7.0f * (float)deltaTime;
    if(glfwGetKey(renderContext.window, Key_W) == GLFW_PRESS)
    {
        renderContext.position += panSpeed * renderContext.direction;
    }
    if(glfwGetKey(renderContext.window, Key_S) == GLFW_PRESS)
    {
        renderContext.position -= panSpeed * renderContext.direction;
    }
    if(glfwGetKey(renderContext.window, Key_A) == GLFW_PRESS)
    {
        renderContext.position -= glm::normalize(glm::cross(renderContext.direction, renderContext.up)) * panSpeed;
    }
    if(glfwGetKey(renderContext.window, Key_D) == GLFW_PRESS)
    {
        renderContext.position += glm::normalize(glm::cross(renderContext.direction, renderContext.up)) * panSpeed;
    }
    
    if(glfwGetMouseButton(renderContext.window, Key_MouseLeft) == GLFW_PRESS)
    {
        glm::vec3 front;
        front.x = (float)(cos(glm::radians(inputState.mousePitch)) * cos(glm::radians(inputState.mouseYaw)));
        front.y = (float)(sin(glm::radians(inputState.mousePitch)));
        front.z = (float)(cos(glm::radians(inputState.mousePitch)) * sin(glm::radians(inputState.mouseYaw)));
        renderContext.direction = glm::normalize(front);
    }
    if(glfwGetMouseButton(renderContext.window, Key_MouseRight) == GLFW_PRESS)
    {
        auto right = glm::normalize(glm::cross(renderContext.direction, renderContext.up));
        renderContext.position += right * panSpeed * (float)-inputState.xDelta;
        auto up = glm::normalize(glm::cross(right, renderContext.direction));
        renderContext.position += glm::normalize(up) * panSpeed * (float)-inputState.yDelta;
    }
    
    renderContext.right = glm::normalize(glm::cross(renderContext.direction, renderContext.up));
    
    renderContext.viewMatrix = glm::lookAt(renderContext.position, renderContext.position + renderContext.direction, renderContext.up);
}

static texture LoadTexture(const char* Path)
{
    texture newTex;
    newTex.data = stbi_load(Path, &newTex.width, &newTex.height, 0, STBI_rgb_alpha);
    
    if(!newTex.data)
    {
        fprintf(stderr, "Could not load texture\n");
        return newTex;
    }
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    newTex.textureID = textureID;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newTex.width, newTex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, newTex.data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    return newTex;
}



