#pragma once
#include "Component.h"
#include <Windows.h>
#include <string>

class Graphics;
class TerrainRenderer;
class MenuRenderer;
class Camera;

enum class TerrainMode
{
    MainMenu = 0,
    FlatGrid = 1,
    PerlinNoise = 2,
    HeightMap = 3,
    TextureSplatting = 4,
    QuadTreeCulling = 5,
    DistanceLOD1 = 6,
    DistanceLOD2 = 7,
    Tessellation = 8,
    SkyDome = 9,
    InfiniteChunks = 10
};

class TerrainShowcase : public Component
{
public:
    TerrainShowcase(GameObject* owner);
    virtual ~TerrainShowcase();

    void Initialize(HWND hWnd, Graphics* graphics, TerrainRenderer* terrainRenderer, MenuRenderer* menuRenderer, Camera* camera);

    virtual void Start() override;
    virtual void Update() override;

    void SetMode(TerrainMode mode);
    TerrainMode GetMode() const { return m_currentMode; }

private:
    void UpdateWindowTitle();

    HWND m_hWnd;
    Graphics* m_graphics;
    TerrainRenderer* m_terrainRenderer;
    MenuRenderer* m_menuRenderer;
    Camera* m_camera;
    TerrainMode m_currentMode;

    float m_titleUpdateTimer;
    int m_frameCount;
    int m_fps;
};
