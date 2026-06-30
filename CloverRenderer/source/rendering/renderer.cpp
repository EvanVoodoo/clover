#include "rendering/renderer.hpp"
#include <random>

using namespace clvr;

Renderer::Renderer()
{
	m_DX2D = nullptr;
}

Renderer::Renderer(const Renderer& other) {

}

Renderer::~Renderer() {

}

bool Renderer::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	bool result;


	// Create and initialize the Direct3D object.
	m_DX2D = new DirectX2D;

	result = m_DX2D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	return true;
}

void Renderer::Shutdown()
{
	// Release the Direct3D object.
	if (m_DX2D)
	{
		m_DX2D->Shutdown();
		delete m_DX2D;
		m_DX2D = nullptr;
	}
	return;
}

bool Renderer::Frame(float dt)
{
	bool result;

	// Render the graphics scene.
	result = Render(dt);
	if (!result)
	{
		return false;
	}

	return true;
}

void Renderer::DrawSprite(const Sprite& sprite) { m_DX2D->DrawSprite(sprite); }

bool Renderer::Render(float dt)
{
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_real_distribution<float> xPosDist(-800.0f, 800.0f);
    static std::uniform_real_distribution<float> yPosDist(-400.0f, 400.0f);
    static std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
    static std::uniform_real_distribution<float> sizeDist(100.0f, 400.0f);

    static const wchar_t* textures[] = {
    L"assets/textures/shrew1.jpg",
    L"assets/textures/shrew2.jpg",
    L"assets/textures/hamper.jpeg"
    };

    static std::uniform_int_distribution<int> texDist(0, (int)(std::size(textures)) - 1);

    static std::uniform_int_distribution<unsigned int> layerDist(0, 5);
    
    static Sprite randomSprites[10];
    static float elapsed = 0.0f;
    static bool initialized = false;

    if (!initialized)
    {
        for (auto& sprite : randomSprites)
        {
            sprite.position = XMFLOAT2(xPosDist(rng), yPosDist(rng));
            float size = sizeDist(rng);
            sprite.size = XMFLOAT2(size, size);
            //sprite.color = XMFLOAT4(colorDist(rng), colorDist(rng), colorDist(rng), 1.0f);
            sprite.uvRect = m_DX2D->GetAtlasRegion(textures[texDist(rng)]).uvRect;
            sprite.layer = layerDist(rng);
        }
        initialized = true;
    }

    elapsed += dt;
    if (elapsed >= 1.f)
    {
        for (auto& sprite : randomSprites)
        {
            sprite.position = XMFLOAT2(xPosDist(rng), yPosDist(rng));
            //sprite.color = XMFLOAT4(colorDist(rng), colorDist(rng), colorDist(rng), 1.0f);
        }
        elapsed = 0.0f;
    }

    m_DX2D->BeginScene(0.1f, 0.1f, 0.1f, 1.0f);

    // Center sprite
    Sprite center;
    center.position = XMFLOAT2(0.0f, 0.0f);
    center.size = XMFLOAT2(600.0f, 600.0f);
    center.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    center.uvRect = m_DX2D->GetAtlasRegion(L"assets/textures/shrew1.jpg").uvRect;
	center.layer = 0; // Ensure it's at the back

	std::vector<Sprite> spritesToDraw = { center };
	spritesToDraw.insert(spritesToDraw.end(), std::begin(randomSprites), std::end(randomSprites));

	// sort sprites by layer before drawing
	std::stable_sort(spritesToDraw.begin(), spritesToDraw.end(), [](const Sprite& a, const Sprite& b) {
		return a.layer < b.layer;
		});

    for (const auto& sprite : spritesToDraw)
        DrawSprite(sprite);

    m_DX2D->EndScene();

    return true;
}