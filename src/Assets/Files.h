#ifndef FILES_H
#define FILES_H

#include "General/Objects.h"
#include <string>

namespace Alamo
{

/* Abstract file interface. Exposes such methods as read, write and seek.
 * Inherits from IObject, so use AddRef()/Release() or the ptr<> template.
 */
class IFile : public IObject
{
    std::wstring m_name;
public:
    // Get the filename
    const std::wstring& name() const {
        return m_name;
    }

    // Has the file cursor reached EOF?
	virtual bool eof() const = 0;

    // Returns the size of the file
	virtual size_t size() const = 0;

    // Returns the current position of the file cursor, relative to the start of the file.
	virtual unsigned long tell() const = 0;
    
    /* Sets the absolute position of the file cursor.
     *  @pos: cursor position, in bytes, relative to the start of the file.
     */
	virtual unsigned long seek(unsigned long pos) = 0;

    /* Sets the relative position of the file cursor.
     *  @pos: cursor position, in bytes, relative to the current cursor position.
     */
	virtual unsigned long skip(long count) = 0;

    /* Reads data from the file at the current file cursor.
     * Returns the number of bytes read. This may differ from @size if an attempt
     * to read past EOF was made.
     * If the file could not be read, a ReadException is thrown.
     *  @buffer: address in memory to write data to.
     *  @size:   number of bytes to read from the file.
     */
	virtual size_t read(void* buffer, size_t size) = 0;

    /* Writes data to the file at the current file cursor.
     * Returns the number of bytes written.
     * If the file could not be written, a WriteException is thrown.
     *  @buffer: address in memory to read data from.
     *  @size:   number of bytes to write to the file.
     */
    virtual size_t write(const void* buffer, size_t size) = 0;

    IFile(const std::wstring& name);
    virtual ~IFile() {}
};

/* IFile implementation for physical files, i.e. files directly visible
 * in the OS' file system */
class PhysicalFile : public IFile
{
	void*         m_hFile;
	unsigned long m_offset;

	~PhysicalFile();
public:
    // Functions inherited from IFile
	bool eof() const;
	size_t size() const;
	unsigned long tell() const;
	unsigned long seek(unsigned long pos);
	unsigned long skip(long count);
	size_t read(void* buffer, size_t size);
	size_t write(const void* buffer, size_t size);

    /* Constructs the file for the given filename.
     * If the file cannot be found, a FileNotFoundException will be thrown.
     * If the file cannot be opened, an IOException will be thrown.
     *  @filename: path of the file.
     */
    PhysicalFile(const std::wstring& filename);
};

/* IFile implementation for files-in-files, i.e. files embedded in other files. */
class SubFile : public IFile
{
	IFile*        m_file;
	unsigned long m_start;
	unsigned long m_size;
	unsigned long m_offset;

	~SubFile();
public:
    // Functions inherited from IFile
	bool eof() const;
	size_t size() const;
	unsigned long tell() const;
	unsigned long seek(unsigned long pos);
	unsigned long skip(long count);
	size_t read(void* buffer, size_t size);
	size_t write(const void* buffer, size_t size);

    /* Constructs the file from the given file, at the specified range.
     *  @file:  file that this file is contained in.
     *  @start: offset, relative to the start of @file, that this file starts at.
     *  @size:  size, in bytes, of this file.
     */
    SubFile(IFile* file, const std::string& subfilename, unsigned long start, unsigned long size);
};

};

#endif