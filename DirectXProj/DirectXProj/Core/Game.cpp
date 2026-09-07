#include "stdafx.h"
#include "Game.h"
#include "Engine/GameObject.h"
#include "Engine/SpriteRenderer.h"
#include "Engine/TerrainRenderer.h"
#include "Engine/TerrainShowcase.h"
#include "Engine/Camera.h"
#include "Engine/MenuRenderer.h"
#include "Engine/ComponentFactory.h"
#include "Engine/Transform.h"
#ifdef HOT_RELOAD_ENABLED
#include "Graphics/ShaderManager.h"
#endif

Game::Game(HINSTANCE hInstance)
    : m_hInstance(hInstance), m_inputManager(nullptr), m_sceneManager(nullptr), m_timeManager(nullptr)
{
}

Game::~Game()
{
    Shutdown();
}

bool Game::Initialize(const std::wstring& title, int width, int height)
{
    // Create Window
    m_window = std::make_unique<Window>(m_hInstance, title, width, height);
    if (!m_window->Create()) return false;

    // Create Graphics
    m_graphics = std::make_unique<Graphics>();
    if (!m_graphics->Initialize(m_window->GetHWND(), width, height)) return false;

    // Get Managers
    m_inputManager = InputManager::GetInstance();
    m_sceneManager = SceneManager::GetInstance();
    m_timeManager = TimeManager::GetInstance();
    m_timeManager->Initialize();

    // Register components with the factory
    auto* factory = ComponentFactory::GetInstance();
    factory->Register<Transform>(typeid(Transform).name());
    factory->Register<SpriteRenderer>(typeid(SpriteRenderer).name());
    factory->Register<Camera>(typeid(Camera).name());
    factory->Register<TerrainRenderer>(typeid(TerrainRenderer).name());
    factory->Register<TerrainShowcase>(typeid(TerrainShowcase).name());
    factory->Register<MenuRenderer>(typeid(MenuRenderer).name());

    // Load initial scene
    m_sceneManager->LoadScene("Terrain Showcase Scene");
    auto* activeScene = m_sceneManager->GetActiveScene();

    // --- 1. Main 3D Camera ---
    auto cameraGo = activeScene->AddGameObject("Main Camera");
    auto cameraComp = cameraGo->AddComponent<Camera>();
    cameraComp->Initialize(m_graphics.get(), 60.0f, 0.1f, 2000.0f);
    cameraComp->SetPosition(DirectX::XMFLOAT3(0.0f, 25.0f, -40.0f));
    cameraComp->SetRotation(25.0f, 0.0f);

    // --- 2. Terrain GameObject ---
    auto terrainGo = activeScene->AddGameObject("Terrain");
    auto terrainComp = terrainGo->AddComponent<TerrainRenderer>();
    terrainComp->Initialize(m_graphics.get());
    terrainGo->GetTransform()->SetLocalPosition(0.0f, 0.0f, 0.0f);

    // --- 3. Main Menu UI (Assets/menu.txt based) ---
    auto menuGo = activeScene->AddGameObject("MenuUI");
    auto menuComp = menuGo->AddComponent<MenuRenderer>();
    menuComp->Initialize(m_graphics.get(), width, height);

    // --- 4. Terrain Showcase Controller ---
    auto showcaseGo = activeScene->AddGameObject("TerrainShowcaseController");
    auto showcaseComp = showcaseGo->AddComponent<TerrainShowcase>();
    showcaseComp->Initialize(m_window->GetHWND(), m_graphics.get(), terrainComp, menuComp, cameraComp);

    // Process pending additions so all components are committed before Initialize & Start
    m_sceneManager->ProcessPendingChanges();

    // Initialize components that need graphics resources
    m_sceneManager->Initialize(m_graphics.get());

    // Start all game objects and components
    m_sceneManager->GetActiveScene()->Start();

    return true;
}

void Game::Run()
{
    GameLoop();
}

void Game::Shutdown()
{
    if (m_sceneManager)
    {
        m_sceneManager->OnDestroy();
    }
    if (m_graphics)
    {
        m_graphics->Shutdown();
    }
}

void Game::GameLoop()
{
    while (m_window->ProcessMessages())
    {
        // 0. Update Time
        m_timeManager->Update();

#ifdef HOT_RELOAD_ENABLED
        ShaderManager::GetInstance()->Update();
#endif

        // 1. Update Game Logic (IsKeyDown is valid here!)
        m_sceneManager->Update();

        // 2. Render
        m_graphics->BeginFrame();
        m_sceneManager->Render(); // This will call render on all components
        m_graphics->EndFrame();

        // 3. Process pending component/object changes
        m_sceneManager->ProcessPendingChanges();

        // 4. Update Input states (Down -> Pressed, Up -> None) at end of frame
        m_inputManager->Update();
    }
}
