/*
 * Defines a XMLTree class that holds ab XML document parsed into a tree.
 * It uses a slight restriction for XML; nodes can either contain child
 * nodes or data, but not both.
 */
#ifndef XML_H
#define XML_H

#include <algorithm>
#include <vector>
#include <string>
#include <map>

#include "Assets/Files.h"
#include <expat/expat.h>

namespace Alamo
{

class XMLTree;

/* Represents a node in an XML Tree.
 * Note that pointers to XMLNode's, as handed out by XMLTree and XMLNode are valid only
 * during the lifetime of the XMLTree variable.
 */
class XMLNode
{
	friend static void onStartElement (void* userData, const XML_Char *name, const XML_Char **atts);
	friend static void onEndElement   (void* userData, const XML_Char *name);
	friend static void onCharacterData(void *userData, const XML_Char *s, int len);
	friend class XMLTree;

    typedef std::vector< std::pair<XML_Char*,XML_Char*> > AttrList;

	XMLNode*              m_parent;
	XML_Char*             m_name;
	XML_Char*             m_data;
	AttrList              m_attributes;
	std::vector<XMLNode*> m_children;

	~XMLNode();

public:
	bool           isAnonymous() const      { return strcmp(m_name,"") == 0; }
	const char*    getData() const          { return m_data; }
	const char*    getName() const          { return m_name; }
	const size_t   getNumChildren() const   { return m_children.size(); }
	const XMLNode* getChild(size_t i) const { return m_children[i]; }
	bool           equals(const char* name)        const { return _stricmp(m_name, name) == 0; }
	bool           equals(const std::string& name) const { return equals(name.c_str()); }
	
    /* Returns the value of the specified attribute.
     * Returns NULL if the attribute does not exist.
     * @name: case-insensitive name of the attribute.
     */
	const char* getAttribute(const char* name) const {
        for (AttrList::const_iterator i = m_attributes.begin(); i != m_attributes.end(); i++) {
            if (_stricmp(i->first, name) == 0) {
                return i->second;
            }
        }
		return NULL;
	}

    const char* getAttribute(const std::string& name) const {
        return getAttribute(name.c_str());
    }
};

// Represents a XML tree built from a XML file
class XMLTree
{
	friend static void onStartElement(void* userData, const XML_Char *name, const XML_Char **atts);
	friend static void onEndElement(void* userData, const XML_Char *name);
	friend static void onCharacterData(void *userData, const XML_Char *s, int len);

    Buffer<XML_Char> m_data;
	XMLNode*         m_root;

    // Used during parsing
	XMLNode*         m_currentNode;
    Buffer<XML_Char> m_currentData;

public:
    // Returns the root node
	const XMLNode* getRoot() const
	{
		return m_root;
	}

    // Builds the tree using the specified XML file
	void parse(IFile* file);

	XMLTree();
	~XMLTree();
};

}

#endif