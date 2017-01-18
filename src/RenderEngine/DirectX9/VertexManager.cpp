#include "RenderEngine/DirectX9/Resources.h"
#include "RenderEngine/DirectX9/Exceptions.h"
using namespace std;

namespace Alamo {
namespace DirectX9 {

// These must be powers of two
static const LONG DEFAULT_VERTEXBUFFER_SIZE =  262144;
static const LONG DEFAULT_INDEXBUFFER_SIZE  = 1048576;
static const LONG MIN_VERTEXGROUP_SIZE      = 32;
static const LONG MIN_INDEXGROUP_SIZE       = 32;

VertexBuffer::~VertexBuffer()
{
    m_buffer->GetVertexManager().FreeVertexBuffer(*this);
}

IndexBuffer::~IndexBuffer()
{
    m_buffer->GetVertexManager().FreeIndexBuffer(*this);
}

VertexFormat VertexManager::GetVertexFormat(const std::string& name)
{
    // Search the list
    for (int i = 0; VertexFormatNames[i].name != NULL; i++)
    {
        if (_stricmp(VertexFormatNames[i].name, name.c_str()) == 0)
        {
            return VertexFormatNames[i].format;
        }
    }
    return VF_GENERIC;
}

void D3DVertexBuffer::StoreVertices(DWORD offset, const void* data, DWORD num, bool isUaW)
{
    HRESULT hRes;
    const VertexFormatInfo& vfi = VertexFormats[m_format];
    
    void* swdata, *hwdata;
    if (FAILED(hRes = m_pVertexBuffer_HW->Lock(offset * (DWORD)vfi.hw.size, num * (DWORD)vfi.hw.size, &hwdata, 0)))
    {
        throw DirectXException(hRes);
    }

    if (FAILED(hRes = m_pVertexBuffer_SW->Lock(offset * (DWORD)vfi.sw.size, num * (DWORD)vfi.sw.size, &swdata, (data == NULL) ? D3DLOCK_READONLY : 0)))
    {
        m_pVertexBuffer_HW->Unlock();
        throw DirectXException(hRes);
    }
    
    if (data != NULL) {
        // Initialize the SW vertex data
        vfi.sw.proc(swdata, data, num, isUaW);
    }

    if (vfi.hw.proc != NULL) {
        // Special conversion from SW to HW required
        vfi.hw.proc(hwdata, swdata, num, isUaW);
    } else {
        // SW and HW Vertex Formats are the same, copy
        assert(vfi.hw.size == vfi.sw.size);
        memcpy(hwdata, swdata, num * vfi.hw.size);
    }

    m_pVertexBuffer_SW->Unlock();
    m_pVertexBuffer_HW->Unlock();
}

void D3DVertexBuffer::OnLostDevice()
{
    SAFE_RELEASE(m_pVertexBuffer_HW);
}

void D3DVertexBuffer::OnResetDevice(bool isUaW)
{
    HRESULT hRes;
    const VertexFormatInfo& vfi = VertexFormats[m_format];

    SAFE_RELEASE(m_pVertexBuffer_HW);
    if (FAILED(hRes = m_manager.GetDevice()->CreateVertexBuffer(m_nVertices * (DWORD)vfi.hw.size, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_pVertexBuffer_HW, NULL)))
    {
        throw DirectXException(hRes);
    }
    StoreVertices(0, NULL, m_nVertices, isUaW);
}

void D3DVertexBuffer::SetAsSource(bool sw)
{
    if (sw) {
        m_manager.SetAsSource(m_pVertexBuffer_SW, m_pDeclaration_SW, (UINT)VertexFormats[m_format].sw.size);
    } else {
        m_manager.SetAsSource(m_pVertexBuffer_HW, m_pDeclaration_HW, (UINT)VertexFormats[m_format].hw.size);
    }
}

void D3DVertexBuffer::ReleaseDeclaration()
{
    VertexFormatInfo& vfi = VertexFormats[m_format];
    if (--vfi.nReferences == 0)
    {
        SAFE_RELEASE(vfi.hw.declaration);
        SAFE_RELEASE(vfi.sw.declaration);
    }
}

D3DVertexBuffer::D3DVertexBuffer(VertexManager& manager, VertexFormat format, DWORD nVertices)
    : m_manager(manager), m_format(format), m_nVertices(nVertices)
{
    // Set declarations
    VertexFormatInfo& vfi = VertexFormats[m_format];
    m_pDeclaration_HW = vfi.hw.declaration;
    m_pDeclaration_SW = vfi.sw.declaration;
    vfi.nReferences++;

    IDirect3DDevice9* pDevice = m_manager.GetDevice();

    // Create buffers
    HRESULT hRes;
    if (FAILED(hRes = pDevice->CreateVertexBuffer(nVertices * (DWORD)vfi.hw.size, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_pVertexBuffer_HW, NULL)))
    {
        throw DirectXException(hRes);
    }

    if (FAILED(hRes = pDevice->CreateVertexBuffer(nVertices * (DWORD)vfi.sw.size, D3DUSAGE_SOFTWAREPROCESSING, 0, D3DPOOL_SYSTEMMEM, &m_pVertexBuffer_SW, NULL)))
    {
        SAFE_RELEASE(m_pVertexBuffer_HW);
        throw DirectXException(hRes);
    }
}

D3DVertexBuffer::~D3DVertexBuffer()
{
    SAFE_RELEASE(m_pVertexBuffer_HW);
    SAFE_RELEASE(m_pVertexBuffer_SW);

    // Release the declaration
    ReleaseDeclaration();

    // Unlink from list
    *m_prev = m_next;
    if (m_next != NULL) {
        m_next->m_prev = m_prev;
    }
}

void D3DIndexBuffer::StoreIndices(DWORD offset, const uint16_t* data, DWORD num)
{
    HRESULT hRes;

    void* hwdata, *swdata;
    if (FAILED(hRes = m_pIndexBuffer_HW->Lock(offset * sizeof(uint16_t), num * sizeof(uint16_t), &hwdata, 0)))
    {
        throw DirectXException(hRes);
    }
    
    if (FAILED(hRes = m_pIndexBuffer_SW->Lock(offset * sizeof(uint16_t), num * sizeof(uint16_t), &swdata, (data == NULL) ? D3DLOCK_READONLY : 0)))
    {
        throw DirectXException(hRes);
    }

    if (data != NULL)
    {
        memcpy(swdata, data, num * sizeof(uint16_t));
    }
    memcpy(hwdata, swdata, num * sizeof(uint16_t));

    m_pIndexBuffer_SW->Unlock();
    m_pIndexBuffer_HW->Unlock();
}

void D3DIndexBuffer::SetAsSource(bool sw)
{
    m_manager.SetAsSource(sw ? m_pIndexBuffer_SW : m_pIndexBuffer_HW);
}

void D3DIndexBuffer::OnLostDevice()
{
    SAFE_RELEASE(m_pIndexBuffer_HW);
}

void D3DIndexBuffer::OnResetDevice()
{
    HRESULT hRes;
    if (FAILED(hRes = m_manager.GetDevice()->CreateIndexBuffer(m_nIndices * sizeof(uint16_t), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pIndexBuffer_HW, NULL)))
    {
        throw DirectXException(hRes);
    }
    StoreIndices(0, NULL, m_nIndices);
}

D3DIndexBuffer::D3DIndexBuffer(VertexManager& manager, DWORD nIndices)
    : m_manager(manager), m_nIndices(nIndices)
{
    IDirect3DDevice9* pDevice = m_manager.GetDevice();

    HRESULT hRes;
    if (FAILED(hRes = pDevice->CreateIndexBuffer(nIndices * sizeof(uint16_t), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pIndexBuffer_HW, NULL)))
    {
        throw DirectXException(hRes);
    }

    if (FAILED(hRes = pDevice->CreateIndexBuffer(nIndices * sizeof(uint16_t), D3DUSAGE_SOFTWAREPROCESSING, D3DFMT_INDEX16, D3DPOOL_SYSTEMMEM, &m_pIndexBuffer_SW, NULL)))
    {
        SAFE_RELEASE(m_pIndexBuffer_HW);
        throw DirectXException(hRes);
    }
}

D3DIndexBuffer::~D3DIndexBuffer()
{
    SAFE_RELEASE(m_pIndexBuffer_HW);
    SAFE_RELEASE(m_pIndexBuffer_SW);

    // Unlink from list
    *m_prev = m_next;
    if (m_next != NULL) {
        m_next->m_prev = m_prev;
    }
}

ptr<VertexBuffer> VertexManager::CreateVertexBuffer(VertexFormat format, const void* data, DWORD num)
{
    VertexFormatInfo& vfi  = VertexFormats[format];
    FreeVertexMap&    fmap = m_vertexBuffers[format];

    // Find a free chunk
    VertexRange range;
    FreeVertexMap::iterator p = fmap.lower_bound(num);
    if (p == fmap.end())
    {
        HRESULT hRes;

        // No free chunk, create a new vertex buffer
        // Get length, round up to nearest buffer size multiple;
        DWORD nVertices = (num + DEFAULT_VERTEXBUFFER_SIZE - 1) & -DEFAULT_VERTEXBUFFER_SIZE;

        if (vfi.nReferences == 0)
        {
            // Create the vertex declaration
            IDirect3DVertexDeclaration9* pDeclaration_HW;
            if (FAILED(hRes = m_pDevice->CreateVertexDeclaration(vfi.hw.elements, &pDeclaration_HW)))
            {
                throw DirectXException(hRes);
            }

            IDirect3DVertexDeclaration9* pDeclaration_SW;
            if (FAILED(hRes = m_pDevice->CreateVertexDeclaration(vfi.sw.elements, &pDeclaration_SW)))
            {
                SAFE_RELEASE(pDeclaration_HW);
                throw DirectXException(hRes);
            }
            vfi.hw.declaration = pDeclaration_HW;
            vfi.sw.declaration = pDeclaration_SW;
        }

        D3DVertexBuffer* pvb = new D3DVertexBuffer(*this, format, nVertices);
        pvb->m_next = m_firstVertexBuffer;
        pvb->m_prev = &m_firstVertexBuffer;
        if (pvb->m_next != NULL) {
            pvb->m_next->m_prev = &pvb->m_next;
        }
        m_firstVertexBuffer = pvb;

        range.m_buffer = pvb;
        range.m_start  = 0;
        range.m_count  = nVertices;
    }
    else
    {
        // Use the first free group on the free stack
        range = p->second.top();
        p->second.pop();
        if (p->second.empty())
        {
            fmap.erase(p);
        }
    }

    // Store vertices and compose result info
    ptr<VertexBuffer> buffer = new VertexBuffer(range);
    buffer->m_count = num;
    buffer->m_buffer->StoreVertices(buffer->m_start, data, num, m_isUaW);

    // Calculate new free group
    num = (num + MIN_VERTEXGROUP_SIZE - 1) & -MIN_VERTEXGROUP_SIZE;
    range.m_start += num;
    range.m_count -= num;
    if (range.m_count > 0)
    {
        // Store it
        fmap[range.m_count].push(range);
    }

    return buffer;
}

ptr<IndexBuffer> VertexManager::CreateIndexBuffer(const uint16_t *indices, DWORD numIndices)
{
    // Find a free chunk
    IndexRange range;
    FreeIndexMap::iterator p = m_indexBuffers.lower_bound(numIndices);
    if (p == m_indexBuffers.end())
    {
        // No free chunk, create a new index buffer
        // Get length, round up to nearest buffer size multiple;
        DWORD nIndices = (numIndices + DEFAULT_INDEXBUFFER_SIZE - 1) & -DEFAULT_INDEXBUFFER_SIZE;

        D3DIndexBuffer* pib = new D3DIndexBuffer(*this, nIndices);
        pib->m_next = m_firstIndexBuffer;
        pib->m_prev = &m_firstIndexBuffer;
        if (pib->m_next != NULL) {
            pib->m_next->m_prev = &pib->m_next;
        }
        m_firstIndexBuffer = pib;

        range.m_buffer = pib;
        range.m_start  = 0;
        range.m_count  = nIndices;
    }
    else
    {
        // Use the first free group on the free stack
        range = p->second.top();
        p->second.pop();
        if (p->second.size() == 0)
        {
            m_indexBuffers.erase(p);
        }
    }

    // Store vertices and compose result info
    ptr<IndexBuffer> buffer = new IndexBuffer(range);
    buffer->m_count = numIndices;
    buffer->m_buffer->StoreIndices(buffer->m_start, indices, numIndices);

    // Calculate new free group
    numIndices = (numIndices + MIN_INDEXGROUP_SIZE - 1) & -MIN_INDEXGROUP_SIZE;
    range.m_start += numIndices;
    range.m_count -= numIndices;
    if (range.m_count > 0)
    {
        // Store it
        m_indexBuffers[range.m_count].push(range);
    }

    return buffer;
}

void VertexManager::FreeVertexBuffer(const VertexBuffer& buffer)
{
    FreeVertexMap& fmap = m_vertexBuffers[buffer.m_buffer->m_format];
    fmap[buffer.m_count].push(buffer);
}

void VertexManager::FreeIndexBuffer(const IndexBuffer& buffer)
{
    m_indexBuffers[buffer.m_count].push(buffer);
}

void VertexManager::SetAsSource(IDirect3DVertexBuffer9* pVertexBuffer, IDirect3DVertexDeclaration9* pDeclaration, UINT size)
{
    if (m_activeDeclaration != pDeclaration)
    {
        m_pDevice->SetVertexDeclaration(pDeclaration);
        m_activeDeclaration = pDeclaration;
    }

    if (m_activeVertexBuffer != pVertexBuffer)
    {
        m_pDevice->SetStreamSource(0, pVertexBuffer, 0, size);
        m_activeVertexBuffer = pVertexBuffer;
    }
}

void VertexManager::SetAsSource(IDirect3DIndexBuffer9* pIndexBuffer)
{
    if (m_activeIndexBuffer != pIndexBuffer)
    {
        m_pDevice->SetIndices(pIndexBuffer);
        m_activeIndexBuffer = pIndexBuffer;
    }
}

void VertexManager::ResetActiveStreams()
{
    m_activeVertexBuffer = NULL;
    m_activeIndexBuffer  = NULL;
    m_activeDeclaration  = NULL;
}

void VertexManager::OnLostDevice()
{   
    ResetActiveStreams();
    for (D3DVertexBuffer* cur = m_firstVertexBuffer; cur != NULL; cur = cur->m_next) cur->OnLostDevice();
    for (D3DIndexBuffer*  cur = m_firstIndexBuffer;  cur != NULL; cur = cur->m_next) cur->OnLostDevice();
}

void VertexManager::OnResetDevice()
{
    for (D3DVertexBuffer* cur = m_firstVertexBuffer; cur != NULL; cur = cur->m_next) cur->OnResetDevice(m_isUaW);
    for (D3DIndexBuffer*  cur = m_firstIndexBuffer;  cur != NULL; cur = cur->m_next) cur->OnResetDevice();
}

VertexManager::VertexManager(IDirect3DDevice9* pDevice, bool isUaW)
    : m_isUaW(isUaW)
{
    m_firstVertexBuffer  = NULL;
    m_firstIndexBuffer   = NULL;
    m_activeVertexBuffer = NULL;
    m_activeIndexBuffer  = NULL;
    m_activeDeclaration  = NULL;

    m_pDevice = pDevice;
    m_pDevice->AddRef();

    // Get the maximum vertex index
    D3DCAPS9 caps;
    pDevice->GetDeviceCaps(&caps);
    m_maxVertexIndex = caps.MaxVertexIndex;
}

VertexManager::~VertexManager()
{
    SAFE_RELEASE(m_pDevice);
}

} }