#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "../Graphics/Shader.h"
#include <memory>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

class SpriteRenderer; // Forward declaration

struct ConstantBufferWVP
{
    DirectX::XMMATRIX wvp;
};

struct TerrainVertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 texCoord;
    DirectX::XMFLOAT4 color;
};

struct TerrainConstantBuffer
{
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMMATRIX worldViewProjection;
    DirectX::XMFLOAT4 lightDirection;
    DirectX::XMFLOAT4 lightColor;
    DirectX::XMFLOAT4 ambientColor;
};

class Graphics
{
public:
    Graphics();
    ~Graphics();

    bool Initialize(HWND hWnd, int width, int height);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void Draw(SpriteRenderer* renderer);
    void DrawTerrain(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexCount, const DirectX::XMMATRIX& worldMatrix);
    void DrawMenu(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, ID3D11ShaderResourceView* textureSRV);

    ID3D11Device* GetDevice() const { return m_device.Get(); }
    ID3D11DeviceContext* GetContext() const { return m_context.Get(); }

    void SetWireframe(bool enable);
    void ToggleWireframe();
    bool IsWireframe() const { return m_isWireframe; }

    void SetViewMatrix(const DirectX::XMMATRIX& view) { m_viewMatrix = view; }
    void SetProjectionMatrix(const DirectX::XMMATRIX& proj) { m_projectionMatrix = proj; }
    const DirectX::XMMATRIX& GetViewMatrix() const { return m_viewMatrix; }
    const DirectX::XMMATRIX& GetProjectionMatrix() const { return m_projectionMatrix; }

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthDisabledState;

    // Rasterizer States
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerStateSolid;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerStateWireframe;
    bool m_isWireframe;

    // Sprite Rendering Resources
    std::unique_ptr<Shader> m_spriteShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;

    // Terrain Rendering Resources
    std::unique_ptr<Shader> m_terrainShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_terrainConstantBuffer;

    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;
};
