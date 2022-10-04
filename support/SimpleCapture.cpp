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
    , m_closed{ false }
    , m_pWorkerThread{ nullptr }
{
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
        uint frameBufferCount,
        void* pObj,
        CALLBACK_ON_FRAME callbackProcessFrame,
        CALLBACK_ON_FRAME_BUFFER callbackProcessFrameBytes)
{
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

    m_captureFramePool = Direct3D11CaptureFramePool::Create(
        m_d3dDevice,
        pixelFormat,
        frameBufferCount,
        m_frameSize);

    m_frameArrivedRevoker = m_captureFramePool.FrameArrived(auto_revoke, { this, &SimpleCapture::OnFrameArrived });

    // NOTE: In case you were wondering and/or worried, the yellow screen border visible on the local capture side is
    // **NOT** visible in the output frames; they are probably capturing the frame before they overlay the yellow border.
    m_graphicsCaptureSession = m_captureFramePool.CreateCaptureSession(m_graphicsCaptureItem);

    m_graphicsCaptureSession.StartCapture();

    WorkerThreadStart();

    m_closed = false;
}

void SimpleCapture::Close()
{
    auto expected = false;
    if (!m_closed.compare_exchange_strong(expected, true)) return;

    //
    // Release the objects in reverse order created in StartCapture
    //
    WorkerThreadStop();
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
    m_d3d11DeviceContext = nullptr;
    m_d3d11Device = nullptr;
    m_d3dDevice = nullptr;
    m_frameTexture = nullptr;
    m_frameSize = { 0, 0 };
    m_pCallbackProcessFrameBytes = nullptr;
    m_pCallbackProcessFrame = nullptr;
    m_pObj = nullptr;
    m_frameBufferCount = 0;
    m_pixelSizeBytes = 0;
    m_pixelFormat = DirectXPixelFormat::Unknown;
    m_graphicsCaptureItem = nullptr;
}

void SimpleCapture::WorkerThreadStart()
{
    WorkerThreadStop();
    m_pWorkerThread = new thread([this]()
    {
        qDebug() << "+m_pWorkerThread";
        qDebug() << "m_pWorkerThread main loop begin";
        while (true)
        {
            const auto videoFrame = m_queueVideoFrames.dequeue();
            if (videoFrame)
            {
                if (m_pCallbackProcessFrame)
                {
                    if (!m_pCallbackProcessFrame(m_pObj, this, videoFrame))
                    {
                        continue;
                    }
                }
                WorkerProcessFrame(videoFrame);
            }
            else
            {
                break;
            }
        }
        qDebug() << "m_pWorkerThread main loop end";
        if (m_pCallbackProcessFrame)
        {
            m_pCallbackProcessFrame(m_pObj, this, nullptr);
        }
        qDebug() << "-m_pWorkerThread";
    });
}

void SimpleCapture::WorkerThreadStop()
{
    if (m_pWorkerThread)
    {
        m_queueVideoFrames.interrupt();

        m_pWorkerThread->join();
        delete m_pWorkerThread;
        m_pWorkerThread = nullptr;

        m_queueVideoFrames.clear();
    }
}

void SimpleCapture::OnFrameArrived(
    Direct3D11CaptureFramePool const& sender,
    winrt::Windows::Foundation::IInspectable const&)
{
    if (m_closed) return;

    const auto frame = sender.TryGetNextFrame();
    if (!frame) return;

#if 0
    // Just calling the first few lines of WorkerProcessFrame seems to bog down the UI.
    WorkerProcessFrame(frame);
#else
    // Enqueue to a separate thread for processing.
    m_queueVideoFrames.enqueue(frame);
#endif
}

/// <summary>
/// Copy frame and then send to callback
/// </summary>
/// <param name="pNdiSend"></param>
/// <param name="frame"></param>
void SimpleCapture::WorkerProcessFrame(Direct3D11CaptureFrame const& frame)
{
    if (!frame) return;

    const auto frameSurface2 = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    if (!frameSurface2) return;

    const auto frameSize = frame.ContentSize();
    bool resized = false;

    HRESULT hr;

    if (!m_frameTexture)
    {
        D3D11_TEXTURE2D_DESC frameSurfaceDesc;
        frameSurface2->GetDesc(&frameSurfaceDesc);
        frameSurfaceDesc.Usage = D3D11_USAGE_STAGING;//D3D11_USAGE_DEFAULT;//D3D11_USAGE_IMMUTABLE;//D3D11_USAGE_DYNAMIC;
        frameSurfaceDesc.BindFlags = 0;// D3D11_BIND_SHADER_RESOURCE;
        frameSurfaceDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        frameSurfaceDesc.MiscFlags = 0;
        frameSurfaceDesc.MipLevels = 1;
        frameSurfaceDesc.ArraySize = 1;
        frameSurfaceDesc.SampleDesc = { 1, 0 };
        hr = m_d3d11Device->CreateTexture2D(&frameSurfaceDesc, nullptr, m_frameTexture.put());
        if (FAILED(hr)) return;
    }
    else
    {
        if (m_frameSize.Width != frameSize.Width || m_frameSize.Height != frameSize.Height)
        {
            resized = true;
            D3D11_TEXTURE2D_DESC frameSurfaceDesc;
            frameSurface2->GetDesc(&frameSurfaceDesc);
            frameSurfaceDesc.Width = frameSize.Width;
            frameSurfaceDesc.Height = frameSize.Height;
            hr = m_d3d11Device->CreateTexture2D(&frameSurfaceDesc, nullptr, m_frameTexture.put());
            if (FAILED(hr)) return;
        }
    }

    const auto pTexture = m_frameTexture.get();
    m_d3d11DeviceContext->CopyResource(pTexture, frameSurface2.get());


    D3D11_MAPPED_SUBRESOURCE mappedResource;
    //ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
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

    if (resized)
    {
        m_frameSize = frameSize;
        m_captureFramePool.Recreate(m_d3dDevice,
                                    m_pixelFormat,
                                    m_frameBufferCount,
                                    m_frameSize);
    }
}
