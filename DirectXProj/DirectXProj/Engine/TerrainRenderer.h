#pragma once
#include "Component.h"
#include "../Core/Graphics.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>

class TerrainRenderer : public Component
{
public:
    TerrainRenderer(GameObject* owner);
    virtual ~TerrainRenderer();

    void Initialize(Graphics* graphics);
    virtual void Start() override;
    virtual void Render() override;

    // Generate basic flat grid mesh
    bool CreateFlatGrid(int width = 65, int depth = 65, float cellSpacing = 1.0f);

    // Generate 3D Perlin Noise terrain mesh
    bool CreatePerlinNoiseTerrain(int width = 129, int depth = 129, float cellSpacing = 1.0f, float heightScale = 22.0f, float noiseScale = 0.035f);

    // Generate 3D HeightMap image based terrain mesh
    bool CreateHeightMapTerrain(const std::wstring& imagePath = L"Assets/heightmap.jpg", int gridWidth = 257, int gridDepth = 257, float cellSpacing = 1.0f, float heightScale = 36.0f);

    int GetGridWidth() const { return m_width; }
    int GetGridDepth() const { return m_depth; }
    float GetCellSpacing() const { return m_cellSpacing; }
    UINT GetIndexCount() const { return m_indexCount; }

    void SetVisible(bool visible) { m_isVisible = visible; }
    bool IsVisible() const { return m_isVisible; }

private:
    Graphics* m_graphics;
    bool m_isVisible;

    int m_width;
    int m_depth;
    float m_cellSpacing;
    UINT m_indexCount;

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
};
