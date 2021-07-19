#define GL (*global_graphics->opengl)
#define D3D (*global_graphics->d3d)

#ifdef DEBUG
#   define D3DCall(x) do{HRESULT r=(x);if(FAILED(r)){Platform_DebugMessageBox("D3DCall Failed\nFile: " __FILE__ "\nLine: %i\nError code: %lx",__LINE__,r);exit(1);}}while(0)
#else
#   define D3DCall(x) (x)
#endif

#define TEXTURE_SLOT_DIFFUSE 0
#define TEXTURE_SLOT_NORMAL 1
#define TEXTURE_SLOT_SPECULAR 2
#define TEXTURE_SLOT_SHADOWMAP 3

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 texcoord;
} typedef Vertex;

enum RenderBufferMode
{
    RenderBufferMode_None = 0,
    
    RenderBufferMode_3DModel,
    RenderBufferMode_Sprite,
} typedef RenderBufferMode;

struct RenderBuffer
{
    uint32 texture;
    uint32 vao, ebo;
    int32 ebo_size;
    
    RenderBufferMode mode;
    int32 len;
    int32 cap;
    
    void* elements;
} typedef RenderBuffer;

struct RenderBufferElement3DModel
{
    mat4 model;
    mat4 inversed_model;
} typedef RenderBufferElement3DModel;

struct PointLightsUniformData
{
    int32 constant;
    int32 linear;
    int32 quadratic;
    
    int32 position;
    int32 ambient;
    int32 diffuse;
    int32 specular;
} typedef PointLightsUniformData;

//~ Globals
internal const GraphicsContext* global_graphics;
internal const Vertex global_quad_vertices[] = {
    // Positions         // Normals           // Texcoords
    { 0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, },
    { 1.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f, },
    { 1.0f, 1.0f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f, },
    { 0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f, },
};

internal const uint32 global_quad_indices[] = {
    0, 1, 2,
    2, 3, 0
};

internal float32 global_width;
internal float32 global_height;
internal mat4 global_view;
internal uint32 global_quad_vbo, global_quad_ebo, global_quad_vao;
internal uint32 global_default_diffuse_texture;
internal uint32 global_default_normal_texture;
internal uint32 global_default_specular_texture;
internal vec3 global_viewpos;

internal uint32 global_default_shader;
internal uint32 global_default_3dshader;
internal uint32 global_default_shadowshader;
internal uint32 global_default_gbuffershader;
internal uint32 global_default_finalpassshader;
internal uint32 global_current_shader;

internal int32 global_uniform_color;
internal int32 global_uniform_view;
internal int32 global_uniform_model;
internal int32 global_uniform_texture;
internal int32 global_uniform_inversed_model;
internal int32 global_uniform_viewpos;

internal int32 global_uniform_position;
internal int32 global_uniform_normal;
internal int32 global_uniform_albedo;
internal int32 global_uniform_depth;

internal int32 global_uniform_material_ambient;
internal int32 global_uniform_material_diffuse;
internal int32 global_uniform_material_specular;
internal int32 global_uniform_material_normal;
internal int32 global_uniform_material_shininess;

internal int32 global_uniform_dirlight_position;
internal int32 global_uniform_dirlight_ambient;
internal int32 global_uniform_dirlight_diffuse;
internal int32 global_uniform_dirlight_specular;
internal int32 global_uniform_dirlight_matrix;
internal int32 global_uniform_dirlight_shadowmap;

internal int32 global_uniform_pointlights_count;
internal PointLightsUniformData global_uniform_pointlights[MAX_POINT_LIGHTS_COUNT];

internal const char* const global_vertex_shader =
"#version 330 core\n"
"layout (location = 0) in vec3 aPosition;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec2 aTexCoord;\n"

"out vec2 vTexCoord;"

"uniform mat4 uView;\n"
"uniform mat4 uModel;\n"

"void main() {"
"    gl_Position = uView * uModel * vec4(aPosition, 1.0);"
"    vTexCoord = aTexCoord;"
"}"
;

internal const char* const global_fragment_shader =
"#version 330 core\n"

"in vec2 vTexCoord;"

"out vec4 oFragColor;"

"uniform vec4 uColor;"
"uniform sampler2D uTexture;"

"void main() {"
"    oFragColor = uColor * texture(uTexture, vTexCoord);"
"}"
;

internal const char* const global_vertex_shadowshader =
"#version 330 core\n"
"layout (location = 0) in vec3 aPosition;\n"

"uniform mat4 uView;\n"
"uniform mat4 uModel;\n"

"void main() {"
"    gl_Position = uView * uModel * vec4(aPosition, 1.0);"
"}"
;

internal const char* const global_fragment_shadowshader =
"#version 330 core\n"

"void main() {"
"}"
;

internal const char* const global_vertex_gbuffershader =
"#version 330 core\n"
"layout (location = 0) in vec3 aPosition;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"layout (location = 3) in vec4 aTangent;\n"

"out vec2 vTexCoord;"
"out vec3 vFragPos;"
"out mat3 vTbn;"

"uniform mat4 uView;\n"
"uniform mat4 uModel;\n"

"void main() {"
"    vec4 fragPos = uModel * vec4(aPosition, 1.0);"
"    gl_Position = uView * fragPos;"
"    vTexCoord = aTexCoord;"

"    vec3 t = normalize(mat3(uModel) * aTangent.xyz);"
"    vec3 n = normalize(mat3(uModel) * aNormal);"
"    vec3 b = cross(n, t) * aTangent.w;"

"    vTbn = mat3(t, b, n);"

"    vFragPos = vec3(fragPos);"
"}"
;

internal const char* const global_fragment_gbuffershader =
"#version 330 core\n"

"in vec2 vTexCoord;"
"in vec3 vFragPos;"
"in mat3 vTbn;"

"layout (location = 0) out vec4 oPosition;"
"layout (location = 1) out vec4 oNormal;"
"layout (location = 2) out vec4 oAlbedo;"

"struct Material {"
"    vec3 ambient;" /* unsued */
"    sampler2D diffuse;"
"    sampler2D specular;"
"    sampler2D normal;"
"    float shininess;"
"};"

"uniform Material uMaterial;"
"uniform vec4 uColor;"

"void main() {"
"    oPosition.rgb = vFragPos;"
"    oPosition.a = uMaterial.shininess;"
"    oNormal = vec4(normalize(vTbn * (texture(uMaterial.normal, vTexCoord).rgb * 2.0 - 1.0)), 1.0);"
"    oAlbedo.rgb = vec3(uColor) * texture(uMaterial.diffuse, vTexCoord).rgb;"
"    oAlbedo.a = texture(uMaterial.specular, vTexCoord).r;"
"}"
;

internal const char* const global_vertex_finalpassshader =
"#version 330 core\n"
"layout (location = 0) in vec3 aPosition;\n"
"layout (location = 2) in vec2 aTexCoord;\n"

"out vec2 vTexCoord;"

"uniform mat4 uModel;\n"

"void main() {"
"    gl_Position = uModel * vec4(aPosition, 1.0);"
"    vTexCoord = aTexCoord;"
"}"
;

internal const char* const global_fragment_finalpassshader =
"#version 330 core\n"

"in vec2 vTexCoord;"

"out vec4 oFragColor;\n"

"struct DirLight {"
"    sampler2D shadowmap;"
"    vec3 position;"
"    vec3 ambient;"
"    vec3 diffuse;"
"    vec3 specular;"
"};"

"struct PointLight {"
"    float constant;"
"    float linear;"
"    float quadratic;"

"    vec3 position;"
"    vec3 ambient;"
"    vec3 diffuse;"
"    vec3 specular;"
"};"

"uniform DirLight uDirLight;"
"uniform PointLight uPointLights[" StrMacro(MAX_POINT_LIGHTS_COUNT) "];"
"uniform int uPointLightsCount;"
"uniform vec3 uViewPos;"
"uniform mat4 uDirLightMatrix;\n"

"uniform sampler2D uPosition;"
"uniform sampler2D uNormal;"
"uniform sampler2D uAlbedo;"
"uniform sampler2D uDepth;"

"float map(float v, float a1, float b1, float a2, float b2) {"
"    return (v - a1) / (b1 - a1) * (b2 - a2) + a2;"
"}"

"float CalculateShadow(vec3 fragPos) {"
"    vec4 fragPosInDirLight = uDirLightMatrix * vec4(fragPos, 1.0);"
"    vec3 coords = fragPosInDirLight.xyz / fragPosInDirLight.w;"
"    coords = coords * 0.5 + 0.5;"
"    const float bias = 0.005;"
"    float currentDepth = coords.z - bias;"
"    vec2 texel = 1.0 / textureSize(uDirLight.shadowmap, 0) * 0.5;"

"    float shadow = 0.0;"

"    shadow += currentDepth > texture(uDirLight.shadowmap, coords.xy+vec2(-1.0, 1.0)*texel).r ? 1.0 : 0.0;"
"    shadow += currentDepth > texture(uDirLight.shadowmap, coords.xy+vec2(0.0, 1.0)*texel).r ? 1.0 : 0.0;"
"    shadow += currentDepth > texture(uDirLight.shadowmap, coords.xy+vec2(1.0, 1.0)*texel).r ? 1.0 : 0.0;"
"    shadow += currentDepth > texture(uDirLight.shadowmap, coords.xy+vec2(-1.0, 0.0)*texel).r ? 1.0 : 0.0;"
"    shadow += currentDepth > texture(uDirLight.shadowmap, coords.xy+vec2(0.0, 0.0)*texel).r ? 1.0 : 0.0;"
"    shadow += currentDepth > texture(uDirLight.shadowmap, coords.xy+vec2(1.0, 0.0)*texel).r ? 1.0 : 0.0;"
"    shadow += currentDepth > texture(uDirLight.shadowmap, coords.xy+vec2(-1.0, -1.0)*texel).r ? 1.0 : 0.0;"
"    shadow += currentDepth > texture(uDirLight.shadowmap, coords.xy+vec2(0.0, -1.0)*texel).r ? 1.0 : 0.0;"
"    shadow += currentDepth > texture(uDirLight.shadowmap, coords.xy+vec2(1.0, -1.0)*texel).r ? 1.0 : 0.0;"

"    shadow /= 9.0;"

"    return shadow;"
"}"

"vec3 CalculateDirLight(vec3 normal, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float shininess, vec3 fragPos) {"
"    vec3 halfwayDir = normalize(uDirLight.position + viewDir);"

"    vec3 ambient = diffuseSample * 0.1;"
"    vec3 diffuse = diffuseSample * max(0.0, dot(normal, halfwayDir));"
"    vec3 specular = specularSample * pow(max(0.0, dot(normal, halfwayDir)), shininess);"

"    ambient *= uDirLight.ambient;"
"    diffuse *= uDirLight.diffuse;"
"    specular *= uDirLight.specular;"

"    float shadow = CalculateShadow(fragPos);"
"    return ambient + (1.0 - shadow) * (diffuse + specular);"
"}"

"vec3 CalculatePointLight(vec3 normal, vec3 viewDir, vec3 diffuseSample, vec3 specularSample, float shininess, vec3 fragPos, int i) {"
"    vec3 lightDir = normalize(uPointLights[i].position - fragPos);"
"    vec3 halfwayDir = normalize(lightDir + viewDir);"

"    float diff = max(dot(normal, halfwayDir), 0.0);"
"    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);"

"    float distance = length(uPointLights[i].position - fragPos);"
"    float attenuation = 1.0 / (uPointLights[i].constant + uPointLights[i].linear * distance + uPointLights[i].quadratic * (distance * distance));"

"    vec3 ambient  = uPointLights[i].ambient * diffuseSample;"
"    vec3 diffuse  = uPointLights[i].diffuse * diff * diffuseSample;"
"    vec3 specular = uPointLights[i].specular * spec * specularSample;"

"    ambient  *= attenuation;"
"    diffuse  *= attenuation;"
"    specular *= attenuation;"
"    return ambient + diffuse + specular;"
"}"

"void main() {"
"    vec4 normalSample = texture(uNormal, vTexCoord);"
"    vec4 positionSample = texture(uPosition, vTexCoord);"
"    vec4 albedoSample = texture(uAlbedo, vTexCoord);"
"    gl_FragDepth = texture(uDepth, vTexCoord).r;"

"    if ((normalSample.x+normalSample.y+normalSample.z) == 0.0) { discard; }"

"    vec3 normal = normalSample.rgb;"
"    vec3 fragPos = positionSample.rgb;"
"    vec3 diffuseSample = albedoSample.rgb;"
"    vec3 specularSample = vec3(albedoSample.a);"
"    float shininess = positionSample.a;"

"    vec3 viewDir = normalize(uViewPos - fragPos);"

"    vec3 color = CalculateDirLight(normal, viewDir, diffuseSample, specularSample, shininess, fragPos);"
"    for(int i = 0; i < uPointLightsCount; ++i) {"
"        color += CalculatePointLight(normal, viewDir, diffuseSample, specularSample, shininess, fragPos, i);"
"    }"

"    oFragColor = vec4(color, 1.0);"
"}\n"
;

//~ Functions
#ifdef DEBUG
internal void APIENTRY
OpenGLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                           GLsizei length, const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR)
    {
        Platform_DebugMessageBox("OpenGL Error:\n\nType = 0x%x\nID = %u\nSeverity = 0x%x\nMessage= %s",
                                 type, id, severity, message);
    }
    else
    {
        Platform_DebugLog("OpenGL Debug Callback:\n\tType = 0x%x\n\tID = %u\n\tSeverity = 0x%x\n\tmessage = %s\n",
                          type, id, severity, message);
    }
}
#endif

internal uint32
CompileShader(const char* vertex_source, const char* fragment_source)
{
    Trace("CompileShader");
    
    char info[512];
    int32 success;
    
    // Compile Vertex Shader
    uint32 vertex_shader = GL.glCreateShader(GL_VERTEX_SHADER);
    GL.glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    GL.glCompileShader(vertex_shader);
    
    // Check for errors
    GL.glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GL.glGetShaderInfoLog(vertex_shader, sizeof info, NULL, info);
        Platform_DebugMessageBox("Vertex Shader Error:\n\n%s", info);
        GL.glDeleteShader(vertex_shader);
        return 0;
    }
    
    // Compile Fragment Shader
    uint32 fragment_shader = GL.glCreateShader(GL_FRAGMENT_SHADER);
    GL.glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    GL.glCompileShader(fragment_shader);
    
    // Check for errors
    GL.glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GL.glGetShaderInfoLog(fragment_shader, sizeof info, NULL, info);
        Platform_DebugMessageBox("Fragment Shader Error:\n\n%s", info);
        GL.glDeleteShader(vertex_shader);
        GL.glDeleteShader(fragment_shader);
        return 0;
    }
    
    // Link
    uint32 program = GL.glCreateProgram();
    GL.glAttachShader(program, vertex_shader);
    GL.glAttachShader(program, fragment_shader);
    GL.glLinkProgram(program);
    
    // Check for errors
    GL.glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GL.glGetProgramInfoLog(program, sizeof info, NULL, info);
        Platform_DebugMessageBox("Shader Linking Error:\n\n%s", info);
        GL.glDeleteProgram(program);
        program = 0;
    }
    
    // Clean-up
    GL.glDeleteShader(vertex_shader);
    GL.glDeleteShader(fragment_shader);
    
    return program;
}

internal void
CalcBitmapSizeForText(const Asset_Font* font, float32 scale, String text, int32* out_width, int32* out_height)
{
    Trace("CalcBitmapSizeForText");
    
    int32 width = 0, line_width = 0, height = 0;
	int32 codepoint, it = 0;
	
	height += (int32)roundf((float32)(font->ascent - font->descent + font->line_gap) * scale);
	while (codepoint = String_Decode(text, &it), codepoint)
	{
		if (codepoint == '\n')
		{
			if (line_width > width)
				width = line_width;
			
			line_width = 0;
			height += (int32)roundf((float32)(font->ascent - font->descent + font->line_gap) * scale);
			continue;
		}
		
		int32 advance, bearing;
		stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &bearing);
		
		line_width += (int32)roundf((float32)advance * scale);
	}
    
    if (line_width > width)
		width = line_width;
	
    *out_width = width + 2;
    *out_height = height + 2;
}

internal void
GetCurrentShaderUniforms(uint32 shader)
{
    Trace("GetCurrentShaderUniforms");
    
    global_uniform_position = GL.glGetUniformLocation(shader, "uPosition");
    global_uniform_normal = GL.glGetUniformLocation(shader, "uNormal");
    global_uniform_albedo = GL.glGetUniformLocation(shader, "uAlbedo");
    global_uniform_depth = GL.glGetUniformLocation(shader, "uDepth");
    
    global_uniform_color = GL.glGetUniformLocation(shader, "uColor");
    global_uniform_view = GL.glGetUniformLocation(shader, "uView");
    global_uniform_texture = GL.glGetUniformLocation(shader, "uTexture");
    global_uniform_model = GL.glGetUniformLocation(shader, "uModel");
    global_uniform_viewpos = GL.glGetUniformLocation(shader, "uViewPos");
    
    global_uniform_dirlight_shadowmap = GL.glGetUniformLocation(shader, "uDirLight.shadowmap");
    global_uniform_dirlight_position = GL.glGetUniformLocation(shader, "uDirLight.position");
    global_uniform_dirlight_ambient = GL.glGetUniformLocation(shader, "uDirLight.ambient");
    global_uniform_dirlight_diffuse = GL.glGetUniformLocation(shader, "uDirLight.diffuse");
    global_uniform_dirlight_specular = GL.glGetUniformLocation(shader, "uDirLight.specular");
    global_uniform_dirlight_matrix = GL.glGetUniformLocation(shader, "uDirLightMatrix");
    
    global_uniform_material_ambient = GL.glGetUniformLocation(shader, "uMaterial.ambient");
    global_uniform_material_diffuse = GL.glGetUniformLocation(shader, "uMaterial.diffuse");
    global_uniform_material_specular = GL.glGetUniformLocation(shader, "uMaterial.specular");
    global_uniform_material_normal = GL.glGetUniformLocation(shader, "uMaterial.normal");
    global_uniform_material_shininess = GL.glGetUniformLocation(shader, "uMaterial.shininess");
    
    global_uniform_pointlights_count = GL.glGetUniformLocation(shader, "uPointLightsCount");
    for (int32 i = 0; i < MAX_POINT_LIGHTS_COUNT; ++i)
    {
        char buf[128];
        
        snprintf(buf, sizeof buf, "uPointLights[%i].constant", i);
        global_uniform_pointlights[i].constant = GL.glGetUniformLocation(shader, buf);
        snprintf(buf, sizeof buf, "uPointLights[%i].linear", i);
        global_uniform_pointlights[i].linear = GL.glGetUniformLocation(shader, buf);
        snprintf(buf, sizeof buf, "uPointLights[%i].quadratic", i);
        global_uniform_pointlights[i].quadratic = GL.glGetUniformLocation(shader, buf);
        
        snprintf(buf, sizeof buf, "uPointLights[%i].position", i);
        global_uniform_pointlights[i].position = GL.glGetUniformLocation(shader, buf);
        snprintf(buf, sizeof buf, "uPointLights[%i].ambient", i);
        global_uniform_pointlights[i].ambient = GL.glGetUniformLocation(shader, buf);
        snprintf(buf, sizeof buf, "uPointLights[%i].diffuse", i);
        global_uniform_pointlights[i].diffuse = GL.glGetUniformLocation(shader, buf);
        snprintf(buf, sizeof buf, "uPointLights[%i].specular", i);
        global_uniform_pointlights[i].specular = GL.glGetUniformLocation(shader, buf);
    }
}

//~ Temporary Internal API
internal void
Engine_InitRender(void)
{
    Trace("Render_Init");
    
    global_width = (float32)Platform_WindowWidth();
    global_height = (float32)Platform_WindowHeight();
    
#ifdef DEBUG
    GL.glEnable(GL_DEBUG_OUTPUT);
    GL.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    GL.glDebugMessageCallback(OpenGLDebugMessageCallback, NULL);
#endif
    
    GL.glEnable(GL_DEPTH_TEST);
    GL.glEnable(GL_BLEND);
    GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GL.glViewport(0, 0, (int32)global_width, (int32)global_height);
    
    uint32 vbo, vao, ebo;
    GL.glGenBuffers(1, &vbo);
    GL.glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GL.glBufferData(GL_ARRAY_BUFFER, sizeof global_quad_vertices, global_quad_vertices, GL_STATIC_DRAW);
    
    GL.glGenVertexArrays(1, &vao);
    GL.glBindVertexArray(vao);
    
    GL.glEnableVertexAttribArray(0);
    GL.glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, position));
    GL.glEnableVertexAttribArray(1);
    GL.glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    GL.glEnableVertexAttribArray(2);
    GL.glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
    
    GL.glGenBuffers(1, &ebo);
    GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    GL.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof global_quad_indices, global_quad_indices, GL_STATIC_DRAW);
    
    global_quad_vbo = vbo;
    global_quad_vao = vao;
    global_quad_ebo = ebo;
    
    global_default_shader = CompileShader(global_vertex_shader, global_fragment_shader);
    global_default_shadowshader = CompileShader(global_vertex_shadowshader, global_fragment_shadowshader);
    global_default_gbuffershader = CompileShader(global_vertex_gbuffershader, global_fragment_gbuffershader);
    global_default_finalpassshader = CompileShader(global_vertex_finalpassshader, global_fragment_finalpassshader);
    
    uint32 white_texture[] = {
        0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF,
    };
    
    GL.glGenTextures(1, &global_default_diffuse_texture);
    GL.glBindTexture(GL_TEXTURE_2D, global_default_diffuse_texture);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_texture);
    
    uint32 normal_texture[] = {
        0xFFFF8080, 0xFFFF8080,
        0xFFFF8080, 0xFFFF8080,
    };
    
    GL.glGenTextures(1, &global_default_normal_texture);
    GL.glBindTexture(GL_TEXTURE_2D, global_default_normal_texture);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, normal_texture);
    
    uint32 specular_texture[] = {
        0xFF808080, 0xFF808080,
        0xFF808080, 0xFF808080,
    };
    
    GL.glGenTextures(1, &global_default_specular_texture);
    GL.glBindTexture(GL_TEXTURE_2D, global_default_specular_texture);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, specular_texture);
    
    GL.glBindTexture(GL_TEXTURE_2D, 0);
}

internal void
Engine_DeinitRender(void)
{
    Trace("Render_Deinit");
    
    // TODO(ljre): deinit
}

//~ API
API void
Render_ClearBackground(float32 r, float32 g, float32 b, float32 a)
{
    GL.glClearColor(r, g, b, a);
    GL.glClear(GL_COLOR_BUFFER_BIT);
}

API void
Render_Begin2D(const Render_Camera* camera)
{
    mat4 proj;
    glm_ortho(0.0f, global_width, global_height, 0.0f, -1.0f, 1.0f, proj);
    
    glm_mat4_identity(global_view);
    if (camera)
    {
        glm_translate(global_view, (vec3) { camera->pos[0], camera->pos[1], 0.0f });
        glm_rotate(global_view, camera->angle, (vec3) { 0.0f, 0.0f, 1.0f });
        glm_scale(global_view, (vec3) { camera->scale[0], camera->scale[1], 0.0f });
    }
    
    glm_mat4_mul(proj, global_view, global_view);
    
    GL.glDisable(GL_DEPTH_TEST);
    GL.glDisable(GL_CULL_FACE);
    
    global_current_shader = global_default_shader;
    GetCurrentShaderUniforms(global_current_shader);
}

API void
Render_DrawRectangle(vec4 color, vec3 pos, vec3 size)
{
    Trace("Render_DrawRectangle");
    
    mat4 matrix = GLM_MAT4_IDENTITY_INIT;
    glm_translate(matrix, pos);
    glm_scale(matrix, size);
    glm_translate(matrix, (vec3) { -0.5f, -0.5f }); // NOTE(ljre): Center
    
    GL.glUseProgram(global_current_shader);
    GL.glUniform4fv(global_uniform_color, 1, color);
    GL.glUniformMatrix4fv(global_uniform_view, 1, false, (float32*)global_view);
    GL.glUniformMatrix4fv(global_uniform_model, 1, false, (float32*)matrix);
    GL.glUniform1i(global_uniform_texture, 0);
    
    GL.glActiveTexture(GL_TEXTURE0);
    GL.glBindTexture(GL_TEXTURE_2D, global_default_diffuse_texture);
    
    GL.glBindVertexArray(global_quad_vao);
    GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, global_quad_ebo);
    GL.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    GL.glBindTexture(GL_TEXTURE_2D, 0);
}

API bool32
Render_LoadFontFromFile(String path, Asset_Font* out_font)
{
    Trace("Render_LoadFontFromFile");
    
    bool32 result = false;
    out_font->data = Platform_ReadEntireFile(path, &out_font->data_size);
    
    if (out_font->data)
    {
        if (stbtt_InitFont(&out_font->info, out_font->data, 0))
        {
            stbtt_GetFontVMetrics(&out_font->info, &out_font->ascent, &out_font->descent, &out_font->line_gap);
            result = true;
        }
        else
        {
            Platform_FreeFileMemory(out_font->data, out_font->data_size);
        }
    }
    
    return result;
}

API void
Render_DrawText(const Asset_Font* font, String text, const vec3 pos, float32 char_height, const vec4 color)
{
    Trace("Render_DrawText");
    float32 scale = stbtt_ScaleForPixelHeight(&font->info, char_height);
    
    int32 bitmap_width, bitmap_height;
    CalcBitmapSizeForText(font, scale, text, &bitmap_width, &bitmap_height);
    
    uint8* bitmap = Engine_PushMemory((uintsize)(bitmap_width * bitmap_height));
    
    float32 xx = 0, yy = 0;
    int32 codepoint, it = 0;
    while (codepoint = String_Decode(text, &it), codepoint)
    {
        if (codepoint == '\n')
		{
			xx = 0;
			yy += (float32)(font->ascent - font->descent + font->line_gap) * scale;
			continue;
		}
		
		int32 advance, bearing;
		stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &bearing);
		
		int32 char_x1, char_y1;
		int32 char_x2, char_y2;
		stbtt_GetCodepointBitmapBox(&font->info, codepoint, scale, scale,
									&char_x1, &char_y1, &char_x2, &char_y2);
		
		int32 char_width = char_x2 - char_x1;
		int32 char_height = char_y2 - char_y1;
		
		int32 end_x = (int32)roundf(xx + (float32)bearing * scale) + char_x1;
		int32 end_y = (int32)roundf(yy + (float32)font->ascent * scale) + char_y1;
		
		int32 offset = end_x + (end_y * bitmap_width);
		stbtt_MakeCodepointBitmap(&font->info, bitmap + offset, char_width, char_height, bitmap_width, scale, scale, codepoint);
		
		xx += (float32)advance * scale;
    }
    
    //Platform_DebugDumpBitmap("test.ppm", bitmap, bitmap_width, bitmap_height, 1);
    
    mat4 matrix = GLM_MAT4_IDENTITY_INIT;
    glm_translate(matrix, (vec3) { pos[0], pos[1], pos[2] });
    glm_scale(matrix, (vec3) { (float32)bitmap_width, (float32)bitmap_height });
    
    GL.glActiveTexture(GL_TEXTURE0);
    
    uint32 texture;
    GL.glGenTextures(1, &texture);
    GL.glBindTexture(GL_TEXTURE_2D, texture);
	GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GL.glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, (int32[]) { GL_ONE, GL_ONE, GL_ONE, GL_RED });
    GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bitmap_width, bitmap_height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
    
    GL.glUseProgram(global_current_shader);
    GL.glUniform4fv(global_uniform_color, 1, color);
    GL.glUniformMatrix4fv(global_uniform_view, 1, false, (float32*)global_view);
    GL.glUniformMatrix4fv(global_uniform_model, 1, false, (float32*)matrix);
    GL.glUniform1i(global_uniform_texture, 0);
    
    GL.glBindVertexArray(global_quad_vao);
    GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, global_quad_ebo);
    GL.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    GL.glBindTexture(GL_TEXTURE_2D, 0);
    GL.glDeleteTextures(1, &texture);
    
    Engine_PopMemory(bitmap);
}

API bool32
Render_Load3DModelFromFile(String path, Asset_3DModel* out)
{
    Trace("Render_Load3DModelFromFile");
    uintsize size;
    uint8* data = Platform_ReadEntireFile(path, &size);
    if (!data)
        return false;
    
    void* state = Engine_PushMemoryState();
    
    Engine_GltfJson model;
    bool32 result;
    
    {
        Trace("Engine_ParseGltf");
        result = Engine_ParseGltf(data, size, &model);
    }
    
    // TODO(ljre)
    if (result)
    {
        GltfJson_Primitive* prim = &model.meshes[model.nodes[model.scenes[model.scene].nodes[0]].mesh].primitives[0];
        GltfJson_BufferView* view;
        int32 total_size = 0;
        
        const uint8* pos_buffer;
        int32 pos_size;
        
        const uint8* norm_buffer;
        int32 norm_size;
        
        const uint8* uv_buffer;
        int32 uv_size;
        
        const uint8* tan_buffer;
        int32 tan_size;
        
        const uint8* indices_buffer;
        int32 indices_size;
        
        // NOTE(ljre): Position
        view = &model.buffer_views[model.accessors[prim->attributes.position].buffer_view];
        pos_buffer = model.buffers[view->buffer].data + view->byte_offset;
        pos_size = view->byte_length;
        total_size += pos_size;
        
        // NOTE(ljre): Normal
        view = &model.buffer_views[model.accessors[prim->attributes.normal].buffer_view];
        norm_buffer = model.buffers[view->buffer].data + view->byte_offset;
        norm_size = view->byte_length;
        total_size += norm_size;
        
        // NOTE(ljre): Texcoords
        view = &model.buffer_views[model.accessors[prim->attributes.texcoord_0].buffer_view];
        uv_buffer = model.buffers[view->buffer].data + view->byte_offset;
        uv_size = view->byte_length;
        total_size += uv_size;
        
        // NOTE(ljre): Tangent
        view = &model.buffer_views[model.accessors[prim->attributes.tangent].buffer_view];
        tan_buffer = model.buffers[view->buffer].data + view->byte_offset;
        tan_size = view->byte_length;
        total_size += tan_size;
        
        GL.glGenBuffers(1, &out->vbo);
        GL.glBindBuffer(GL_ARRAY_BUFFER, out->vbo);
        GL.glBufferData(GL_ARRAY_BUFFER, total_size, NULL, GL_STATIC_DRAW);
        
        GL.glBufferSubData(GL_ARRAY_BUFFER, 0, pos_size, pos_buffer);
        GL.glBufferSubData(GL_ARRAY_BUFFER, pos_size, norm_size, norm_buffer);
        GL.glBufferSubData(GL_ARRAY_BUFFER, pos_size + norm_size, uv_size, uv_buffer);
        GL.glBufferSubData(GL_ARRAY_BUFFER, pos_size + norm_size + uv_size, tan_size, tan_buffer);
        
        // NOTE(ljre): VAO
        GL.glGenVertexArrays(1, &out->vao);
        GL.glBindVertexArray(out->vao);
        
        GL.glEnableVertexAttribArray(0);
        GL.glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(vec3), 0);
        GL.glEnableVertexAttribArray(1);
        GL.glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(vec3), (void*)pos_size);
        GL.glEnableVertexAttribArray(2);
        GL.glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(vec2), (void*)(pos_size + norm_size));
        GL.glEnableVertexAttribArray(3);
        GL.glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(vec4), (void*)(pos_size + norm_size + uv_size));
        
        // NOTE(ljre): Indices
        view = &model.buffer_views[model.accessors[prim->indices].buffer_view];
        indices_buffer = model.buffers[view->buffer].data + view->byte_offset;
        indices_size = view->byte_length;
        
        GL.glGenBuffers(1, &out->ebo);
        GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out->ebo);
        GL.glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_buffer, GL_STATIC_DRAW);
        
        out->prim_mode = (uint32)prim->mode;
        out->index_type = (uint32)model.accessors[prim->indices].component_type;
        out->index_count = model.accessors[prim->indices].count;
        glm_mat4_copy(model.nodes[model.scenes[model.scene].nodes[0]].transform, out->transform);
        out->material.ambient[0] = 1.0f;
        out->material.ambient[1] = 1.0f;
        out->material.ambient[2] = 1.0f;
        out->material.diffuse = 0;
        out->material.specular = 0;
        out->material.normal = 0;
        out->material.shininess = 32.0f;
        
        if (prim->material != -1)
        {
            GltfJson_Material* mat = &model.materials[prim->material];
            
            out->material.base_color[0] = mat->pbr_metallic_roughness.base_color_factor[0];
            out->material.base_color[1] = mat->pbr_metallic_roughness.base_color_factor[1];
            out->material.base_color[2] = mat->pbr_metallic_roughness.base_color_factor[2];
            out->material.base_color[3] = mat->pbr_metallic_roughness.base_color_factor[3];
            
            if (mat->pbr_metallic_roughness.base_color_texture.specified)
            {
                GltfJson_Texture* tex = &model.textures[mat->pbr_metallic_roughness.base_color_texture.index];
                GltfJson_Sampler* sampler = &model.samplers[tex->sampler];
                GltfJson_Image* image = &model.images[tex->source];
                GltfJson_BufferView* view = &model.buffer_views[image->buffer_view];
                bool32 needs_mipmaps = false;
                
                int32 width, height, channels;
                uint8* data = stbi_load_from_memory(model.buffers[view->buffer].data + view->byte_offset,
                                                    view->byte_length,
                                                    &width, &height, &channels, 3);
                
                GL.glGenTextures(1, &out->material.diffuse);
                GL.glBindTexture(GL_TEXTURE_2D, out->material.diffuse);
                
                if (sampler->mag_filter != -1)
                {
                    needs_mipmaps = needs_mipmaps || (sampler->mag_filter != GL_NEAREST &&
                                                      sampler->mag_filter != GL_LINEAR);
                    GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler->mag_filter);
                }
                else
                {
                    GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                }
                
                if (sampler->min_filter != -1)
                {
                    needs_mipmaps = needs_mipmaps || (sampler->min_filter != GL_NEAREST &&
                                                      sampler->min_filter != GL_LINEAR);
                    GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler->min_filter);
                }
                else
                {
                    GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                }
                
                GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler->wrap_s);
                GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler->wrap_t);
                
                GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                if (needs_mipmaps)
                    GL.glGenerateMipmap(GL_TEXTURE_2D);
                GL.glBindTexture(GL_TEXTURE_2D, 0);
            }
            
            if (mat->normal_texture.specified)
            {
                GltfJson_Texture* tex = &model.textures[mat->normal_texture.index];
                GltfJson_Sampler* sampler = &model.samplers[tex->sampler];
                GltfJson_Image* image = &model.images[tex->source];
                GltfJson_BufferView* view = &model.buffer_views[image->buffer_view];
                bool32 needs_mipmaps = false;
                
                int32 width, height, channels;
                uint8* data = stbi_load_from_memory(model.buffers[view->buffer].data + view->byte_offset,
                                                    view->byte_length,
                                                    &width, &height, &channels, 3);
                
                GL.glGenTextures(1, &out->material.normal);
                GL.glBindTexture(GL_TEXTURE_2D, out->material.normal);
                
                if (sampler->mag_filter != -1)
                {
                    needs_mipmaps = needs_mipmaps || (sampler->mag_filter != GL_NEAREST &&
                                                      sampler->mag_filter != GL_LINEAR);
                    GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler->mag_filter);
                }
                else
                {
                    GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                }
                
                if (sampler->min_filter != -1)
                {
                    needs_mipmaps = needs_mipmaps || (sampler->min_filter != GL_NEAREST &&
                                                      sampler->min_filter != GL_LINEAR);
                    GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler->min_filter);
                }
                else
                {
                    GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                }
                
                GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler->wrap_s);
                GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler->wrap_t);
                
                GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                if (needs_mipmaps)
                    GL.glGenerateMipmap(GL_TEXTURE_2D);
                GL.glBindTexture(GL_TEXTURE_2D, 0);
            }
            
        }
        
        GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
        GL.glBindVertexArray(0);
        GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
    Engine_PopMemoryState(state);
    Platform_FreeFileMemory(data, size);
    
    return result;
}

API Render_Entity*
Render_AddToManager(Render_Manager* mgr, Render_EntityKind kind)
{
    Trace("Render_AddToManager");
    Render_Entity* result;
    
    //- NOTE(ljre): Get Component Handle
    int32* comp_count;
    void* comp_ptr;
    uintsize comp_size;
    Render_Entity** comp_handle_ptr;
    
    if (kind == Render_EntityKind_PointLight &&
        mgr->point_lights_count < ArrayLength(mgr->point_lights))
    {
        comp_count = &mgr->point_lights_count;
        comp_ptr = &mgr->point_lights[mgr->point_lights_count];
        comp_handle_ptr = &mgr->point_lights[mgr->point_lights_count].entity;
        comp_size = sizeof(*mgr->point_lights);
    }
    else if (kind == Render_EntityKind_3DModel &&
             mgr->model_count < ArrayLength(mgr->models))
    {
        comp_count = &mgr->model_count;
        comp_ptr = &mgr->models[mgr->model_count];
        comp_handle_ptr = &mgr->models[mgr->model_count].entity;
        comp_size = sizeof(*mgr->models);
    }
    else
    {
        return NULL;
    }
    
    //- NOTE(ljre): Get Entity Handle
    if (mgr->free_space_count > 0)
    {
        result = mgr->free_spaces[--mgr->free_space_count];
    }
    else if (mgr->entity_count < ArrayLength(mgr->entities))
    {
        result = &mgr->entities[mgr->entity_count++];
    }
    else
    {
        return NULL;
    }
    
    //- Done
    memset(result, 0, sizeof *result);
    memset(comp_ptr, 0, comp_size);
    
    result->handle = comp_ptr;
    result->kind = kind;
    ++*comp_count;
    *comp_handle_ptr = result;
    
    result->scale[0] = 1.0f;
    result->scale[1] = 1.0f;
    result->scale[2] = 1.0f;
    
    return result;
}

API void
Render_RemoveFromManager(Render_Manager* mgr, Render_Entity* handle)
{
    Trace("Render_RemoveFromManager");
    Assert(handle);
    
    switch (handle->kind)
    {
        case Render_EntityKind_PointLight:
        {
            if (mgr->point_lights_count > 1)
            {
                *handle->point_light = mgr->point_lights[mgr->point_lights_count-1];
                
                handle->point_light->entity->point_light = handle->point_light;
            }
            --mgr->point_lights_count;
        } break;
        
        case Render_EntityKind_3DModel:
        {
            if (mgr->model_count > 1)
            {
                *handle->model = mgr->models[mgr->model_count-1];
                handle->model->entity->model = handle->model;
            }
            --mgr->model_count;
        } break;
    }
    
    mgr->free_spaces[mgr->free_space_count++] = handle;
}

API void
Render_ProperlySetupManager(Render_Manager* mgr)
{
    Trace("Render_ProperlySetupManager");
    const uint32 depthmap_width = 2048, depthmap_height = 2048;
    
    if (!mgr->shadow_fbo)
    {
        GL.glGenTextures(1, &mgr->shadow_depthmap);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->shadow_depthmap);
        GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, depthmap_width, depthmap_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GL.glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (vec4) { 1.0f, 1.0f, 1.0f, 1.0f });
        
        GL.glGenFramebuffers(1, &mgr->shadow_fbo);
        GL.glBindFramebuffer(GL_FRAMEBUFFER, mgr->shadow_fbo);
        GL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mgr->shadow_depthmap, 0);
        GL.glDrawBuffer(GL_NONE);
        GL.glReadBuffer(GL_NONE);
    }
    
    if (!mgr->gbuffer)
    {
        int32 width = (int32)global_width;
        int32 height = (int32)global_height;
        
        GL.glGenFramebuffers(1, &mgr->gbuffer);
        GL.glBindFramebuffer(GL_FRAMEBUFFER, mgr->gbuffer);
        
        GL.glGenTextures(1, &mgr->gbuffer_pos);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->gbuffer_pos);
        GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mgr->gbuffer_pos, 0);
        
        GL.glGenTextures(1, &mgr->gbuffer_norm);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->gbuffer_norm);
        GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mgr->gbuffer_norm, 0);
        
        GL.glGenTextures(1, &mgr->gbuffer_albedo);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->gbuffer_albedo);
        GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, mgr->gbuffer_albedo, 0);
        
        uint32 buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        GL.glDrawBuffers(ArrayLength(buffers), buffers);
        
#if 0
        GL.glGenRenderbuffers(1, &mgr->gbuffer_depth);
        GL.glBindRenderbuffer(GL_RENDERBUFFER, mgr->gbuffer_depth);
        GL.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        GL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mgr->gbuffer_depth);
#else
        GL.glGenTextures(1, &mgr->gbuffer_depth);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->gbuffer_depth);
        GL.glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mgr->gbuffer_depth, 0);
#endif
    }
}

API void
Render_DrawManager(Render_Manager* mgr, const Render_Camera* camera)
{
    Trace("Render_DrawManager");
    const uint32 depthmap_width = 2048, depthmap_height = 2048;
    mat4 view;
    mat4 matrix;
    mat4 light_space_matrix;
    
    vec3 viewpos;
    
    //~ NOTE(ljre): Setup
    {
        mat4 proj;
        glm_perspective(glm_rad(90.0f), global_width / global_height, 0.01f, 100.0f, proj);
        
        glm_mat4_identity(view);
        if (camera)
        {
            glm_look((float32*)camera->pos, (float32*)camera->dir, (float32*)camera->up, view);
        }
        
        glm_mat4_mul(proj, view, view);
        glm_vec3_copy((float32*)camera->pos, viewpos);
    }
    
    GL.glEnable(GL_DEPTH_TEST);
    GL.glEnable(GL_CULL_FACE);
    GL.glClear(GL_DEPTH_BUFFER_BIT);
    
    if (!mgr->shadow_fbo || !mgr->gbuffer)
        Render_ProperlySetupManager(mgr);
    
    //~ NOTE(ljre): Draw to Framebuffer
    {
        {
            float32 umbrella = 20.0f;
            float32 range = 30.0f;
            mat4 proj;
            glm_ortho(-range, range, -range, range, 0.5f, 80.0f, proj);
            
            glm_lookat((vec3) { mgr->dirlight[0] * umbrella,
                           mgr->dirlight[1] * umbrella,
                           mgr->dirlight[2] * umbrella },
                       (vec3) { 0.0f, 0.0f, 0.0f },
                       (vec3) { 0.0f, 1.0f, 0.0f }, light_space_matrix);
            glm_mat4_mul(proj, light_space_matrix, light_space_matrix);
        }
        
        GL.glBindFramebuffer(GL_FRAMEBUFFER, mgr->shadow_fbo);
        GL.glViewport(0, 0, depthmap_width, depthmap_height);
        GL.glClear(GL_DEPTH_BUFFER_BIT);
        GL.glUseProgram(global_default_shadowshader);
        //GL.glCullFace(GL_FRONT);
        
        GetCurrentShaderUniforms(global_default_shadowshader);
        
        GL.glUniformMatrix4fv(global_uniform_view, 1, false, (float32*)light_space_matrix);
        
        for (int32 i = 0; i < mgr->model_count; ++i)
        {
            Render_3DModel* const model = &mgr->models[i];
            Asset_3DModel* const asset = model->asset;
            Render_Entity* const entity = model->entity;
            
            glm_mat4_identity(matrix);
            glm_translate(matrix, entity->position);
            glm_rotate(matrix, entity->rotation[0], (vec3) { 1.0f, 0.0f, 0.0f });
            glm_rotate(matrix, entity->rotation[1], (vec3) { 0.0f, 1.0f, 0.0f });
            glm_rotate(matrix, entity->rotation[2], (vec3) { 0.0f, 0.0f, 1.0f });
            glm_scale(matrix, entity->scale);
            
            glm_mat4_mul(matrix, asset->transform, matrix);
            
            GL.glBindVertexArray(asset->vao);
            GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asset->ebo);
            
            GL.glUniformMatrix4fv(global_uniform_model, 1, false, (float32*)matrix);
            
            GL.glDrawElements(asset->prim_mode, asset->index_count, asset->index_type, 0);
        }
    }
    
    GL.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL.glViewport(0, 0, (int32)global_width, (int32)global_height);
    GL.glCullFace(GL_BACK);
    
    //~ NOTE(ljre): GBuffer
    {
        GL.glBindFramebuffer(GL_FRAMEBUFFER, mgr->gbuffer);
        GL.glDisable(GL_BLEND);
        
        GL.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        GL.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        GL.glUseProgram(global_default_gbuffershader);
        GetCurrentShaderUniforms(global_default_gbuffershader);
        
        GL.glUniformMatrix4fv(global_uniform_view, 1, false, (float32*)view);
        
        GL.glUniform1i(global_uniform_material_diffuse, TEXTURE_SLOT_DIFFUSE);
        GL.glUniform1i(global_uniform_material_specular, TEXTURE_SLOT_SPECULAR);
        GL.glUniform1i(global_uniform_material_normal, TEXTURE_SLOT_NORMAL);
        
        for (int32 i = 0; i < mgr->model_count; ++i)
        {
            Render_3DModel* const model = &mgr->models[i];
            Asset_3DModel* const asset = model->asset;
            Render_Entity* const entity = model->entity;
            
            glm_mat4_identity(matrix);
            glm_translate(matrix, entity->position);
            glm_rotate(matrix, entity->rotation[0], (vec3) { 1.0f, 0.0f, 0.0f });
            glm_rotate(matrix, entity->rotation[1], (vec3) { 0.0f, 1.0f, 0.0f });
            glm_rotate(matrix, entity->rotation[2], (vec3) { 0.0f, 0.0f, 1.0f });
            glm_scale(matrix, entity->scale);
            
            glm_mat4_mul(matrix, asset->transform, matrix);
            
            GL.glBindVertexArray(asset->vao);
            GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asset->ebo);
            
            GL.glUniformMatrix4fv(global_uniform_model, 1, false, (float32*)matrix);
            GL.glUniform4fv(global_uniform_color, 1, model->color);
            GL.glUniform3fv(global_uniform_material_ambient, 1, asset->material.ambient);
            GL.glUniform1f(global_uniform_material_shininess, asset->material.shininess);
            
            GL.glActiveTexture(GL_TEXTURE0 + TEXTURE_SLOT_DIFFUSE);
            GL.glBindTexture(GL_TEXTURE_2D, asset->material.diffuse ? asset->material.diffuse : global_default_diffuse_texture);
            
            GL.glActiveTexture(GL_TEXTURE0 + TEXTURE_SLOT_SPECULAR);
            GL.glBindTexture(GL_TEXTURE_2D, asset->material.specular ? asset->material.specular : global_default_specular_texture);
            
            GL.glActiveTexture(GL_TEXTURE0 + TEXTURE_SLOT_NORMAL);
            GL.glBindTexture(GL_TEXTURE_2D, asset->material.normal ? asset->material.normal : global_default_normal_texture);
            
            GL.glDrawElements(asset->prim_mode, asset->index_count, asset->index_type, 0);
        }
        
        // NOTE(ljre): Copy depth buffer
        //GL.glBindFramebuffer(GL_READ_FRAMEBUFFER, mgr->gbuffer);
        //GL.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        //GL.glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        GL.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        GL.glEnable(GL_BLEND);
    }
    
    //- NOTE(ljre): Render GBuffer
    {
        //GL.glDepthFunc(GL_ALWAYS);
        GL.glUseProgram(global_default_finalpassshader);
        GetCurrentShaderUniforms(global_default_finalpassshader);
        
        mat4 model;
        glm_ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, model);
        
        GL.glActiveTexture(GL_TEXTURE0 + 0);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->gbuffer_pos);
        GL.glActiveTexture(GL_TEXTURE0 + 1);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->gbuffer_norm);
        GL.glActiveTexture(GL_TEXTURE0 + 2);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->gbuffer_albedo);
        GL.glActiveTexture(GL_TEXTURE0 + 3);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->shadow_depthmap);
        GL.glActiveTexture(GL_TEXTURE0 + 4);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->gbuffer_depth);
        
        GL.glUniform1i(global_uniform_position, 0);
        GL.glUniform1i(global_uniform_normal, 1);
        GL.glUniform1i(global_uniform_albedo, 2);
        GL.glUniform1i(global_uniform_dirlight_shadowmap, 3);
        GL.glUniform1i(global_uniform_depth, 4);
        
        GL.glUniformMatrix4fv(global_uniform_dirlight_matrix, 1, false, (float32*)light_space_matrix);
        GL.glUniformMatrix4fv(global_uniform_model, 1, false, (float32*)model);
        GL.glUniform3fv(global_uniform_viewpos, 1, viewpos);
        GL.glUniform3fv(global_uniform_dirlight_position, 1, mgr->dirlight);
        GL.glUniform3f(global_uniform_dirlight_ambient, 0.2f, 0.2f, 0.2f);
        GL.glUniform3f(global_uniform_dirlight_diffuse, 0.4f, 0.4f, 0.4f);
        GL.glUniform3f(global_uniform_dirlight_specular, 0.0f, 0.0f, 0.0f);
        
        GL.glUniform1i(global_uniform_pointlights_count, mgr->point_lights_count);
        for (int32 i = 0; i < mgr->point_lights_count; ++i)
        {
            Render_PointLight* const l = &mgr->point_lights[i];
            Render_Entity* const e = l->entity;
            
            GL.glUniform3fv(global_uniform_pointlights[i].position, 1, e->position);
            GL.glUniform1f(global_uniform_pointlights[i].constant, l->constant);
            GL.glUniform1f(global_uniform_pointlights[i].linear, l->linear);
            GL.glUniform1f(global_uniform_pointlights[i].quadratic, l->quadratic);
            GL.glUniform3fv(global_uniform_pointlights[i].ambient, 1, l->ambient);
            GL.glUniform3fv(global_uniform_pointlights[i].diffuse, 1, l->diffuse);
            GL.glUniform3fv(global_uniform_pointlights[i].specular, 1, l->specular);
        }
        
        GL.glBindVertexArray(global_quad_vao);
        GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, global_quad_ebo);
        GL.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        //GL.glDepthFunc(GL_LESS);
    }
    
    //~ NOTE(ljre): Point Lights
    {
        global_current_shader = global_default_shader;
        GL.glUseProgram(global_current_shader);
        GetCurrentShaderUniforms(global_current_shader);
        
        GL.glUniformMatrix4fv(global_uniform_view, 1, false, (float32*)view);
        GL.glUniform1i(global_uniform_texture, 0);
        
        for (int32 i = 0; i < mgr->point_lights_count; ++i)
        {
            Render_PointLight* const light = &mgr->point_lights[i];
            Render_Entity* const entity = light->entity;
            Asset_3DModel* const asset = mgr->cube_model;
            
            glm_mat4_identity(matrix);
            glm_translate(matrix, entity->position);
            glm_rotate(matrix, entity->rotation[0], (vec3) { 1.0f, 0.0f, 0.0f });
            glm_rotate(matrix, entity->rotation[1], (vec3) { 0.0f, 1.0f, 0.0f });
            glm_rotate(matrix, entity->rotation[2], (vec3) { 0.0f, 0.0f, 1.0f });
            glm_scale(matrix, entity->scale);
            
            GL.glUniform4f(global_uniform_color, light->diffuse[0], light->diffuse[1], light->diffuse[2], 1.0f);
            GL.glUniformMatrix4fv(global_uniform_model, 1, false, (float32*)matrix);
            
            GL.glActiveTexture(GL_TEXTURE0);
            GL.glBindTexture(GL_TEXTURE_2D, global_default_diffuse_texture);
            
            GL.glBindVertexArray(asset->vao);
            GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asset->ebo);
            
            GL.glDrawElements(asset->prim_mode, asset->index_count, asset->index_type, 0);
        }
    }
    
    GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL.glBindVertexArray(0);
    
    if (false) {
        mat4 view, matrix;
        glm_ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, view);
        glm_mat4_identity(matrix);
        
        GL.glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        GL.glUseProgram(global_default_shader);
        GetCurrentShaderUniforms(global_default_shader);
        
        GL.glUniform4fv(global_uniform_color, 1, (vec4) { 1.0f, 1.0f, 1.0f, 1.0f });
        GL.glUniformMatrix4fv(global_uniform_view, 1, false, (float32*)view);
        GL.glUniformMatrix4fv(global_uniform_model, 1, false, (float32*)matrix);
        GL.glUniform1i(global_uniform_texture, 0);
        
        GL.glActiveTexture(GL_TEXTURE0);
        GL.glBindTexture(GL_TEXTURE_2D, mgr->gbuffer_norm);
        
        GL.glBindVertexArray(global_quad_vao);
        GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, global_quad_ebo);
        GL.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        GL.glBindTexture(GL_TEXTURE_2D, 0);
    }
}
