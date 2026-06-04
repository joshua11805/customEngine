#include "pch.h"
#include "Shader.h"
#include "Profiler.h"
#include "Renderer.h"
#include "Texture.h"
#include <SDL3_shadercross/SDL_shadercross.h>


/// The constructor will load the shader, but will NOT create the pipeline
/// Call CreatePipeline ONCE before you try to render with this shader
/// @param renderer
/// @param filename the name of the file to load ("Shaders/Simple.hlsl")
Shader::Shader(Renderer* renderer, const char* filename)
    : m_renderer(renderer)
{
    bool loaded = Load(filename);
    DbgAssert(loaded, "Shader failed to load");
}

Shader::~Shader()
{
    SDL_GPUDevice* device = m_renderer->GetDevice();
    if (m_pipeline)
        SDL_ReleaseGPUGraphicsPipeline(device, m_pipeline);
    SDL_ReleaseGPUShader(device, m_vertexShader);
    SDL_ReleaseGPUShader(device, m_fragmentShader);
}

void Shader::SetBlend(bool enable, SDL_GPUBlendFactor sourceFactor, SDL_GPUBlendFactor distanceFactor)
{
    m_blendEnable = enable;
    m_srcBlendFactor = sourceFactor;
    m_dstBlendFactor = distanceFactor;
}


/// Shaders default to using the "BackBuffer"
/// If you leave the colorTarget as nullptr (the default), the pipeline will be set up with the backbuffer
/// @param colorTarget give a texture intended to be used as the render target (nullptr to use the backbuffer)
void Shader::SetColorTarget(Texture* colorTarget)
{
    m_colorTarget = colorTarget;
}

/// Before rendering with a Shader, you must create the pipeline
/// Call this function ONCE at set-up
/// Call this function AFTER you've set any optional settings, but before you call SetActive
/// @param vertexAttributes provide an array of SDL_GPUVertexAttribute
/// @param numVertexAttributes specify how many SDL_GPUVertexAttribute are in the array
/// @param vertexStride the size in bytes of a single vertex
/// @return true on success or false on failure
bool Shader::CreatePipeline(const SDL_GPUVertexAttribute* vertexAttributes, uint32_t numVertexAttributes, uint32_t vertexStride)
{
    PROFILE_SCOPE(CreatePipeline);

    DbgAssert(nullptr == m_pipeline, "You can only CreatePipeline once per shader");

    // set up the vertex input layout
    SDL_GPUVertexBufferDescription vertexBufferDescription = { };
    vertexBufferDescription.slot = 0;
    vertexBufferDescription.pitch = vertexStride;
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    SDL_GPUVertexInputState vertexInputState = { };
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = numVertexAttributes;
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
    vertexInputState.num_vertex_buffers = 1;

    // set the color target description
    SDL_GPUColorTargetDescription colorTargetDesc = { };
    if (m_colorTarget)
    {
        colorTargetDesc.format = m_colorTarget->GetFormat();
    }
    else
    {   // if no color taret was specified, default to the backbuffer
        colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(m_renderer->GetDevice(), m_renderer->GetWindow());
    }
    SDL_GPUColorTargetBlendState blendState = { };
    blendState.enable_blend          = m_blendEnable;
    blendState.src_color_blendfactor = m_srcBlendFactor;
    blendState.dst_color_blendfactor = m_dstBlendFactor;
    blendState.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    blendState.src_alpha_blendfactor = m_srcBlendFactor;  //same as color
    blendState.dst_alpha_blendfactor = m_dstBlendFactor;
    blendState.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    blendState.enable_color_write_mask = false;
    colorTargetDesc.blend_state = blendState;

    // set up the pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = { };
    pipelineInfo.vertex_shader = m_vertexShader;
    pipelineInfo.fragment_shader = m_fragmentShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
pipelineInfo.depth_stencil_state.compare_op = m_zTest ? SDL_GPU_COMPAREOP_LESS : SDL_GPU_COMPAREOP_ALWAYS;
    pipelineInfo.depth_stencil_state.write_mask = 0xFF;
    pipelineInfo.depth_stencil_state.enable_stencil_test = false;
    pipelineInfo.depth_stencil_state.enable_depth_write = m_zWrite;
    pipelineInfo.depth_stencil_state.enable_depth_test = m_zTest;
    pipelineInfo.target_info.depth_stencil_format = m_renderer->GetTextureFormat();
    pipelineInfo.target_info.has_depth_stencil_target = (m_zWrite || m_zTest);
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
    pipelineInfo.target_info.num_color_targets = 1;



    m_pipeline = SDL_CreateGPUGraphicsPipeline(m_renderer->GetDevice(), &pipelineInfo);
    if (nullptr == m_pipeline) {
        SDL_Log("Failed to create graphics pipeline: %s", SDL_GetError());
        return false;
    }

    return true;
}

/// Set this shader to be active on the renderPass
/// Any draw call following this will use this shader
/// @param renderPass
void Shader::SetActive(SDL_GPURenderPass* renderPass)
{
    DbgAssert(m_pipeline, "You must call CreatePipeline before you can call SetActive");
    SDL_BindGPUGraphicsPipeline(renderPass, m_pipeline);
}

/// Attempt to load this shader from a file
/// This will not automatically create the pipeline... this is to let you set options before the pipeline is fixed
/// @param filename the name of the shader to load ("Shaders/Simple.hlsl")
/// @return true on success or false on failure
bool Shader::Load(const char* filename)
{
    PROFILE_SCOPE(ShaderLoad);

    std::ifstream file(filename);
    if (false == file.is_open())
    {
        SDL_Log("Unable to open shader file \"%s\"\n", filename);
        return false;
    }
    std::stringstream textStream;
    textStream << file.rdbuf();
    std::string hlslSource = textStream.str();

    {   // load vertex shader
        SDL_ShaderCross_HLSL_Info vertexInfo = { };
        vertexInfo.source = hlslSource.c_str();
        vertexInfo.entrypoint = "VS";
        vertexInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
        vertexInfo.enable_debug = true;
        vertexInfo.name = filename;

        // load the SPIRV byte code
        void* vertexByteCode = nullptr;
        size_t vertexByteCodeSize = 0;
        {
            PROFILE_SCOPE(SPIRV_VS);
            vertexByteCode = SDL_ShaderCross_CompileSPIRVFromHLSL(&vertexInfo, &vertexByteCodeSize);
            if (!vertexByteCode) {
                SDL_Log("Failed to compile vertex shader from HLSL: %s", SDL_GetError());
                return false;
            }
        }

        SDL_ShaderCross_GraphicsShaderMetadata* meta;
        {   // read the meta data
            PROFILE_SCOPE(Reflect_VS);
            meta = SDL_ShaderCross_ReflectGraphicsSPIRV(
                (Uint8*)vertexByteCode,
                vertexByteCodeSize,
                0
            );
        }

        {   // create the shader
            PROFILE_SCOPE(Compile_VS);
            SDL_ShaderCross_SPIRV_Info info = { };
            info.bytecode = (Uint8*)vertexByteCode;
            info.bytecode_size = vertexByteCodeSize;
            info.entrypoint = "VS";
            info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
            info.enable_debug = true;
            m_vertexShader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
                m_renderer->GetDevice(),
                &info,
                meta,
                0);
        }

        SDL_free(vertexByteCode);
        if (!m_vertexShader) {
            SDL_Log("Failed to create vertex shader: %s", SDL_GetError());
            return false;
        }
    }

    {   // load fragment shader
        SDL_ShaderCross_HLSL_Info fragInfo = { };
        fragInfo.source = hlslSource.c_str();
        fragInfo.entrypoint = "PS";
        fragInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
        fragInfo.enable_debug = true;
        fragInfo.name = filename;

        // load the SPIRV byte code
        void* fragmentByteCode = nullptr;
        size_t fragmentByteCodeSize = 0;
        {
            PROFILE_SCOPE(SPIRV_PS);
            fragmentByteCode = SDL_ShaderCross_CompileSPIRVFromHLSL(&fragInfo, &fragmentByteCodeSize);
            if (!fragmentByteCode) {
                SDL_Log("Failed to compile pixel shader from HLSL: %s", SDL_GetError());
                return false;
            }
        }

        SDL_ShaderCross_GraphicsShaderMetadata* meta;
        {   // read the meta data
            PROFILE_SCOPE(Reflect_PS);
            meta = SDL_ShaderCross_ReflectGraphicsSPIRV(
                (Uint8*)fragmentByteCode,
                fragmentByteCodeSize,
                0
            );
        }

        {   // create the shader
            PROFILE_SCOPE(Compile_PS);
            SDL_ShaderCross_SPIRV_Info info = { };
            info.bytecode = (Uint8*)fragmentByteCode;
            info.bytecode_size = fragmentByteCodeSize;
            info.entrypoint = "PS";
            info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
            info.enable_debug = true;
            m_fragmentShader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
                m_renderer->GetDevice(),
                &info,
                meta,
                0);
        }

        SDL_free(fragmentByteCode);
        if (!m_fragmentShader) {
            SDL_Log("Failed to create fragment shader: %s", SDL_GetError());
            return false;
        }
    }

    return true;
}
