#include "MenuRenderer.h"
#include "GameObject.h"
#include "../Core/Graphics.h"
#include "../Engine/SpriteRenderer.h"
#include "../Input/InputManager.h"
#include <fstream>
#include <sstream>
#include <windows.h>

namespace {
    std::wstring Utf8ToWstring(const std::string& str)
    {
        if (str.empty()) return {};
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
        std::wstring result(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size);
        return result;
    }
}

MenuRenderer::MenuRenderer(GameObject* owner)
    : Component(owner),
      m_graphics(nullptr),
      m_screenWidth(1280),
      m_screenHeight(720),
      m_isVisible(true),
      m_hoveredItem(-1)
{
}

MenuRenderer::~MenuRenderer()
{
}

bool MenuRenderer::Initialize(Graphics* graphics, int screenWidth, int screenHeight)
{
    m_graphics = graphics;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    CreateQuadBuffers();
    ReloadMenuText(L"Assets/menu.txt");

    return true;
}

void MenuRenderer::Start()
{
    if (m_menuLines.empty())
    {
        ReloadMenuText(L"Assets/menu.txt");
    }
}

void MenuRenderer::Update()
{
    if (!m_isVisible) return;

    InputManager* input = InputManager::GetInstance();
    if (!input) return;

    int newHover = CheckItemClick(input->GetMouseX(), input->GetMouseY());
    if (newHover != m_hoveredItem)
    {
        m_hoveredItem = newHover;
        RedrawMenuTexture();
    }
}

void MenuRenderer::RedrawMenuTexture()
{
    if (!m_menuLines.empty())
    {
        CreateMenuTexture(m_menuLines);
    }
}

bool MenuRenderer::ReloadMenuText(const std::wstring& filePath)
{
    m_menuLines.clear();

    std::vector<std::wstring> candidatePaths = {
        filePath,
        L"Assets/menu.txt",
        L"DirectXProj/Assets/menu.txt",
        L"../Assets/menu.txt"
    };

    std::ifstream file;
    for (const auto& path : candidatePaths)
    {
        file.open(path);
        if (file.is_open()) break;
    }

    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            m_menuLines.push_back(Utf8ToWstring(line));
        }
        file.close();
    }

    // Fallback default lines if file couldn't be loaded
    if (m_menuLines.empty())
    {
        m_menuLines.push_back(L"=== Terrain Showcase System ===");
        m_menuLines.push_back(L"마우스 클릭 또는 키(1~9, 0, -)로 원하는 지형 샘플을 선택하세요");
        m_menuLines.push_back(L"");
        m_menuLines.push_back(L"1. 기본 평면 그리드 (Basic Flat Grid)");
        m_menuLines.push_back(L"2. 펄린 노이즈 지형 (Perlin Noise)");
        m_menuLines.push_back(L"3. 높이맵 이미지 지형 (HeightMap)");
        m_menuLines.push_back(L"4. 텍스처 스플래팅 정점 경사도/높이 기반 (Texture Splatting)");
        m_menuLines.push_back(L"5. 쿼드트리 컬링 (QuadTree Culling)");
        m_menuLines.push_back(L"6. 거리 기반 LOD 지형 1 (Distance LOD 1)");
        m_menuLines.push_back(L"6-2. 고급 거리 LOD 지형 2 (스티칭 & 지오머핑)");
        m_menuLines.push_back(L"7. 하드웨어 테셀레이션 지형 (Tessellation)");
        m_menuLines.push_back(L"8. 스카이맵 (SkyDome / SkyBox)");
        m_menuLines.push_back(L"9. 동적 왜곡 구름 (Perturbed Clouds)");
        m_menuLines.push_back(L"10. 무한 지형 청크 (Infinite Chunks)");
    }

    CreateMenuTexture(m_menuLines);
    return true;
}

void MenuRenderer::CreateMenuTexture(const std::vector<std::wstring>& lines)
{
    if (!m_graphics) return;
    ID3D11Device* device = m_graphics->GetDevice();
    if (!device) return;

    int width = m_screenWidth;
    int height = m_screenHeight;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hBitmap);

    // Deep rich blue background
    HBRUSH bgBrush = CreateSolidBrush(RGB(6, 41, 85));
    RECT rcScreen = { 0, 0, width, height };
    FillRect(memDC, &rcScreen, bgBrush);
    DeleteObject(bgBrush);

    SetBkMode(memDC, TRANSPARENT);

    HFONT hTitleFont = CreateFontW(
        32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"맑은 고딕"
    );

    HFONT hSubFont = CreateFontW(
        21, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"맑은 고딕"
    );

    HFONT hItemFont = CreateFontW(
        23, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"맑은 고딕"
    );

    m_itemHitBoxes.clear();
    m_itemHitBoxes.resize(12, { 0, 0, 0, 0 });

    int curY = 65;
    int lineIndex = 0;

    for (const auto& line : lines)
    {
        if (line.empty())
        {
            curY += 12;
            continue;
        }

        RECT rc = { 0, curY, width, curY + 45 };

        if (lineIndex == 0 || line.find(L"===") != std::wstring::npos)
        {
            SelectObject(memDC, hTitleFont);
            SetTextColor(memDC, RGB(255, 222, 60));
            DrawTextW(memDC, line.c_str(), -1, &rc, DT_CENTER | DT_SINGLELINE);
            curY += 44;
        }
        else if (lineIndex == 1 || line.find(L"마우스 클릭") != std::wstring::npos)
        {
            SelectObject(memDC, hSubFont);
            SetTextColor(memDC, RGB(180, 215, 255));
            DrawTextW(memDC, line.c_str(), -1, &rc, DT_CENTER | DT_SINGLELINE);
            curY += 48;
        }
        else
        {
            SelectObject(memDC, hItemFont);

            int itemNum = -1;
            if (line.rfind(L"1.", 0) == 0) itemNum = 1;
            else if (line.rfind(L"2.", 0) == 0) itemNum = 2;
            else if (line.rfind(L"3.", 0) == 0) itemNum = 3;
            else if (line.rfind(L"4.", 0) == 0) itemNum = 4;
            else if (line.rfind(L"5.", 0) == 0) itemNum = 5;
            else if (line.rfind(L"6-2.", 0) == 0) itemNum = 7;
            else if (line.rfind(L"6.", 0) == 0) itemNum = 6;
            else if (line.rfind(L"7.", 0) == 0) itemNum = 8;
            else if (line.rfind(L"8.", 0) == 0) itemNum = 9;
            else if (line.rfind(L"9.", 0) == 0) itemNum = 10;
            else if (line.rfind(L"10.", 0) == 0) itemNum = 11;

            int leftMargin = width / 2 - 250;
            RECT itemRc = { leftMargin, curY, leftMargin + 560, curY + 32 };

            if (itemNum > 0 && itemNum < (int)m_itemHitBoxes.size())
            {
                m_itemHitBoxes[itemNum] = itemRc;
            }

            bool isHovered = (itemNum > 0 && itemNum == m_hoveredItem);
            if (isHovered)
            {
                // Hover highlight background bar
                HBRUSH hHoverBrush = CreateSolidBrush(RGB(15, 60, 120));
                RECT hoverBg = { itemRc.left - 10, itemRc.top - 2, itemRc.right + 10, itemRc.bottom + 2 };
                FillRect(memDC, &hoverBg, hHoverBrush);
                DeleteObject(hHoverBrush);

                // Golden yellow hover text
                SetTextColor(memDC, RGB(255, 235, 50));
            }
            else
            {
                // Normal item text: uniform clean white / light ice blue
                SetTextColor(memDC, RGB(225, 238, 255));
            }

            DrawTextW(memDC, line.c_str(), -1, &itemRc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
            curY += 34;
        }

        lineIndex++;
    }

    GdiFlush();

    if (pBits)
    {
        uint32_t* pixels = reinterpret_cast<uint32_t*>(pBits);
        int total = width * height;
        for (int i = 0; i < total; ++i)
        {
            pixels[i] |= 0xFF000000;
        }
    }

    if (m_menuTexture)
    {
        // In-place subresource update for instant, smooth hover response
        m_graphics->GetContext()->UpdateSubresource(m_menuTexture.Get(), 0, nullptr, pBits, width * 4, 0);
    }
    else
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subData = {};
        subData.pSysMem = pBits;
        subData.SysMemPitch = width * 4;

        m_menuTexture.Reset();
        m_menuSRV.Reset();

        HRESULT hr = device->CreateTexture2D(&texDesc, &subData, &m_menuTexture);
        if (SUCCEEDED(hr))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = texDesc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            device->CreateShaderResourceView(m_menuTexture.Get(), &srvDesc, &m_menuSRV);
        }
    }

    SelectObject(memDC, hOldBmp);
    DeleteObject(hBitmap);
    DeleteObject(hTitleFont);
    DeleteObject(hSubFont);
    DeleteObject(hItemFont);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

void MenuRenderer::CreateQuadBuffers()
{
    if (!m_graphics) return;
    ID3D11Device* device = m_graphics->GetDevice();
    if (!device) return;

    Vertex quadVertices[] = {
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },
        { DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },
    };

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(Vertex) * 4;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vData = {};
    vData.pSysMem = quadVertices;

    device->CreateBuffer(&vbd, &vData, &m_vertexBuffer);

    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(unsigned int) * 6;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iData = {};
    iData.pSysMem = indices;

    device->CreateBuffer(&ibd, &iData, &m_indexBuffer);

    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(ConstantBufferWVP);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    device->CreateBuffer(&cbd, nullptr, &m_wvpBuffer);
}

void MenuRenderer::Render()
{
    if (!m_isVisible || !m_vertexBuffer || !m_indexBuffer || !m_menuSRV || !m_graphics)
    {
        return;
    }

    m_graphics->DrawMenu(m_vertexBuffer.Get(), m_indexBuffer.Get(), m_menuSRV.Get());
}

int MenuRenderer::CheckItemClick(int mouseX, int mouseY) const
{
    POINT pt = { mouseX, mouseY };
    for (size_t i = 1; i < m_itemHitBoxes.size(); ++i)
    {
        if (PtInRect(&m_itemHitBoxes[i], pt))
        {
            return (int)i;
        }
    }
    return -1;
}