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

static void ComputeMeshTransformation(mesh& object)
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
        char* buffer = (char*)malloc((infoLogLength + 1) * sizeof(char));
        glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL, &buffer[0]);
        printf("%s\n", &buffer[0]);
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
        char* buffer = (char*)malloc((infoLogLength + 1) * sizeof(char));
        glGetShaderInfoLog(programID, infoLogLength, NULL, &buffer[0]);
        printf("%s\n", &buffer[0]);
        printf("%d\n", result);
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
    glfwSetKeyCallback(renderContext.window, KeyCallback);
    glfwSetMouseButtonCallback(renderContext.window, MouseButtonCallback);
    
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
    
    //glfwSetInputMode(renderContext.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    
    
    renderContext.textureShader = LoadShaders("../shaders/texture.vert", "../shaders/texture.frag");
    renderContext.colorShader = LoadShaders("../shaders/color.vert", "../shaders/color.frag");
    renderContext.basicShader = LoadShaders("../shaders/basic.vert", "../shaders/basic.frag");
    renderContext.particleShader = LoadShaders("../shaders/particle.vert", "../shaders/particle.frag");
    
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
    
    // Initialize point cloud
    glGenVertexArrays(1, &renderContext.pointCloudVAO);
    glBindVertexArray(renderContext.pointCloudVAO);
    
    static const GLfloat vbData[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
    };
    
    glGenBuffers(1, &renderContext.billboardVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderContext.billboardVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vbData), vbData, GL_STATIC_DRAW);
    glGenBuffers(1, &renderContext.particlesVBO);
    glGenBuffers(1, &renderContext.particlesColorVBO);
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
    
    GLfloat points[18] = {v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, v3.x, v3.y, v3.z, v2.x, v2.y, v2.z, v4.x, v4.y, v4.z};
    glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), &points[0], GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    
    auto m = glm::scale(glm::mat4(1.0f), glm::vec3(globalScale));
    
    SetMatrixUniform(renderContext.basicShader.programID, "M", m);
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

static GLfloat* BuildVertexBuffer(face* faces, int numFaces, vertex* input)
{
    GLfloat* vertices = (GLfloat*)malloc(numFaces * 3 * sizeof(vertex_info));
    for(int i = 0; i < numFaces; i++)
    {
        auto v1 = input[faces[i].vertices[0]];
        auto v2 = input[faces[i].vertices[1]];
        auto v3 = input[faces[i].vertices[2]];
        // First vertex
        vertices[i * 30] = v1.position.x;
        vertices[i * 30 + 1] = v1.position.y;
        vertices[i * 30 + 2] = v1.position.z;
        
        // First normal
        vertices[i * 30 + 3] = faces[i].faceNormal.x;
        vertices[i * 30 + 4] = faces[i].faceNormal.y;
        vertices[i * 30 + 5] = faces[i].faceNormal.z;
        
        // First color
        vertices[i * 30 + 6] = faces[i].faceColor.x;//v1.color.x;
        vertices[i * 30 + 7] = faces[i].faceColor.y;//v1.color.y;
        vertices[i * 30 + 8] = faces[i].faceColor.z;//v1.color.z;
        vertices[i * 30 + 9] = faces[i].faceColor.w;//v1.color.w;
        
        // Second vertex
        vertices[i * 30 + 10] = v2.position.x;
        vertices[i * 30 + 11] = v2.position.y;
        vertices[i * 30 + 12] = v2.position.z;
        
        // Second normal
        vertices[i * 30 + 13] = faces[i].faceNormal.x;
        vertices[i * 30 + 14] = faces[i].faceNormal.y;
        vertices[i * 30 + 15] = faces[i].faceNormal.z;
        
        // Second color
        vertices[i * 30 + 16] = faces[i].faceColor.x;//v2.color.x;
        vertices[i * 30 + 17] = faces[i].faceColor.y;//v2.color.y;
        vertices[i * 30 + 18] = faces[i].faceColor.z;//v2.color.z;
        vertices[i * 30 + 19] = faces[i].faceColor.w;//v2.color.w;
        
        // Third vertex
        vertices[i * 30 + 20] = v3.position.x;
        vertices[i * 30 + 21] = v3.position.y;
        vertices[i * 30 + 22] = v3.position.z;
        
        // Third normal
        vertices[i * 30 + 23] = faces[i].faceNormal.x;
        vertices[i * 30 + 24] = faces[i].faceNormal.y;
        vertices[i * 30 + 25] = faces[i].faceNormal.z;
        
        // Third color
        vertices[i * 30 + 26] = faces[i].faceColor.x;//v3.color.x;
        vertices[i * 30 + 27] = faces[i].faceColor.y;//v3.color.y;
        vertices[i * 30 + 28] = faces[i].faceColor.z;//v3.color.z;
        vertices[i * 30 + 29] = faces[i].faceColor.w;//v3.color.w;
    }
    return vertices;
}

static glm::vec3 ComputeFaceNormal(render_context& renderContext, face f, vertex* vertices)
{
    // Newell's Method
    // https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal
    glm::vec3 normal = glm::vec3(0.0f);
    
    for(int i = 0; i < 3; i++)
    {
        auto& current = vertices[f.vertices[i]].position;
        auto& next = vertices[f.vertices[(i + 1) % 3]].position;
        
        normal.x = normal.x + (current.y - next.y) * (current.z + next.z);
        normal.y = normal.y + (current.z - next.z) * (current.x + next.x);
        normal.z = normal.z + (current.x - next.x) * (current.y + next.y);
    }
    
    return glm::normalize(normal);
}

static face& AddFace(render_context& renderContext, mesh& m, vertex& v1, vertex& v2, vertex& v3, vertex* vertices)
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
            m.facesSize *= 2;
            m.faces = (face*)realloc(m.faces, m.facesSize * sizeof(face));
        }
    }
    
    face newFace = {};
    
    //!!!TODO: This is extremely slow now, but naive solution is fine for vertical slice!!!
    // Find face neighbours: i.e. faces sharing two vertices
    for(int v1FaceIndex = 0; v1FaceIndex < v1.numFaceHandles; v1FaceIndex++)
    {
        for(int v2FaceIndex = 0; v2FaceIndex < v2.numFaceHandles; v2FaceIndex++)
        {
            if(v1.faceHandles[v1FaceIndex] == v2.faceHandles[v2FaceIndex])
            {
                newFace.neighbours[0].faceHandle = v1.faceHandles[v1FaceIndex];
                newFace.neighbours[0].originVertex = v1.vertexIndex;
                newFace.neighbours[0].endVertex = v2.vertexIndex;
                m.faces[newFace.neighbours[0].faceHandle].visited = false;
            }
        }
        
        for(int v3FaceIndex = 0; v3FaceIndex < v3.numFaceHandles; v3FaceIndex++)
        {
            if(v1.faceHandles[v1FaceIndex] == v3.faceHandles[v3FaceIndex])
            {
                newFace.neighbours[1].faceHandle = v1.faceHandles[v1FaceIndex];
                newFace.neighbours[1].originVertex = v1.vertexIndex;
                newFace.neighbours[1].endVertex = v3.vertexIndex;
                m.faces[newFace.neighbours[1].faceHandle].visited = false;
            }
        }
    }
    
    for(int v2FaceIndex = 0; v2FaceIndex < v2.numFaceHandles; v2FaceIndex++)
    {
        for(int v3FaceIndex = 0; v3FaceIndex < v3.numFaceHandles; v3FaceIndex++)
        {
            if(v2.faceHandles[v2FaceIndex] == v3.faceHandles[v3FaceIndex])
            {
                newFace.neighbours[2].faceHandle = v2.faceHandles[v2FaceIndex];
                newFace.neighbours[2].originVertex = v2.vertexIndex;
                newFace.neighbours[2].endVertex = v3.vertexIndex;
                m.faces[newFace.neighbours[2].faceHandle].visited = false;
            }
        }
    }
    
    v1.faceHandles[v1.numFaceHandles++] = m.numFaces;
    v2.faceHandles[v2.numFaceHandles++] = m.numFaces;
    v3.faceHandles[v3.numFaceHandles++] = m.numFaces;
    newFace.vertices[0] = v1.vertexIndex;
    newFace.vertices[1] = v2.vertexIndex;
    newFace.vertices[2] = v3.vertexIndex;
    newFace.faceNormal = ComputeFaceNormal(renderContext, newFace, vertices);
    
    auto centerPoint = (vertices[newFace.vertices[0]].position + vertices[newFace.vertices[1]].position + vertices[newFace.vertices[2]].position)/3.0f;
    
    auto v1C = v1.position - centerPoint;
    auto v2C = v2.position - centerPoint;
    
    auto counterclockwise = glm::dot(newFace.faceNormal, glm::cross(v1C, v2C)) >= 0.0f;
    
    if(!counterclockwise)
    {
        newFace.faceNormal = -newFace.faceNormal;
    }
    
    newFace.faceColor = RandomColor();
    newFace.faceColor.w = 0.5f;
    newFace.indexInMesh = m.numFaces;
    
    m.faces[m.numFaces++] = newFace;
    
    m.dirty = true;
    return m.faces[m.numFaces - 1];
}

static void RemoveFace(mesh& m, face* f)
{
    //TODO: Remove face index from vertices
    bool found = false;
    //free(f->neighbours);
    for(int faceIndex = 0; faceIndex < m.numFaces; faceIndex++)
    {
        if(!found && &m.faces[faceIndex] == f)
        {
            found = true;
        }
        else if(found)
        {
            m.faces[faceIndex - 1] = m.faces[faceIndex];
        }
    }
    
    if(found)
    {
        m.numFaces--;
    }
}

static mesh& InitEmptyMesh(render_context& renderContext)
{
    mesh& object = renderContext.meshes[renderContext.meshCount++];
    
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
    object.dirty = true;
    
    glBindVertexArray(0);
    
    return object;
}

static mesh& LoadMesh(render_context& renderContext, gl_buffer vbo, gl_buffer* uvBuffer = 0, gl_buffer* colorBuffer = 0, gl_buffer* normalBuffer = 0, glm::vec3 diffuseColor = glm::vec3(1.0f))
{
    mesh& object = renderContext.meshes[renderContext.meshCount++];
    
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

static void RenderPointCloud(render_context& renderContext, vertex* inputPoints, int numPoints)
{
    glBindVertexArray(renderContext.pointCloudVAO);
    glUseProgram(renderContext.particleShader.programID);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderContext.particlesVBO);
    glBufferData(GL_ARRAY_BUFFER, numPoints * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    
    GLfloat* positions = (GLfloat*)malloc(numPoints * sizeof(GLfloat) * 4);
    GLfloat* colors = (GLfloat*)malloc(numPoints * sizeof(GLfloat) * 4);
    
    for(int i = 0; i < numPoints; i++)
    {
        positions[4 * i + 0] = inputPoints[i].position.x;
        positions[4 * i + 1] = inputPoints[i].position.y;
        positions[4 * i + 2] = inputPoints[i].position.z;
        positions[4 * i + 3] = 50.0f;
        auto c = inputPoints[i].color;
        colors[4 * i + 0] = c.x;
        colors[4 * i + 1] = c.y;
        colors[4 * i + 2] = c.z;
        colors[4 * i + 3] = c.w;
    }
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, numPoints * sizeof(GLfloat) * 4, positions);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderContext.particlesColorVBO);
    glBufferData(GL_ARRAY_BUFFER, numPoints * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, numPoints * sizeof(GLfloat) * 4, colors);
    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, renderContext.billboardVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, renderContext.particlesVBO);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, renderContext.particlesColorVBO);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    
    auto m = glm::scale(glm::mat4(1.0f), glm::vec3(globalScale));
    SetMatrixUniform(renderContext.basicShader.programID, "M", m);
    SetMatrixUniform(renderContext.basicShader.programID, "V", renderContext.viewMatrix);
    SetMatrixUniform(renderContext.basicShader.programID, "P", renderContext.projectionMatrix);
    
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numPoints);
    
    glBindVertexArray(0);
    free(positions);
    free(colors);
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

static void RenderMesh(render_context& renderContext, mesh& m, vertex* vertices)
{
    auto& material = m.material;
    glUseProgram(material.materialShader.programID);
    ComputeMeshTransformation(m);
    SetMatrixUniform(material.materialShader.programID, "M", m.transform);
    SetMatrixUniform(material.materialShader.programID, "V", renderContext.viewMatrix);
    SetMatrixUniform(material.materialShader.programID, "P", renderContext.projectionMatrix);
    
    auto lightPos = glm::vec3(1, 1, 1);
    auto lightColor = glm::vec3(1, 1, 1);
    auto lightPower = 2.0f;
    
    if(renderContext.lightCount > 0)
    {
        auto& light = renderContext.lights[0];
        lightPos = light.position;
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
    
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    if(m.dirty)
    {
        free(m.currentVBO);
        m.dirty = false;
        m.currentVBO = BuildVertexBuffer(m.faces, m.numFaces, vertices);
    }
    
    m.vertexCount = m.numFaces * 3;
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_info) * m.vertexCount, &m.currentVBO[0], GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_info), (void*)offsetof(vertex, position));
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_info), (void*)offsetof(vertex, normal));
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_info), (void*) offsetof(vertex, color));
    
    
    glDrawArrays(GL_TRIANGLES, 0, m.vertexCount * 10);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindVertexArray(0);
    
    auto lineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    auto lineWidth = 10.0f;
    auto normalColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    auto faceNormalColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
    
    auto lineLength = 20.0f;
    
    auto c1 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    auto c2 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    auto c3 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    auto vertexSum = glm::vec3(0.0f);
    
    if(renderContext.renderNormals)
    {
        
        for(int i = 0; i < m.numFaces; i++)
        {
            auto& f = m.faces[i];
            
            auto v1 = vertices[f.vertices[0]];
            auto v2 = vertices[f.vertices[1]];
            auto v3 = vertices[f.vertices[2]];
            
            RenderLine(renderContext, v1.position, v1.position + f.faceNormal * lineLength, normalColor, lineWidth);
            RenderLine(renderContext, v2.position, v2.position + f.faceNormal * lineLength, normalColor, lineWidth);
            RenderLine(renderContext, v3.position, v3.position + f.faceNormal * lineLength, normalColor, lineWidth);
            RenderLine(renderContext, v1.position, v2.position, c1, lineWidth);
            RenderLine(renderContext, v2.position, v3.position, c2, lineWidth);
            RenderLine(renderContext, v3.position, v1.position, c3, lineWidth);
            
            auto centerPoint = (v1.position + v2.position + v3.position)/3.0f;
            
            RenderLine(renderContext, centerPoint, centerPoint + lineLength * f.faceNormal, faceNormalColor, lineWidth);
        }
    }
    else
    {
        for(int i = 0; i < m.numFaces; i++)
        {
            auto& f = m.faces[i];
            
            auto v1 = vertices[f.vertices[0]];
            auto v2 = vertices[f.vertices[1]];
            auto v3 = vertices[f.vertices[2]];
            
            if(i == 0)
            {
                RenderLine(renderContext, v1.position, v2.position, c1, lineWidth);
                RenderLine(renderContext, v2.position, v3.position, c2, lineWidth);
                RenderLine(renderContext, v3.position, v1.position, c3, lineWidth);
            }
            
        }
    }
    
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


static void Render(render_context& renderContext, vertex* vertices)
{
    for(int meshIndex = 0; meshIndex < renderContext.meshCount; meshIndex++)
    {
        RenderMesh(renderContext, renderContext.meshes[meshIndex], vertices);
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
    
    float panSpeed = 10.0f * (float)deltaTime;
    if(Key(Key_W))
    {
        renderContext.position += panSpeed * renderContext.direction;
    }
    if(Key(Key_S))
    {
        renderContext.position -= panSpeed * renderContext.direction;
    }
    if(Key(Key_A))
    {
        renderContext.position -= glm::normalize(glm::cross(renderContext.direction, renderContext.up)) * panSpeed;
    }
    if(Key(Key_D))
    {
        renderContext.position += glm::normalize(glm::cross(renderContext.direction, renderContext.up)) * panSpeed;
    }
    
    if(Mouse(MouseLeft))
    {
        glm::vec3 front;
        front.x = (float)(cos(glm::radians(inputState.mousePitch)) * cos(glm::radians(inputState.mouseYaw)));
        front.y = (float)(sin(glm::radians(inputState.mousePitch)));
        front.z = (float)(cos(glm::radians(inputState.mousePitch)) * sin(glm::radians(inputState.mouseYaw)));
        renderContext.direction = glm::normalize(front);
    }
    if(Mouse(MouseRight))
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



