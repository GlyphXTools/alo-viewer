#ifndef OBJECTS_H
#define OBJECTS_H

#include <cstdlib>
#include <cassert>

/*
 * Reference counted object base.
 * Inherit from this to make your objects reference counted.
 */
class IObject
{
	unsigned long nReferences;

protected:
    virtual ~IObject() {}

public:
	unsigned long AddRef()
	{
		return ++nReferences;
	}

	unsigned long Release()
	{
		unsigned long refs = --nReferences;
		if (nReferences == 0)
		{
			delete this;
		}
		return refs;
	}

	IObject() : nReferences(1) {}
};

/*
 * A reference counting pointer template for RefCounted or COM objects
 * Note: when creating or assigning from a 'raw' pointer, the reference
 *       count is NOT increased. Only when copying between ptr<>'s are
 *       the reference counts properly taken care of.
 */
template <typename T>
class ptr
{
    friend class ptr;

	T* object;

public:
	// Equality and inequality
	bool operator==(const T* obj)     const { return object == obj; }
	bool operator==(const ptr<T> ptr) const { return object == ptr.object; }
	bool operator!=(const T* obj)	  const { return object != obj; }
	bool operator!=(const ptr<T> ptr) const { return object != ptr.object; }
	
	// Dereference and cast operator
	T* operator->() const { assert(object != NULL); return object; }
    operator T*()   const { return object; }

	// Assignment
	ptr<T>& operator=(const ptr<T>& ptr) {
		SAFE_RELEASE(object);
		object = ptr.object;
		if (object != NULL) object->AddRef();
		return *this;
	}

	ptr<T>& operator=(T* obj) {
        SAFE_RELEASE(object);
		object = obj;
		return *this;
	}

    // Dynamic cast with references
    template <typename TT>
    ptr<TT> cast() const
    {
        TT* obj = dynamic_cast<TT*>(object);
        if (obj != NULL)
        {
            obj->AddRef();
            return ptr<TT>(obj);
        }
        return NULL;
    }

	// Constructors
	ptr(T* obj) { object = obj; }
    template <typename TT>
	ptr(const ptr<TT>& ptr) { object = ptr.object; if (object != NULL) object->AddRef(); }
	ptr(const ptr<T>& ptr)  { object = ptr.object; if (object != NULL) object->AddRef(); }
	ptr()  { object = NULL; }
	~ptr() { SAFE_RELEASE(object); }
};

/*
 * A buffer template for primitives (integers, vertices, etc)
 *
 * Kinda like a vector, only not suited for anything but primitives since it
 * uses malloc, realloc and free, not paying attention to constructors and
 * destructors.
 */
template <typename T>
class Buffer
{
	T*				m_data;
	size_t			m_size;
    size_t			m_capacity;

public:
	// Cast operator
	operator T*() const { return m_data; }

    void clear() {
        free(m_data);
        m_data     = NULL;
        m_size     = 0;
        m_capacity = 0;
    }

	// Size functions
	size_t size()     const  { return m_size; }
    size_t capacity() const  { return m_capacity; }
	bool   empty()    const  { return m_size == 0; }
	void   resize(size_t size) {
        if (size > capacity()) {
            reserve(size);
        }
        m_size = size;
	}

    void reserve(size_t capacity) {
        if (capacity > size()) {
		    T* tmp = (T*)realloc(m_data, capacity * sizeof(T));
            if (tmp == NULL) throw std::bad_alloc();
		    m_data     = tmp;
		    m_capacity = capacity;
        }
	}

    T* append(const T* data, size_t len) {
        size_t pos = size();
        resize(size() + len);
        memcpy(m_data + pos, data, len * sizeof(T));
        return m_data + pos;
    }

    T* append(const Buffer<T> data) {
        return append(&data[0], data.size());
    }

	Buffer& operator =(const Buffer& buf) {
        free(m_data);
        m_data     = 0;
        m_size     = 0;
        m_capacity = 0;
        append(buf.m_data, buf.m_size);
		return *this;
	}

	// Constructors
	Buffer() {
		m_data     = NULL;
		m_size     = 0;
        m_capacity = 0;
	}

	Buffer(size_t size) {
		m_data     = NULL;
		m_size     = 0;
        m_capacity = 0;
		resize(size);
	}

	Buffer(size_t size, const T& def) {
		m_data     = NULL;
		m_size     = 0;
        m_capacity = 0;
		resize(size);
        for (size_t i = 0; i < size; i++) {
            m_data[i] = def;
        }
	}

	Buffer(const Buffer& buf) {
        m_data     = 0;
        m_size     = 0;
        m_capacity = 0;
        append(buf.m_data, buf.m_size);
	}

 	~Buffer() {
        free(m_data);
	}
};

/*
 * A safe release template
 * Releases the object and sets the pointer to NULL
 */
#ifndef SAFE_RELEASE
template <typename T>
static void SAFE_RELEASE(ptr<T> &p) {
	p = NULL;
}

template <typename T>
static void SAFE_RELEASE(T* &ptr) {
	if (ptr != NULL) {
		ptr->Release();
		ptr = NULL;
	}
}
#endif

/*
 * Linked list class
 */
template <typename T>
class LinkedListObject;

template <typename T>
class LinkedList
{
    friend LinkedListObject<T>;

    T* m_head;

public:
    operator T* () const {
        return m_head;
    }

    LinkedList() : m_head(NULL) {}
};

template <typename T>
class LinkedListObject
{
    T** m_prev;
    T*  m_next;

protected:
    void Link(LinkedList<T>& list)
    {
        m_next = list.m_head;
        m_prev = &list.m_head;
        if (list.m_head != NULL) {
            list.m_head->m_prev = &m_next;
        }
        list.m_head = dynamic_cast<T*>(this);
        assert(list.m_head != NULL);
    }

    void Unlink()
    {
        if (m_prev != NULL)
        {
            *m_prev = m_next;
            if (m_next != NULL) {
                m_next->m_prev = m_prev;
            }
        }
    }

    LinkedListObject() : m_prev(NULL) {
    }

    virtual ~LinkedListObject() {
        Unlink();
    }

public:
    T* GetNext() const {
        return m_next;
    }
};

#endif