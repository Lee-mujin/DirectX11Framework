#include "stdafx.h"
#include "Graphics.h"
#include "../Engine/SpriteRenderer.h"
#include "../Engine/GameObject.h"
#include "../Graphics/ShaderManager.h"
#include <cassert>

Graphics::Graphics()
    : m_isWireframe(false)
{
}

Graphics::~Graphics()
{
    // ComPtr will auto-release
}

bool Graphics::Initialize(HWND hWnd, int width, int height)
{
    // --- Basic D3D11 setup ---
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, nullptr, 0, D3D11_SDK_VERSION, &scd, &m_swapChain, &m_device, nullptr, &m_context);
    if (FAILED(hr)) { MessageBox(hWnd, L"D3D11CreateDeviceAndSwapChain Failed.", L"Error", MB_OK); return false; }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to get back buffer.", L"Error", MB_OK); return false; }

    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create render target view.", L"Error", MB_OK); return false; }

    // --- Depth-Stencil Buffer Setup ---
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
    hr = m_device->CreateTexture2D(&depthDesc, nullptr, &depthStencilBuffer);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create depth texture.", L"Error", MB_OK); return false; }

    hr = m_device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, &m_depthStencilView);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create depth stencil view.", L"Error", MB_OK); return false; }

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    hr = m_device->CreateDepthStencilState(&dsDesc, &m_depthStencilState);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create depth stencil state.", L"Error", MB_OK); return false; }

    D3D11_DEPTH_STENCIL_DESC dsDisabledDesc = {};
    dsDisabledDesc.DepthEnable = FALSE;
    dsDisabledDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDisabledDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    hr = m_device->CreateDepthStencilState(&dsDisabledDesc, &m_depthDisabledState);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create depth-disabled stencil state.", L"Error", MB_OK); return false; }

    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // --- Viewport Setup ---
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_context->RSSetViewports(1, &vp);

    // --- Rasterizer States (Solid & Wireframe) ---
    D3D11_RASTERIZER_DESC solidDesc = {};
    solidDesc.FillMode = D3D11_FILL_SOLID;
    solidDesc.CullMode = D3D11_CULL_NONE;
    solidDesc.FrontCounterClockwise = FALSE;
    solidDesc.DepthClipEnable = TRUE;
    hr = m_device->CreateRasterizerState(&solidDesc, &m_rasterizerStateSolid);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create solid rasterizer state.", L"Error", MB_OK); return false; }

    D3D11_RASTERIZER_DESC wireDesc = {};
    wireDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireDesc.CullMode = D3D11_CULL_NONE;
    wireDesc.FrontCounterClockwise = FALSE;
    wireDesc.DepthClipEnable = TRUE;
    hr = m_device->CreateRasterizerState(&wireDesc, &m_rasterizerStateWireframe);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create wireframe rasterizer state.", L"Error", MB_OK); return false; }

    m_isWireframe = false;
    m_context->RSSetState(m_rasterizerStateSolid.Get());

    // --- Sprite Rendering Resources ---
    m_spriteShader = std::make_unique<Shader>();
    if (!m_spriteShader->Load(m_device.Get(), L"Shaders/SpriteVS.hlsl", L"Shaders/SpritePS.hlsl")) {
        MessageBox(hWnd, L"Failed to compile/load sprite shaders.", L"Error", MB_OK);
        return false;
    }
#ifdef HOT_RELOAD_ENABLED
    ShaderManager::GetInstance()->RegisterShader(m_spriteShader.get());
#endif

    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(ConstantBufferWVP);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_device->CreateBuffer(&cbd, nullptr, &m_constantBuffer);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create constant buffer.", L"Error", MB_OK); return false; }

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_device->CreateSamplerState(&sd, &m_samplerState);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create sampler state.", L"Error", MB_OK); return false; }

    // --- Terrain Rendering Resources ---
    D3D11_INPUT_ELEMENT_DESC terrainLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    m_terrainShader = std::make_unique<Shader>();
    if (!m_terrainShader->LoadWithLayout(m_device.Get(), L"Shaders/TerrainVS.hlsl", L"Shaders/TerrainPS.hlsl", terrainLayout, ARRAYSIZE(terrainLayout))) {
        MessageBox(hWnd, L"Failed to compile/load terrain shaders.", L"Error", MB_OK);
        return false;
    }
#ifdef HOT_RELOAD_ENABLED
    ShaderManager::GetInstance()->RegisterShader(m_terrainShader.get());
#endif

    D3D11_BUFFER_DESC tcbd = {};
    tcbd.Usage = D3D11_USAGE_DYNAMIC;
    tcbd.ByteWidth = sizeof(TerrainConstantBuffer);
    tcbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    tcbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_device->CreateBuffer(&tcbd, nullptr, &m_terrainConstantBuffer);
    if (FAILED(hr)) { MessageBox(hWnd, L"Failed to create terrain constant buffer.", L"Error", MB_OK); return false; }

    // Default 3D Perspective Matrix & View Matrix
    float aspect = (float)width / (float)height;
    m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(60.0f), aspect, 0.1f, 2000.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(
        DirectX::XMVectorSet(0.0f, 25.0f, -40.0f, 1.0f), // Eye position
        DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),   // Focus point
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)    // Up direction
    );

    return true;
}

void Graphics::Shutdown()
{
    // Resources are released by ComPtr when this object is destroyed.
}

void Graphics::BeginFrame()
{
    // Clear back buffer with dark blue
    const float clearColor[] = { 0.04f, 0.15f, 0.32f, 1.0f };
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

    if (m_depthStencilView)
    {
        m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
}

void Graphics::EndFrame()
{
    m_swapChain->Present(1, 0);
}

void Graphics::SetWireframe(bool enable)
{
    m_isWireframe = enable;
    if (m_isWireframe)
    {
        m_context->RSSetState(m_rasterizerStateWireframe.Get());
    }
    else
    {
        m_context->RSSetState(m_rasterizerStateSolid.Get());
    }
}

void Graphics::ToggleWireframe()
{
    SetWireframe(!m_isWireframe);
}

void Graphics::Draw(SpriteRenderer* renderer)
{
    if (!renderer) return;

    auto texture = renderer->m_texture;
    auto vertexBuffer = renderer->m_vertexBuffer;
    auto indexBuffer = renderer->m_indexBuffer;
    if (!texture || !vertexBuffer || !indexBuffer) return;

    m_spriteShader->Set(m_context.Get());

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        ConstantBufferWVP* dataPtr = (ConstantBufferWVP*)mappedResource.pData;
        DirectX::XMMATRIX world = renderer->GetOwner()->GetTransform()->GetWorldMatrix();
        dataPtr->wvp = DirectX::XMMatrixTranspose(world * m_viewMatrix * m_projectionMatrix);
        m_context->Unmap(m_constantBuffer.Get(), 0);
    }

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    texture->Set(m_context.Get(), 0);
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    m_context->DrawIndexed(6, 0, 0);
}

void Graphics::DrawTerrain(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexCount, const DirectX::XMMATRIX& worldMatrix)
{
    if (!vertexBuffer || !indexBuffer || indexCount == 0 || !m_terrainShader) return;

    m_terrainShader->Set(m_context.Get());

    DirectX::XMMATRIX wvp = worldMatrix * m_viewMatrix * m_projectionMatrix;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (SUCCEEDED(m_context->Map(m_terrainConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        TerrainConstantBuffer* dataPtr = (TerrainConstantBuffer*)mappedResource.pData;
        dataPtr->world = DirectX::XMMatrixTranspose(worldMatrix);
        dataPtr->view = DirectX::XMMatrixTranspose(m_viewMatrix);
        dataPtr->projection = DirectX::XMMatrixTranspose(m_projectionMatrix);
        dataPtr->worldViewProjection = DirectX::XMMatrixTranspose(wvp);
        dataPtr->lightDirection = DirectX::XMFLOAT4(0.577f, -0.707f, 0.577f, 0.0f);
        dataPtr->lightColor = DirectX::XMFLOAT4(1.0f, 0.98f, 0.92f, 1.0f);
        dataPtr->ambientColor = DirectX::XMFLOAT4(0.35f, 0.38f, 0.45f, 1.0f);
        m_context->Unmap(m_terrainConstantBuffer.Get(), 0);
    }

    UINT stride = sizeof(TerrainVertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetConstantBuffers(0, 1, m_terrainConstantBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(0, 1, m_terrainConstantBuffer.GetAddressOf());

    m_context->DrawIndexed(indexCount, 0, 0);
}

void Graphics::DrawMenu(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, ID3D11ShaderResourceView* textureSRV)
{
    if (!vertexBuffer || !indexBuffer || !textureSRV || !m_spriteShader) return;

    m_spriteShader->Set(m_context.Get());
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        ConstantBufferWVP* dataPtr = (ConstantBufferWVP*)mappedResource.pData;
        dataPtr->wvp = DirectX::XMMatrixIdentity();
        m_context->Unmap(m_constantBuffer.Get(), 0);
    }
    m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    m_context->PSSetShaderResources(0, 1, &textureSRV);

    if (m_depthDisabledState)
    {
        m_context->OMSetDepthStencilState(m_depthDisabledState.Get(), 0);
    }
    m_context->RSSetState(m_rasterizerStateSolid.Get());

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_context->DrawIndexed(6, 0, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_context->PSSetShaderResources(0, 1, &nullSRV);

    if (m_depthStencilState)
    {
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);
    }
    if (m_isWireframe)
    {
        m_context->RSSetState(m_rasterizerStateWireframe.Get());
    }
}
