#pragma once

#include "pch.h"

using namespace std;
using namespace winrt;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics;

class SimpleCapture
{
public:
    typedef bool (*CALLBACK_ON_FRAME)(void* pObj, SimpleCapture* pSender, Direct3D11CaptureFrame const& frame);
    typedef void (*CALLBACK_ON_FRAME_BUFFER)(void* pObj, SimpleCapture* pSender, int width, int height, int strideBytes, void* pFrameBuffer);

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
            int frameBufferCount,
            void* pObj,
            CALLBACK_ON_FRAME callbackFrame,
            CALLBACK_ON_FRAME_BUFFER callbackFrameBytes);
    void Close();

private:
    void OnFrameArrived(
            Direct3D11CaptureFramePool const& sender,
            winrt::Windows::Foundation::IInspectable const& args);
    void WorkerThreadStart();
    void WorkerThreadStop();
    void WorkerProcessFrame(Direct3D11CaptureFrame const& frame);

private:
    GraphicsCaptureItem m_graphicsCaptureItem;
    DirectXPixelFormat m_pixelFormat;
    size_t m_pixelSizeBytes;
    int m_frameBufferCount;
    void* m_pObj;
    CALLBACK_ON_FRAME m_pCallbackProcessFrame;
    CALLBACK_ON_FRAME_BUFFER m_pCallbackProcessFrameBytes;

    SizeInt32 m_frameSize;
    com_ptr<ID3D11Texture2D> m_frameTexture;

    IDirect3DDevice m_d3dDevice;
    com_ptr<ID3D11Device> m_d3d11Device;
    com_ptr<ID3D11DeviceContext> m_d3d11DeviceContext;
    Direct3D11CaptureFramePool m_captureFramePool;
    Direct3D11CaptureFramePool::FrameArrived_revoker m_frameArrivedRevoker;
    GraphicsCaptureSession m_graphicsCaptureSession;

    bool m_isCapturing;

    CRITICAL_SECTION m_lock;
};
