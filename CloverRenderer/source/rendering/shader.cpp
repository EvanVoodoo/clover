#include "rendering/shader.hpp"

using namespace clvr;

Shader::Shader()
{
	m_vertexShader = nullptr;
	m_pixelShader = nullptr;
	m_layout = nullptr;
}

Shader::Shader(const Shader& other)
{
}

Shader::~Shader()
{
}

bool Shader::Initialize(ID3D11Device* device, HWND hwnd, const wchar_t* vs, const wchar_t* ps)
{
	m_vsFilename = vs;
	m_psFilename = ps;

	return InitializeShader(device, hwnd, vs, ps);
}

void Shader::Shutdown()
{
	// Shutdown the vertex and pixel shaders as well as the related objects.
	ShutdownShader();
	return;
}

bool Shader::Bind(ID3D11DeviceContext* deviceContext)
{
	deviceContext->IASetInputLayout(m_layout);
	deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
	deviceContext->PSSetShader(m_pixelShader, nullptr, 0);

	return true;
}

bool Shader::Reload(ID3D11Device* device, HWND hwnd)
{
	ID3D11VertexShader* newVS = nullptr;
	ID3D11PixelShader* newPS = nullptr;
	ID3D11InputLayout* newLayout = nullptr;

	// Try to compile into new resources without touching member variables
	if (!InitializeShaderInto(device, hwnd, m_vsFilename.c_str(), m_psFilename.c_str(),
		&newVS, &newPS, &newLayout))
	{
		if (newVS)     newVS->Release();
		if (newPS)     newPS->Release();
		if (newLayout) newLayout->Release();
		return false;
	}

	// Success — release old and swap in new
	if (m_vertexShader) m_vertexShader->Release();
	if (m_pixelShader)  m_pixelShader->Release();
	if (m_layout)       m_layout->Release();

	m_vertexShader = newVS;
	m_pixelShader = newPS;
	m_layout = newLayout;

	return true;
}

bool Shader::InitializeShader(ID3D11Device* device, HWND hwnd, const wchar_t* vsFilename, const wchar_t* psFilename)
{
	return InitializeShaderInto(device, hwnd, vsFilename, psFilename,
		&m_vertexShader, &m_pixelShader, &m_layout);
}

bool Shader::InitializeShaderInto(ID3D11Device* device, HWND hwnd, const wchar_t* vsFilename, const wchar_t* psFilename,
	ID3D11VertexShader** outVS, ID3D11PixelShader** outPS, ID3D11InputLayout** outLayout)
{
	HRESULT result;
	ID3D10Blob* errorMessage = nullptr;
	ID3D10Blob* vertexShaderBuffer = nullptr;
	ID3D10Blob* pixelShaderBuffer = nullptr;

	result = D3DCompileFromFile(vsFilename, NULL, NULL, "ColorVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&vertexShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		if (errorMessage) { OutputShaderErrorMessage(errorMessage, hwnd, vsFilename); errorMessage = nullptr; }
		else MessageBox(hwnd, vsFilename, L"Missing Shader File", MB_OK);
		return false;
	}

	if (errorMessage) { errorMessage->Release(); errorMessage = nullptr; }

	result = D3DCompileFromFile(psFilename, NULL, NULL, "ColorPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		if (errorMessage) { OutputShaderErrorMessage(errorMessage, hwnd, psFilename); errorMessage = nullptr; }
		else MessageBox(hwnd, psFilename, L"Missing Shader File", MB_OK);
		vertexShaderBuffer->Release();
		return false;
	}

	if (errorMessage) { errorMessage->Release(); errorMessage = nullptr; }

	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, outVS);
	if (FAILED(result)) { vertexShaderBuffer->Release(); pixelShaderBuffer->Release(); return false; }

	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, outPS);
	if (FAILED(result)) { (*outVS)->Release(); vertexShaderBuffer->Release(); pixelShaderBuffer->Release(); return false; }

	D3D11_INPUT_ELEMENT_DESC polygonLayout[3];
	polygonLayout[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 };
	polygonLayout[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	polygonLayout[2] = { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	result = device->CreateInputLayout(polygonLayout, 3, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), outLayout);
	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();
	if (FAILED(result)) { (*outVS)->Release(); (*outPS)->Release(); return false; }

	return true;
}

void Shader::ShutdownShader()
{
	// Release the layout.
	if (m_layout)
	{
		m_layout->Release();
		m_layout = nullptr;
	}
	// Release the pixel shader.
	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}
	// Release the vertex shader.
	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}
	return;
}

void Shader::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, const wchar_t* shaderFilename, bool silent)
{
	char* compileErrors = (char*)errorMessage->GetBufferPointer();
	unsigned long bufferSize = static_cast<unsigned long>(errorMessage->GetBufferSize());

	std::ofstream fout("shader-error.txt");
	for (unsigned long i = 0; i < bufferSize; i++)
		fout << compileErrors[i];
	fout.close();

	errorMessage->Release();

	if (!silent)
		MessageBox(hwnd, L"Error compiling shader. Check shader-error.txt for message.", shaderFilename, MB_OK);
	else
		OutputDebugStringW(L"Shader compile error — see shader-error.txt\n");
}