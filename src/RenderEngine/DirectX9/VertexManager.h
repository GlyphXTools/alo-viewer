#ifndef VERTEXBUFFERMANAGER_DX9_H
#define VERTEXBUFFERMANAGER_DX9_H

#include "RenderEngine/DirectX9/VertexFormats.h"
#include "General/Objects.h"

#include <stack>
#include <map>

namespace Alamo {
namespace DirectX9 {

class VertexManager;

class D3DVertexBuffer : public IObject
{
    friend class VertexManager;

    D3DVertexBuffer             *m_next, **m_prev;
    IDirect3DVertexBuffer9*      m_pVertexBuffer_HW;
    IDirect3DVertexBuffer9*      m_pVertexBuffer_SW;
    IDirect3DVertexDeclaration9* m_pDeclaration_HW;
    IDirect3DVertexDeclaration9* m_pDeclaration_SW;
    VertexFormat                 m_format;
    VertexManager&               m_manager;
    DWORD                        m_nVertices;

    void StoreVertices(DWORD offset, const void* data, DWORD length, bool isUaW);
    void OnLostDevice();
    void OnResetDevice(bool isUaW);
    void ReleaseDeclaration();

    D3DVertexBuffer(VertexManager& manager, VertexFormat format, DWORD nVertices);
    ~D3DVertexBuffer();
public:
    VertexManager& GetVertexManager() const { return m_manager; }
    void SetAsSource(bool sw = false);
};

class D3DIndexBuffer : public IObject
{
    friend class VertexManager;

    D3DIndexBuffer        *m_next, **m_prev;
    IDirect3DIndexBuffer9* m_pIndexBuffer_HW;
    IDirect3DIndexBuffer9* m_pIndexBuffer_SW;
    VertexManager&         m_manager;
    DWORD                  m_nIndices;

    void StoreIndices(DWORD offset, const uint16_t* data, DWORD num);
    void OnLostDevice();
    void OnResetDevice();

    D3DIndexBuffer(VertexManager& manager, DWORD nIndices);
    ~D3DIndexBuffer();
public:
    VertexManager& GetVertexManager() const { return m_manager; }
    void SetAsSource(bool sw = false);
};

struct VertexRange
{
    ptr<D3DVertexBuffer> m_buffer;
    DWORD                m_start;
    DWORD                m_count;
};

struct VertexBuffer : public IObject, public VertexRange
{
    VertexBuffer(const VertexRange& range) : VertexRange(range) {}
    ~VertexBuffer();
};

struct IndexRange
{
    ptr<D3DIndexBuffer> m_buffer;
    DWORD               m_start;
    DWORD               m_count;
};

struct IndexBuffer : public IObject, public IndexRange
{
    IndexBuffer(const IndexRange& range) : IndexRange(range) {}
    ~IndexBuffer();
};

class VertexManager : public IObject
{
    friend struct VertexBuffer;
    friend struct IndexBuffer;
    friend class D3DVertexBuffer;
    friend class D3DIndexBuffer;

    typedef std::stack<VertexRange>               FreeVertexList;
    typedef std::map<DWORD, FreeVertexList>       FreeVertexMap;
    typedef std::map<VertexFormat, FreeVertexMap> FormatMap;
    typedef std::stack<IndexRange>                FreeIndexList;
    typedef std::map<DWORD, FreeIndexList>        FreeIndexMap;

    IDirect3DDevice9* m_pDevice;
    bool              m_isUaW;
    DWORD             m_maxVertexIndex;
    DWORD             m_maxPrimitiveCount;
    FormatMap         m_vertexBuffers;
    FreeIndexMap      m_indexBuffers;

    D3DVertexBuffer*             m_firstVertexBuffer;
    D3DIndexBuffer*              m_firstIndexBuffer;

    IDirect3DVertexBuffer9*      m_activeVertexBuffer;
    IDirect3DVertexDeclaration9* m_activeDeclaration;
    IDirect3DIndexBuffer9*       m_activeIndexBuffer;

    void SetAsSource(IDirect3DVertexBuffer9* pVertexBuffer, IDirect3DVertexDeclaration9* pDeclaration, UINT size);
    void SetAsSource(IDirect3DIndexBuffer9* pIndexBuffer);
    IDirect3DDevice9* GetDevice() const { return m_pDevice; }

    void FreeVertexBuffer(const VertexBuffer& buffer);
    void FreeIndexBuffer(const IndexBuffer& buffer);

public:
    void OnLostDevice();
    void OnResetDevice();

    // Clears the active streams
    void ResetActiveStreams();

    /* Adds the specified vertices in the specified format to a vertex buffer.
     * @format: Format of the vertices in the vertex array. You can get a predefined
     *          vertex format from a description string with GetVertexFormat().
     * @data:   Pointer to the vertex array.
     * @num:    Number of vertices. Multiplied with the size of vertex (as derived
     *          from the format) this yields the byte-size of the @data array.
     */
    ptr<VertexBuffer> CreateVertexBuffer(VertexFormat format, const void* vertices, DWORD numVertices);

    /* Creates a renderable mesh from an array of vertices and indices.
     * @format:      Format of the vertices in the vertex array. You can get a predefined
     *               vertex format from a description string with GetVertexFormat().
     * @vertices:    Pointer to the vertex array.
     * @numVertices: Number of vertices. Multiplied with the size of vertex (as derived
     *               from the format) this yields the byte-size of the @data array.
     * @indices:     Pointer to the index array.
     * @numIndices:  Number of indices.
     */
    ptr<IndexBuffer> CreateIndexBuffer(const uint16_t *indices, unsigned long numIndices);

    /* Returns the internal vertex format for the specified name. If the specified name
     * does not indicate an existing vertex format, VF_UNKNOWN is returned.
     * @name: Name of the vertex format.
     */
    static VertexFormat GetVertexFormat(const std::string& name);

    VertexManager(IDirect3DDevice9* pDevice, bool isUaW);
    ~VertexManager();
};

} }

#endif