
#include "Tutorial21_RayTracing.hpp"
#include "MapHelper.hpp"
#include "GraphicsTypesX.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ShaderMacroHelper.hpp"
#include "imgui.h"
#include "ImGuiUtils.hpp"
#include "AdvancedMath.hpp"
#include "PlatformMisc.hpp"
#include "GLTFLoader.hpp"
#include <random>

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial21_RayTracing();
}

void Tutorial21_RayTracing::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    CreateGraphicsPSO();
    CreateRayTracingPSO();
    LoadTextures();

    // === Cargar modelo GLTF (CesiumMan) ===

    GLTF::ModelCreateInfo ModelCI;
    ModelCI.FileName             = "models/CesiumMan/glTF/CesiumMan.gltf";
    ModelCI.ComputeBoundingBoxes = true;

    m_pGLTFModel = std::make_unique<GLTF::Model>(m_pDevice, m_pImmediateContext, ModelCI);

    // Crea buffers para ray tracing desde el primer mesh
    const auto& mesh = m_pGLTFModel->Meshes[0];
    auto* pVB        = mesh.pVertexBuffer.RawPtr();
    auto* pIB        = mesh.pIndexBuffer.RawPtr();

    BottomLevelASDesc BLASDesc;
    BLASDesc.Name     = "CesiumMan BLAS";
    BLASDesc.Flags    = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;

    BLASTriangleDesc& TriDesc = BLASDesc.Triangles.emplace_back();
    TriDesc.GeometryName         = "CesiumManGeometry";
    TriDesc.MaxVertexCount       = mesh.VertexCount;
    TriDesc.VertexValueType      = VT_FLOAT32;
    TriDesc.VertexComponentCount = 3; // XYZ
    TriDesc.MaxPrimitiveCount    = mesh.IndexCount / 3;
    TriDesc.IndexType            = VT_UINT32;

    m_pDevice->CreateBLAS(BLASDesc, &m_pCesiumBLAS);
    VERIFY_EXPR(m_pCesiumBLAS);

    // Scratch buffer
    RefCntAutoPtr<IBuffer> pScratchBuffer;
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "BLAS Scratch Buffer";
        BuffDesc.Usage     = USAGE_DEFAULT;
        BuffDesc.BindFlags = BIND_RAY_TRACING;
        BuffDesc.Size      = m_pCesiumBLAS->GetScratchBufferSizes().Build;

        m_pDevice->CreateBuffer(BuffDesc, nullptr, &pScratchBuffer);
    }

    // Build BLAS
    BLASBuildTriangleData BuildData;
    BuildData.GeometryName         = TriDesc.GeometryName;
    BuildData.pVertexBuffer        = pVB;
    BuildData.VertexStride         = mesh.VertexStride;
    BuildData.VertexCount          = mesh.VertexCount;
    BuildData.VertexValueType      = VT_FLOAT32;
    BuildData.VertexComponentCount = 3;
    BuildData.pIndexBuffer         = pIB;
    BuildData.PrimitiveCount       = mesh.IndexCount / 3;
    BuildData.IndexType            = VT_UINT32;

    BuildBLASAttribs BuildAttribs;
    BuildAttribs.pBLAS             = m_pCesiumBLAS;
    BuildAttribs.pTriangleData     = &BuildData;
    BuildAttribs.TriangleDataCount = 1;
    BuildAttribs.pScratchBuffer    = pScratchBuffer;
    BuildAttribs.BLASTransitionMode          = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.GeometryTransitionMode      = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    m_pImmediateContext->BuildBLAS(BuildAttribs);
}

void Tutorial21_RayTracing::UpdateTLAS()
{
    // Parte original omitida por brevedad...

    // Agregar instancia del modelo CesiumMan
    TLASBuildInstanceData& inst = Instances[4];
    inst.InstanceName = "CesiumMan";
    inst.pBLAS        = m_pCesiumBLAS;
    inst.Mask         = OPAQUE_GEOM_MASK;
    inst.Transform.SetTranslation(0.0f, -4.5f, 0.0f);

    // Asegúrate que MaxTotal sea al menos 5 + MaxSmallSpheres
}

}
