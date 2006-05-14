#ifndef XML_H
#define XML_H

#include <vector>
#include <string>
#include <map>

#include "expat/expat.h"
#include "managers.h"

class XMLTree;

class XMLNode
{
	friend static void onStartElement(void* userData, const XML_Char *name, const XML_Char **atts);
	friend static void onEndElement(void* userData, const XML_Char *name);
	friend static void onCharacterData(void *userData, const XML_Char *s, int len);
	friend static void checkEmpty(XMLNode* node);
	friend class XMLTree;

	typedef std::map<std::string,std::string> AttrMap;

	std::string name;
	XMLNode*    parent;
	std::string data;
	AttrMap     attributes;


	std::vector<XMLNode*> children;

	XMLNode(XMLNode* parent, const XML_Char* name, const XML_Char **atts);
	XMLNode(XMLNode* parent, const XML_Char* s, int len);
	~XMLNode();

public:
	bool               isAnonymous() const    { return name == ""; }
	const std::string& getData() const        { return data; }
	const std::string& getName() const        { return name; }
	const unsigned int getNumChildren() const { return (unsigned int)children.size(); }
	const XMLNode*     getChild(int i) const  { return children[i]; }
	
	const bool hasAttribute(std::string name) const
	{
		return attributes.find(name) != attributes.end();
	}

	std::string getAttribute(std::string name) const
	{
		AttrMap::const_iterator p = attributes.find(name);
		return (p == attributes.end()) ? "" : p->second;
	}
};


class XMLTree
{
	friend static void onStartElement(void* userData, const XML_Char *name, const XML_Char **atts);
	friend static void onEndElement(void* userData, const XML_Char *name);
	friend static void onCharacterData(void *userData, const XML_Char *s, int len);

	XMLNode* root;
	XMLNode* current;	// Used during parsing

public:
	const XMLNode* getRoot() const
	{
		return root;
	}

	void parse(File* file);

	XMLTree();
	~XMLTree();
};

#endif