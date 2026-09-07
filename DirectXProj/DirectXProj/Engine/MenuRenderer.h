#pragma once
#include "Component.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

class Graphics;

class MenuRenderer : public Component
{
public:
    MenuRenderer(GameObject* owner);
    virtual ~MenuRenderer();

    bool Initialize(Graphics* graphics, int screenWidth = 1280, int screenHeight = 720);
    virtual void Start() override;
    virtual void Update() override;
    virtual void Render() override;

    // Reload menu text from file and recreate texture
    bool ReloadMenuText(const std::wstring& filePath = L"Assets/menu.txt");
    void RedrawMenuTexture();

    // Check if mouse click hits any menu item (returns -1 if none, 1 for FlatGrid, 2 for Perlin, etc.)
    int CheckItemClick(int mouseX, int mouseY) const;

    void SetVisible(bool visible) { m_isVisible = visible; }
    bool IsVisible() const { return m_isVisible; }

private:
    void CreateMenuTexture(const std::vector<std::wstring>& lines);
    void CreateQuadBuffers();

    Graphics* m_graphics;
    int m_screenWidth;
    int m_screenHeight;
    bool m_isVisible;
    int m_hoveredItem; // -1 if no item hovered

    std::vector<std::wstring> m_menuLines;
    std::vector<RECT> m_itemHitBoxes; // Screen rectangles for each numbered item

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_menuTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_menuSRV;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_wvpBuffer;
};