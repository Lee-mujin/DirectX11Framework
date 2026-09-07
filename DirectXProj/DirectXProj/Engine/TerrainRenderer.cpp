#include "TerrainRenderer.h"
#include "GameObject.h"
#include "Transform.h"
#include "../Utils/PerlinNoise.h"
#include <wincodec.h>
#include <vector>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "windowscodecs.lib")

TerrainRenderer::TerrainRenderer(GameObject* owner)
    : Component(owner),
      m_graphics(nullptr),
      m_isVisible(true),
      m_width(65),
      m_depth(65),
      m_cellSpacing(1.0f),
      m_indexCount(0)
{
}

TerrainRenderer::~TerrainRenderer()
{
}

void TerrainRenderer::Initialize(Graphics* graphics)
{
    m_graphics = graphics;
    CreateFlatGrid(m_width, m_depth, m_cellSpacing);
}

void TerrainRenderer::Start()
{
    if (m_graphics && m_indexCount == 0)
    {
        CreateFlatGrid(m_width, m_depth, m_cellSpacing);
    }
}

void TerrainRenderer::Render()
{
    if (!m_isVisible || !m_vertexBuffer || !m_indexBuffer || m_indexCount == 0 || !m_graphics)
    {
        return;
    }

    DirectX::XMMATRIX worldMatrix = GetOwner()->GetTransform()->GetWorldMatrix();
    m_graphics->DrawTerrain(m_vertexBuffer.Get(), m_indexBuffer.Get(), m_indexCount, worldMatrix);
}

bool TerrainRenderer::CreateFlatGrid(int width, int depth, float cellSpacing)
{
    if (!m_graphics) return false;
    ID3D11Device* device = m_graphics->GetDevice();
    if (!device) return false;

    m_width = width;
    m_depth = depth;
    m_cellSpacing = cellSpacing;

    int totalVertices = width * depth;
    std::vector<TerrainVertex> vertices(totalVertices);

    float halfW = (width - 1) * cellSpacing * 0.5f;
    float halfD = (depth - 1) * cellSpacing * 0.5f;

    // Generate grid vertices
    for (int z = 0; z < depth; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            int index = z * width + x;

            float posX = x * cellSpacing - halfW;
            float posY = 0.0f; // Basic flat grid: height is 0
            float posZ = z * cellSpacing - halfD;

            vertices[index].position = DirectX::XMFLOAT3(posX, posY, posZ);
            vertices[index].normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
            vertices[index].texCoord = DirectX::XMFLOAT2(
                static_cast<float>(x) / static_cast<float>(width - 1),
                static_cast<float>(z) / static_cast<float>(depth - 1)
            );

            // Subtle checker / grid coloring for high readability
            bool isMajorLine = (x % 5 == 0 || z % 5 == 0);
            bool isChecker = ((x + z) % 2 == 0);

            if (isMajorLine)
            {
                // Crisp subtle highlight on every 5th line
                vertices[index].color = DirectX::XMFLOAT4(0.48f, 0.65f, 0.45f, 1.0f);
            }
            else if (isChecker)
            {
                vertices[index].color = DirectX::XMFLOAT4(0.38f, 0.56f, 0.38f, 1.0f);
            }
            else
            {
                vertices[index].color = DirectX::XMFLOAT4(0.33f, 0.50f, 0.33f, 1.0f);
            }
        }
    }

    // Generate indices (2 triangles per quad cell)
    int numQuads = (width - 1) * (depth - 1);
    m_indexCount = numQuads * 6;
    std::vector<UINT> indices(m_indexCount);

    int idx = 0;
    for (int z = 0; z < depth - 1; ++z)
    {
        for (int x = 0; x < width - 1; ++x)
        {
            UINT v0 = static_cast<UINT>(z * width + x);             // Bottom-Left
            UINT v1 = static_cast<UINT>((z + 1) * width + x);       // Top-Left
            UINT v2 = static_cast<UINT>((z + 1) * width + (x + 1)); // Top-Right
            UINT v3 = static_cast<UINT>(z * width + (x + 1));       // Bottom-Right

            // Triangle 1: v0 -> v1 -> v2 (Clockwise from top)
            indices[idx++] = v0;
            indices[idx++] = v1;
            indices[idx++] = v2;

            // Triangle 2: v0 -> v2 -> v3
            indices[idx++] = v0;
            indices[idx++] = v2;
            indices[idx++] = v3;
        }
    }

    // Create D3D11 Vertex Buffer
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(TerrainVertex) * totalVertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vData = {};
    vData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vbd, &vData, &m_vertexBuffer);
    if (FAILED(hr)) return false;

    // Create D3D11 Index Buffer
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(UINT) * m_indexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iData = {};
    iData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibd, &iData, &m_indexBuffer);
    if (FAILED(hr)) return false;

    return true;
}

bool TerrainRenderer::CreatePerlinNoiseTerrain(int width, int depth, float cellSpacing, float heightScale, float noiseScale)
{
    if (!m_graphics) return false;
    ID3D11Device* device = m_graphics->GetDevice();
    if (!device) return false;

    m_width = width;
    m_depth = depth;
    m_cellSpacing = cellSpacing;

    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();

    PerlinNoise perlin(2026);

    int totalVertices = width * depth;
    std::vector<TerrainVertex> vertices(totalVertices);
    std::vector<float> heights(totalVertices);

    float halfW = (width - 1) * cellSpacing * 0.5f;
    float halfD = (depth - 1) * cellSpacing * 0.5f;

    // 1. Calculate heights using multi-octave Perlin noise
    for (int z = 0; z < depth; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            int index = z * width + x;

            float sampleX = x * noiseScale;
            float sampleZ = z * noiseScale;

            // 5 octaves fBm for rich mountainous landscapes
            float noiseVal = perlin.OctaveNoise2D(sampleX, sampleZ, 5, 0.5f, 2.0f);
            
            // Non-linear shaping for sharper peaks and broader valleys
            float sign = (noiseVal >= 0.0f) ? 1.0f : -1.0f;
            float shaped = sign * powf(fabsf(noiseVal), 1.25f);

            float posY = shaped * heightScale;
            heights[index] = posY;
        }
    }

    // Helper lambda to safely query height at grid coordinates
    auto GetHeight = [&](int x, int z) -> float {
        x = std::clamp(x, 0, width - 1);
        z = std::clamp(z, 0, depth - 1);
        return heights[z * width + x];
    };

    // 2. Generate vertices with accurate normals and elevation-based coloring
    for (int z = 0; z < depth; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            int index = z * width + x;

            float posX = x * cellSpacing - halfW;
            float posY = heights[index];
            float posZ = z * cellSpacing - halfD;

            vertices[index].position = DirectX::XMFLOAT3(posX, posY, posZ);

            // Central difference normal calculation
            float hL = GetHeight(x - 1, z);
            float hR = GetHeight(x + 1, z);
            float hD = GetHeight(x, z - 1);
            float hU = GetHeight(x, z + 1);

            DirectX::XMVECTOR normalVec = DirectX::XMVectorSet(
                (hL - hR) / (2.0f * cellSpacing),
                1.0f,
                (hD - hU) / (2.0f * cellSpacing),
                0.0f
            );
            normalVec = DirectX::XMVector3Normalize(normalVec);
            DirectX::XMStoreFloat3(&vertices[index].normal, normalVec);

            vertices[index].texCoord = DirectX::XMFLOAT2(
                static_cast<float>(x) / static_cast<float>(width - 1),
                static_cast<float>(z) / static_cast<float>(depth - 1)
            );

            // Slope factor (1.0 = flat, 0.0 = sheer vertical cliff)
            float slope = vertices[index].normal.y;

            // Height-based & slope-based terrain coloring
            DirectX::XMFLOAT4 vertexColor;

            if (posY < -3.5f)
            {
                // Deep valley / waterbed (Sandy / dark wet earth)
                vertexColor = DirectX::XMFLOAT4(0.32f, 0.42f, 0.35f, 1.0f);
            }
            else if (posY < 5.0f)
            {
                // Lush green lowlands and hills
                float t = (posY - (-3.5f)) / 8.5f;
                vertexColor = DirectX::XMFLOAT4(
                    0.28f + 0.08f * t,
                    0.52f + 0.08f * t,
                    0.22f + 0.04f * t,
                    1.0f
                );
            }
            else if (posY < 13.0f)
            {
                // Rocky high slopes / mountain cliffs (Gray-brown)
                float t = (posY - 5.0f) / 8.0f;
                vertexColor = DirectX::XMFLOAT4(
                    0.40f + 0.15f * t,
                    0.42f + 0.08f * t,
                    0.35f + 0.10f * t,
                    1.0f
                );
            }
            else
            {
                // Snow-capped peaks (White with soft cool tint)
                float t = std::clamp((posY - 13.0f) / 6.0f, 0.0f, 1.0f);
                vertexColor = DirectX::XMFLOAT4(
                    0.75f + 0.22f * t,
                    0.78f + 0.20f * t,
                    0.82f + 0.16f * t,
                    1.0f
                );
            }

            // If cliff slope is steep, blend toward rock color
            if (slope < 0.65f)
            {
                float rockBlend = std::clamp((0.65f - slope) / 0.35f, 0.0f, 0.85f);
                DirectX::XMFLOAT4 rockColor(0.42f, 0.39f, 0.36f, 1.0f);
                vertexColor.x = vertexColor.x * (1.0f - rockBlend) + rockColor.x * rockBlend;
                vertexColor.y = vertexColor.y * (1.0f - rockBlend) + rockColor.y * rockBlend;
                vertexColor.z = vertexColor.z * (1.0f - rockBlend) + rockColor.z * rockBlend;
            }

            vertices[index].color = vertexColor;
        }
    }

    // 3. Generate indices
    int numQuads = (width - 1) * (depth - 1);
    m_indexCount = numQuads * 6;
    std::vector<UINT> indices(m_indexCount);

    int idx = 0;
    for (int z = 0; z < depth - 1; ++z)
    {
        for (int x = 0; x < width - 1; ++x)
        {
            UINT v0 = static_cast<UINT>(z * width + x);             // Bottom-Left
            UINT v1 = static_cast<UINT>((z + 1) * width + x);       // Top-Left
            UINT v2 = static_cast<UINT>((z + 1) * width + (x + 1)); // Top-Right
            UINT v3 = static_cast<UINT>(z * width + (x + 1));       // Bottom-Right

            indices[idx++] = v0;
            indices[idx++] = v1;
            indices[idx++] = v2;

            indices[idx++] = v0;
            indices[idx++] = v2;
            indices[idx++] = v3;
        }
    }

    // 4. Create D3D11 Buffers
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(TerrainVertex) * totalVertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vData = {};
    vData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vbd, &vData, &m_vertexBuffer);
    if (FAILED(hr)) return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(UINT) * m_indexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iData = {};
    iData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibd, &iData, &m_indexBuffer);
    if (FAILED(hr)) return false;

    return true;
}

bool TerrainRenderer::CreateHeightMapTerrain(const std::wstring& imagePath, int gridWidth, int gridDepth, float cellSpacing, float heightScale)
{
    if (!m_graphics) return false;
    ID3D11Device* device = m_graphics->GetDevice();
    if (!device) return false;

    // 1. Resolve image file path across multiple candidate directories
    std::vector<std::wstring> candidatePaths = {
        imagePath,
        L"Assets/heightmap.jpg",
        L"DirectXProj/Assets/heightmap.jpg",
        L"../Assets/heightmap.jpg",
        L"Assets/" + imagePath,
        L"DirectXProj/Assets/" + imagePath,
        L"../Assets/" + imagePath
    };

    std::wstring resolvedPath;
    for (const auto& path : candidatePaths)
    {
        DWORD dwAttrib = GetFileAttributesW(path.c_str());
        if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            resolvedPath = path;
            break;
        }
    }

    if (resolvedPath.empty())
    {
        // Fallback to Perlin noise if image is missing
        return CreatePerlinNoiseTerrain(gridWidth, gridDepth, cellSpacing, heightScale);
    }

    // 2. Decode image using WIC (Windows Imaging Component)
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory)
    );
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> wicDecoder;
    hr = wicFactory->CreateDecoderFromFilename(
        resolvedPath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &wicDecoder
    );
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> wicFrame;
    hr = wicDecoder->GetFrame(0, &wicFrame);
    if (FAILED(hr)) return false;

    UINT imgW = 0, imgH = 0;
    hr = wicFrame->GetSize(&imgW, &imgH);
    if (FAILED(hr) || imgW == 0 || imgH == 0) return false;

    Microsoft::WRL::ComPtr<IWICFormatConverter> wicConverter;
    hr = wicFactory->CreateFormatConverter(&wicConverter);
    if (FAILED(hr)) return false;

    hr = wicConverter->Initialize(
        wicFrame.Get(),
        GUID_WICPixelFormat8bppGray,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeMedianCut
    );
    if (FAILED(hr)) return false;

    UINT stride = imgW;
    UINT bufferSize = stride * imgH;
    std::vector<BYTE> rawPixels(bufferSize);
    hr = wicConverter->CopyPixels(nullptr, stride, bufferSize, rawPixels.data());
    if (FAILED(hr)) return false;

    auto ClampF = [](float val, float minV, float maxV) -> float {
        return (val < minV) ? minV : ((val > maxV) ? maxV : val);
    };
    auto MinI = [](int a, int b) -> int {
        return (a < b) ? a : b;
    };

    // 3. Bilinear interpolation sampler for seamless, smooth elevation
    auto SampleHeightNorm = [&](float u, float v) -> float {
        u = ClampF(u, 0.0f, 1.0f);
        v = ClampF(v, 0.0f, 1.0f);

        float fx = u * (imgW - 1);
        float fy = v * (imgH - 1);

        int x0 = (int)fx;
        int y0 = (int)fy;
        int x1 = MinI(x0 + 1, (int)imgW - 1);
        int y1 = MinI(y0 + 1, (int)imgH - 1);

        float s = fx - x0;
        float t = fy - y0;

        float h00 = rawPixels[y0 * imgW + x0] / 255.0f;
        float h10 = rawPixels[y0 * imgW + x1] / 255.0f;
        float h01 = rawPixels[y1 * imgW + x0] / 255.0f;
        float h11 = rawPixels[y1 * imgW + x1] / 255.0f;

        float top = h00 * (1.0f - s) + h10 * s;
        float bot = h01 * (1.0f - s) + h11 * s;
        return top * (1.0f - t) + bot * t;
    };

    m_width = gridWidth;
    m_depth = gridDepth;
    m_cellSpacing = cellSpacing;

    int totalVertices = gridWidth * gridDepth;
    std::vector<TerrainVertex> vertices(totalVertices);

    float halfW = (gridWidth - 1) * cellSpacing * 0.5f;
    float halfD = (gridDepth - 1) * cellSpacing * 0.5f;

    float du = 1.0f / (gridWidth - 1);
    float dv = 1.0f / (gridDepth - 1);

    // 4. Generate terrain vertices with normals & altitude coloring
    for (int z = 0; z < gridDepth; ++z)
    {
        for (int x = 0; x < gridWidth; ++x)
        {
            int index = z * gridWidth + x;

            float u = static_cast<float>(x) / static_cast<float>(gridWidth - 1);
            float v = static_cast<float>(z) / static_cast<float>(gridDepth - 1);

            float posX = x * cellSpacing - halfW;
            float hNorm = SampleHeightNorm(u, v);
            float posY = (hNorm - 0.28f) * heightScale;
            float posZ = z * cellSpacing - halfD;

            vertices[index].position = DirectX::XMFLOAT3(posX, posY, posZ);

            // Central difference normal calculation
            float hL = (SampleHeightNorm(u - du, v) - 0.28f) * heightScale;
            float hR = (SampleHeightNorm(u + du, v) - 0.28f) * heightScale;
            float hD = (SampleHeightNorm(u, v - dv) - 0.28f) * heightScale;
            float hU = (SampleHeightNorm(u, v + dv) - 0.28f) * heightScale;

            DirectX::XMVECTOR normalVec = DirectX::XMVectorSet(
                (hL - hR) / (2.0f * cellSpacing),
                1.0f,
                (hD - hU) / (2.0f * cellSpacing),
                0.0f
            );
            normalVec = DirectX::XMVector3Normalize(normalVec);
            DirectX::XMStoreFloat3(&vertices[index].normal, normalVec);

            vertices[index].texCoord = DirectX::XMFLOAT2(u, v);

            // Slope & Altitude based coloring
            float slope = vertices[index].normal.y;
            DirectX::XMFLOAT4 vertexColor;

            if (posY < -2.0f)
            {
                // Riverbed / ravine: moist dark earth / slate
                vertexColor = DirectX::XMFLOAT4(0.32f, 0.38f, 0.33f, 1.0f);
            }
            else if (posY < 6.0f)
            {
                // Lush green meadow & valley floor
                float t = (posY - (-2.0f)) / 8.0f;
                vertexColor = DirectX::XMFLOAT4(
                    0.30f + 0.05f * t,
                    0.52f + 0.10f * t,
                    0.28f + 0.05f * t,
                    1.0f
                );
            }
            else if (posY < 16.0f)
            {
                // High altitude highlands & rocky slopes
                float t = (posY - 6.0f) / 10.0f;
                vertexColor = DirectX::XMFLOAT4(
                    0.35f * (1.0f - t) + 0.48f * t,
                    0.62f * (1.0f - t) + 0.44f * t,
                    0.33f * (1.0f - t) + 0.38f * t,
                    1.0f
                );
            }
            else
            {
                // Snow-capped peaks
                float t = ClampF((posY - 16.0f) / 9.0f, 0.0f, 1.0f);
                vertexColor = DirectX::XMFLOAT4(
                    0.48f * (1.0f - t) + 0.95f * t,
                    0.44f * (1.0f - t) + 0.96f * t,
                    0.38f * (1.0f - t) + 0.98f * t,
                    1.0f
                );
            }

            // Steep slope rock shading
            if (slope < 0.78f)
            {
                float rockBlend = ClampF((0.78f - slope) / 0.38f, 0.0f, 0.88f);
                DirectX::XMFLOAT4 rockColor(0.40f, 0.37f, 0.35f, 1.0f);
                vertexColor.x = vertexColor.x * (1.0f - rockBlend) + rockColor.x * rockBlend;
                vertexColor.y = vertexColor.y * (1.0f - rockBlend) + rockColor.y * rockBlend;
                vertexColor.z = vertexColor.z * (1.0f - rockBlend) + rockColor.z * rockBlend;
            }

            vertices[index].color = vertexColor;
        }
    }

    // 5. Generate indices
    int numQuads = (gridWidth - 1) * (gridDepth - 1);
    m_indexCount = numQuads * 6;
    std::vector<UINT> indices(m_indexCount);

    int idx = 0;
    for (int z = 0; z < gridDepth - 1; ++z)
    {
        for (int x = 0; x < gridWidth - 1; ++x)
        {
            UINT v0 = static_cast<UINT>(z * gridWidth + x);
            UINT v1 = static_cast<UINT>((z + 1) * gridWidth + x);
            UINT v2 = static_cast<UINT>((z + 1) * gridWidth + (x + 1));
            UINT v3 = static_cast<UINT>(z * gridWidth + (x + 1));

            indices[idx++] = v0;
            indices[idx++] = v1;
            indices[idx++] = v2;

            indices[idx++] = v0;
            indices[idx++] = v2;
            indices[idx++] = v3;
        }
    }

    // 6. Create D3D11 Buffers
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(TerrainVertex) * totalVertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vData = {};
    vData.pSysMem = vertices.data();

    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();

    hr = device->CreateBuffer(&vbd, &vData, &m_vertexBuffer);
    if (FAILED(hr)) return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(UINT) * m_indexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iData = {};
    iData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibd, &iData, &m_indexBuffer);
    if (FAILED(hr)) return false;

    return true;
}
