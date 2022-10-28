//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THE SOFTWARE IS PROVIDED �AS IS? WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*********************************************************

//#include "pch.h"
#include "SimpleCapture.h"

#include <QDebug>

using namespace Windows::Graphics::DirectX;

SimpleCapture::SimpleCapture()
    : m_graphicsCaptureItem{ nullptr }
    , m_pixelFormat{ DirectXPixelFormat::Unknown }
    , m_pixelSizeBytes{ 0 }
    , m_frameBufferCount{ 0 }
    , m_pObj{ nullptr }
    , m_pCallbackProcessFrame{ nullptr }
    , m_pCallbackProcessFrameBytes{ nullptr }
    , m_frameSize{ 0,  0 }
    , m_frameTexture{ nullptr }
    , m_d3dDevice{ nullptr }
    , m_d3d11Device{ nullptr }
    , m_d3d11DeviceContext{ nullptr }
    , m_captureFramePool{ nullptr }
    , m_frameArrivedRevoker{ }
    , m_graphicsCaptureSession{ nullptr }
    , m_isCapturing{ false }
{
    InitializeCriticalSection(&m_lock);
}

SimpleCapture::~SimpleCapture()
{
    Close();
}

bool SimpleCapture::IsCursorEnabled()
{
    return m_graphicsCaptureSession ? m_graphicsCaptureSession.IsCursorCaptureEnabled() : false;
}

void SimpleCapture::IsCursorEnabled(bool value)
{
    if (m_graphicsCaptureSession) m_graphicsCaptureSession.IsCursorCaptureEnabled(value);

}

//bool SimpleCapture::IsBorderRequired() {return m_graphicsCaptureSession.IsBorderRequired(); }
//void SimpleCapture::IsBorderRequired(bool value) { m_graphicsCaptureSession.IsBorderRequired(value); }

void SimpleCapture::StartCapture(
        GraphicsCaptureItem const& item,
        DirectXPixelFormat pixelFormat,
        size_t pixelSizeBytes,
        int frameBufferCount,
        void* pObj,
        CALLBACK_ON_FRAME callbackProcessFrame,
        CALLBACK_ON_FRAME_BUFFER callbackProcessFrameBytes)
{
    EnterCriticalSection(&m_lock);

    Close();

    m_graphicsCaptureItem = item;
    m_pixelFormat = pixelFormat;
    m_pixelSizeBytes = pixelSizeBytes;
    m_frameBufferCount = frameBufferCount;
    m_pObj = pObj;
    m_pCallbackProcessFrame = callbackProcessFrame;
    m_pCallbackProcessFrameBytes = callbackProcessFrameBytes;

    m_frameSize = m_graphicsCaptureItem.Size();

    auto d3dDevice = CreateD3DDevice();
    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    m_d3dDevice = CreateDirect3DDevice(dxgiDevice.get());
    m_d3d11Device = GetDXGIInterfaceFromObject<ID3D11Device>(m_d3dDevice);
    m_d3d11Device->GetImmediateContext(m_d3d11DeviceContext.put());

    m_captureFramePool = Direct3D11CaptureFramePool::CreateFreeThreaded(
        m_d3dDevice,
        m_pixelFormat,
        m_frameBufferCount,
        m_frameSize);

    m_frameArrivedRevoker = m_captureFramePool.FrameArrived(auto_revoke, { this, &SimpleCapture::OnFrameArrived });

    m_graphicsCaptureSession = m_captureFramePool.CreateCaptureSession(m_graphicsCaptureItem);

    m_graphicsCaptureSession.StartCapture();

    m_isCapturing = true;

    LeaveCriticalSection(&m_lock);
}

void SimpleCapture::Close()
{
    qDebug() << "+Close()";

    EnterCriticalSection(&m_lock);

    if (!m_isCapturing)
    {
        qDebug() << "already closed; ignoring";
        goto cleanup;
    }

    m_isCapturing = false;

    //
    // Release the objects in reverse order created in StartCapture
    //
    if (m_graphicsCaptureSession)
    {
        m_graphicsCaptureSession.Close();
        m_graphicsCaptureSession = nullptr;
    }
    if (m_frameArrivedRevoker)
    {
        m_frameArrivedRevoker.revoke();
        //m_frameArrivedRevoker = nullptr;
    }
    if (m_captureFramePool)
    {
        m_captureFramePool.Close();
        m_captureFramePool = nullptr;
    }

    m_d3d11DeviceContext.detach();
    m_d3d11Device.detach();
    m_d3dDevice = nullptr;
    m_frameTexture.detach();
    m_frameSize = { 0, 0 };
    m_pCallbackProcessFrameBytes = nullptr;
    if (m_pCallbackProcessFrame)
    {
        m_pCallbackProcessFrame(m_pObj, this, nullptr);
        m_pCallbackProcessFrame = nullptr;
    }
    m_pObj = nullptr;
    m_frameBufferCount = 0;
    m_pixelSizeBytes = 0;
    m_pixelFormat = DirectXPixelFormat::Unknown;
    m_graphicsCaptureItem = nullptr;

cleanup:
    LeaveCriticalSection(&m_lock);
    qDebug() << "-Close()";
}

void SimpleCapture::OnFrameArrived(
    Direct3D11CaptureFramePool const& sender,
    winrt::Windows::Foundation::IInspectable const&)
{
    //qDebug() << "+OnFrameArrived(...)";
    EnterCriticalSection(&m_lock);
    if (m_isCapturing)
    {
        const auto frame = sender.TryGetNextFrame();
        if (frame)
        {
            if (m_pCallbackProcessFrame)
            {
                if (!m_pCallbackProcessFrame(m_pObj, this, frame))
                {
                    goto cleanup;
                }
            }
            WorkerProcessFrame(frame);
        }
    }
cleanup:
    LeaveCriticalSection(&m_lock);
    //qDebug() << "-OnFrameArrived(...)";
}

/// <summary>
/// Copy frame and then send to callback
/// </summary>
/// <param name="pNdiSend"></param>
/// <param name="frame"></param>
void SimpleCapture::WorkerProcessFrame(Direct3D11CaptureFrame const& frame)
{
    if (!frame) return;

    const auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    if (!frameSurface) return;

    const auto frameSize = frame.ContentSize();

    bool resized = false;

    HRESULT hr = S_OK;

    if (!m_frameTexture)
    {
        // First time; create the texture
        m_frameSize = frameSize;
        D3D11_TEXTURE2D_DESC frameSurfaceDesc;
        frameSurface->GetDesc(&frameSurfaceDesc);
        frameSurfaceDesc.Usage = D3D11_USAGE_STAGING;//D3D11_USAGE_DEFAULT;//D3D11_USAGE_IMMUTABLE;//D3D11_USAGE_DYNAMIC;
        frameSurfaceDesc.BindFlags = 0;// D3D11_BIND_SHADER_RESOURCE;
        frameSurfaceDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        frameSurfaceDesc.MiscFlags = 0;
        frameSurfaceDesc.MipLevels = 1;
        frameSurfaceDesc.ArraySize = 1;
        frameSurfaceDesc.SampleDesc = { 1, 0 };
        hr = m_d3d11Device->CreateTexture2D(&frameSurfaceDesc, nullptr, m_frameTexture.put());
    }
    else
    {
        // Not first time; re-create the texture if dimensions changed since last time
        if (m_frameSize.Width != frameSize.Width || m_frameSize.Height != frameSize.Height)
        {
            resized = true;
            m_frameSize = frameSize;
            m_frameTexture.detach();
            D3D11_TEXTURE2D_DESC frameSurfaceDesc;
            frameSurface->GetDesc(&frameSurfaceDesc);
            frameSurfaceDesc.Width = m_frameSize.Width;
            frameSurfaceDesc.Height = m_frameSize.Height;
            hr = m_d3d11Device->CreateTexture2D(&frameSurfaceDesc, nullptr, m_frameTexture.put());
        }
    }

    if (SUCCEEDED(hr))
    {
        const auto pTexture = m_frameTexture.get();
        m_d3d11DeviceContext->CopyResource(pTexture, frameSurface.get());

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = m_d3d11DeviceContext->Map(pTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (SUCCEEDED(hr))
        {
            if (m_pCallbackProcessFrameBytes)
            {
                m_pCallbackProcessFrameBytes(m_pObj,
                                             this,
                                             frameSize.Width,
                                             frameSize.Height,
                                             mappedResource.RowPitch,
                                             mappedResource.pData);
            }
            m_d3d11DeviceContext->Unmap(pTexture, 0);
        }
    }

    if (resized)
    {
        m_captureFramePool.Recreate(m_d3dDevice,
                                    m_pixelFormat,
                                    m_frameBufferCount,
                                    m_frameSize);
    }
}
