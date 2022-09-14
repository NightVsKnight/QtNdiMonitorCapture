#pragma once

#include "pch.h"
#include "SafeQueue.h"

using namespace std;
using namespace winrt;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::Graphics;

class SimpleCapture
{
public:
    SimpleCapture();
    ~SimpleCapture();

    bool IsCursorEnabled();
    void IsCursorEnabled(bool value);
    //bool IsBorderRequired();
    //void IsBorderRequired(bool value);

    void StartCapture(
            GraphicsCaptureItem const& item,
            DirectXPixelFormat pixelFormat,
            size_t pixelSizeBytes,
            uint frameBufferCount,
            function<bool(SimpleCapture *pSender, Direct3D11CaptureFrame frame)> callbackFrame,
            function<void(SimpleCapture *pSender, uint width, uint height, uint strideBytes, void *pFrameBuffer)> callbackFrameBytes);
    void Close();

private:
    void OnFrameArrived(
            Direct3D11CaptureFramePool const& sender,
            winrt::Windows::Foundation::IInspectable const& args);
    void WorkerThreadStart();
    void WorkerThreadStop();
    void WorkerProcessFrame(Direct3D11CaptureFrame frame);

private:
    GraphicsCaptureItem m_graphicsCaptureItem;
    DirectXPixelFormat m_pixelFormat;
    size_t m_pixelSizeBytes;
    uint m_frameBufferCount;
    function<bool(SimpleCapture*,Direct3D11CaptureFrame)> m_pCallbackProcessFrame;
    function<void(SimpleCapture*,uint,uint,uint,void*)> m_pCallbackProcessFrameBytes;

    SizeInt32 m_frameSize;

    IDirect3DDevice m_d3dDevice;
    com_ptr<ID3D11Device> m_d3d11Device;
    com_ptr<ID3D11DeviceContext> m_d3d11DeviceContext;
    Direct3D11CaptureFramePool m_captureFramePool;
    GraphicsCaptureSession m_graphicsCaptureSession;

    atomic<bool> m_closed;

    thread *m_pWorkerThread;
    SafeQueue<Direct3D11CaptureFrame> m_queueVideoFrames;
};
