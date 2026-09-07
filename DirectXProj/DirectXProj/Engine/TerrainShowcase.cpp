#include "TerrainShowcase.h"
#include "TerrainRenderer.h"
#include "MenuRenderer.h"
#include "Camera.h"
#include "../Core/Graphics.h"
#include "../Input/InputManager.h"
#include "../Core/TimeManager.h"
#include <sstream>

TerrainShowcase::TerrainShowcase(GameObject* owner)
    : Component(owner),
      m_hWnd(nullptr),
      m_graphics(nullptr),
      m_terrainRenderer(nullptr),
      m_menuRenderer(nullptr),
      m_camera(nullptr),
      m_currentMode(TerrainMode::MainMenu),
      m_titleUpdateTimer(0.0f),
      m_frameCount(0),
      m_fps(0)
{
}

TerrainShowcase::~TerrainShowcase()
{
}

void TerrainShowcase::Initialize(HWND hWnd, Graphics* graphics, TerrainRenderer* terrainRenderer, MenuRenderer* menuRenderer, Camera* camera)
{
    m_hWnd = hWnd;
    m_graphics = graphics;
    m_terrainRenderer = terrainRenderer;
    m_menuRenderer = menuRenderer;
    m_camera = camera;
}

void TerrainShowcase::Start()
{
    // Start with Main Menu as requested
    SetMode(TerrainMode::MainMenu);
}

void TerrainShowcase::Update()
{
    InputManager* input = InputManager::GetInstance();
    TimeManager* time = TimeManager::GetInstance();
    if (!input || !time) return;

    // Key 1: Basic Flat Grid
    if (input->IsKeyDown('1') || input->IsKeyDown(VK_NUMPAD1))
    {
        SetMode(TerrainMode::FlatGrid);
    }
    // Key 2: Perlin Noise Terrain
    else if (input->IsKeyDown('2') || input->IsKeyDown(VK_NUMPAD2))
    {
        SetMode(TerrainMode::PerlinNoise);
    }
    // Key 3: HeightMap Image Terrain
    else if (input->IsKeyDown('3') || input->IsKeyDown(VK_NUMPAD3))
    {
        SetMode(TerrainMode::HeightMap);
    }
    // ESC or 0 or Backspace: Return to Main Menu
    else if (input->IsKeyDown(VK_ESCAPE) || input->IsKeyDown('0') || input->IsKeyDown(VK_NUMPAD0) || input->IsKeyDown(VK_BACK))
    {
        SetMode(TerrainMode::MainMenu);
    }

    // 메인 메뉴 화면에서 마우스 좌클릭 시 항목 감지
    if (m_currentMode == TerrainMode::MainMenu && input->IsKeyDown(VK_LBUTTON))
    {
        if (m_menuRenderer)
        {
            int clicked = m_menuRenderer->CheckItemClick(input->GetMouseX(), input->GetMouseY());
            if (clicked == 1)
            {
                SetMode(TerrainMode::FlatGrid);
            }
            else if (clicked == 2)
            {
                SetMode(TerrainMode::PerlinNoise);
            }
            else if (clicked == 3)
            {
                SetMode(TerrainMode::HeightMap);
            }
        }
    }

    // F1 또는 F 키: 와이어프레임 토글
    if (input->IsKeyDown(VK_F1) || input->IsKeyDown('F'))
    {
        if (m_graphics)
        {
            m_graphics->ToggleWireframe();
            UpdateWindowTitle();
        }
    }

    // FPS 계산 및 타이틀 갱신
    m_frameCount++;
    m_titleUpdateTimer += time->GetDeltaTime();
    if (m_titleUpdateTimer >= 0.25f)
    {
        m_fps = static_cast<int>(m_frameCount / m_titleUpdateTimer);
        m_frameCount = 0;
        m_titleUpdateTimer = 0.0f;
        UpdateWindowTitle();
    }
}

void TerrainShowcase::SetMode(TerrainMode mode)
{
    m_currentMode = mode;

    if (mode == TerrainMode::MainMenu)
    {
        if (m_menuRenderer) m_menuRenderer->SetVisible(true);
        if (m_terrainRenderer) m_terrainRenderer->SetVisible(false);
        if (m_camera) m_camera->SetEnabled(false);
    }
    else if (mode == TerrainMode::FlatGrid)
    {
        if (m_menuRenderer) m_menuRenderer->SetVisible(false);
        if (m_terrainRenderer)
        {
            m_terrainRenderer->SetVisible(true);
            m_terrainRenderer->CreateFlatGrid(65, 65, 1.0f);
        }
        if (m_camera)
        {
            m_camera->SetEnabled(true);
            m_camera->SetPosition(DirectX::XMFLOAT3(0.0f, 25.0f, -40.0f));
            m_camera->SetRotation(25.0f, 0.0f);
        }
    }
    else if (mode == TerrainMode::PerlinNoise)
    {
        if (m_menuRenderer) m_menuRenderer->SetVisible(false);
        if (m_terrainRenderer)
        {
            m_terrainRenderer->SetVisible(true);
            // Generate rich Perlin Noise terrain with 129x129 grid
            m_terrainRenderer->CreatePerlinNoiseTerrain(129, 129, 1.0f, 24.0f, 0.035f);
        }
        if (m_camera)
        {
            m_camera->SetEnabled(true);
            // Position camera higher to view sweeping mountains
            m_camera->SetPosition(DirectX::XMFLOAT3(0.0f, 42.0f, -70.0f));
            m_camera->SetRotation(32.0f, 0.0f);
        }
    }
    else if (mode == TerrainMode::HeightMap)
    {
        if (m_menuRenderer) m_menuRenderer->SetVisible(false);
        if (m_terrainRenderer)
        {
            m_terrainRenderer->SetVisible(true);
            // Generate high-density HeightMap terrain with 257x257 grid
            m_terrainRenderer->CreateHeightMapTerrain(L"Assets/heightmap.jpg", 257, 257, 1.0f, 36.0f);
        }
        if (m_camera)
        {
            m_camera->SetEnabled(true);
            // Wide sweeping viewpoint for grand heightmap mountain ranges
            m_camera->SetPosition(DirectX::XMFLOAT3(0.0f, 65.0f, -110.0f));
            m_camera->SetRotation(30.0f, 0.0f);
        }
    }

    UpdateWindowTitle();
}

void TerrainShowcase::UpdateWindowTitle()
{
    if (!m_hWnd) return;

    std::wstring wireframeStr = (m_graphics && m_graphics->IsWireframe()) ? L"ON" : L"OFF";
    std::wstringstream ss;

    if (m_currentMode == TerrainMode::MainMenu)
    {
        ss << L"[Main Menu] | Wireframe: " << wireframeStr
           << L" | [1~9: 샘플 선택] [1: 기본 평면 그리드, 2: 펄린 노이즈, 3: 높이맵 지형]";
    }
    else if (m_currentMode == TerrainMode::FlatGrid)
    {
        ss << L"[Terrain Showcase] 1. 기본 평면 그리드 (Basic Flat Grid)"
           << L" | Wireframe (F1): " << wireframeStr
           << L" | FPS: " << m_fps
           << L" | [ESC: 메인메뉴, 2: 펄린, 3: 높이맵, WASD: 이동, 우클릭: 회전]";
    }
    else if (m_currentMode == TerrainMode::PerlinNoise)
    {
        ss << L"[Terrain Showcase] 2. 펄린 노이즈 지형 (Perlin Noise)"
           << L" | Wireframe (F1): " << wireframeStr
           << L" | FPS: " << m_fps
           << L" | [ESC: 메인메뉴, 1: 평면, 3: 높이맵, WASD: 이동, 우클릭: 회전]";
    }
    else if (m_currentMode == TerrainMode::HeightMap)
    {
        ss << L"[Terrain Showcase] 3. 높이맵 이미지 지형 (HeightMap)"
           << L" | Wireframe (F1): " << wireframeStr
           << L" | FPS: " << m_fps
           << L" | [ESC: 메인메뉴, 1: 평면, 2: 펄린, WASD: 이동, 우클릭: 회전]";
    }

    SetWindowTextW(m_hWnd, ss.str().c_str());
}
