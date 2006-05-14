#include <iostream>
#include <fstream>
#include "xml.h"
#include "exceptions.h"
using namespace std;

static const int BUFFER_SIZE = 32*1024;	// Read 32k at once

XMLNode::XMLNode(XMLNode* parent, const XML_Char* name, const XML_Char **atts)
{
	this->parent   = parent;
	this->name     = name;

	while (*atts != NULL)
	{
		attributes.insert(make_pair(atts[0], atts[1]));
	}
}

XMLNode::XMLNode(XMLNode* parent, const XML_Char* s, int len) : data(s, len)
{
	this->parent = parent;
}

XMLNode::~XMLNode()
{
	for (vector<XMLNode*>::iterator i = children.begin(); i != children.end(); i++)
	{
		delete *i;
	}
}

static string trim(const string& source, const char* delims = " \t\r\n")
{
	string result(source);
	string::size_type index = result.find_last_not_of(delims);
	if (index != string::npos)
		result.erase(index + 1);

	index = result.find_first_not_of(delims);
	if(index != string::npos) 
		result.erase(0, index);
	else
		result.erase();
	return result;
}

static void checkEmpty(XMLNode* node)
{
	if (node->children.size() > 0)
	{
		XMLNode* child = node->children.back();
		child->data = trim(child->data);
		if (child->data == "" && child->name == "")
		{
			delete child;
			node->children.erase( node->children.end() - 1 );
		}
	}
}

static void onStartElement(void* userData, const XML_Char *name, const XML_Char **atts)
{
	XMLTree* tree = (XMLTree*)userData;
	XMLNode* node = new XMLNode(tree->current, name, atts);
	if (tree->current == NULL)
	{
		if (tree->root == NULL)
		{
			delete tree->root;
		}
		tree->root = node;
	}
	else
	{
		checkEmpty(tree->current);
		tree->current->children.push_back( node );
	}
	tree->current = node;
}

static void onEndElement(void* userData, const XML_Char *name)
{
	XMLTree* tree = (XMLTree*)userData;
	if (tree->current != NULL)
	{
		// Post-process this node; if it contains a single anonymous child, put it into
		// this node's data field
		if ((tree->current->children.size() == 1) && (tree->current->children.front()->name == ""))
		{
			tree->current->data = tree->current->children.front()->data;
			delete tree->current->children.front();
			tree->current->children.clear();
		}
		tree->current->data = trim(tree->current->data);

		checkEmpty(tree->current);
		tree->current = tree->current->parent;
	}
}

static void onCharacterData(void *userData, const XML_Char *s, int len)
{
	XMLTree* tree = (XMLTree*)userData;
	if (tree->current != NULL)
	{
		if ((tree->current->children.size() > 0) && (tree->current->children.back()->name == ""))
		{
			tree->current->children.back()->data += string(s, len);
		}
		else
		{
			tree->current->children.push_back( new XMLNode(tree->current, s, len) );
		}
	}
}

void XMLTree::parse(File* file)
{
	// Reset tree
	delete root;
	root    = NULL;
	current = NULL;

	XML_Parser parser = XML_ParserCreate(NULL);
	if (parser == NULL)
	{
		throw Exception("Unable to create XML parser");
	}

	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, onStartElement, onEndElement);
	XML_SetCharacterDataHandler(parser, onCharacterData);

	try
	{
		while (!file->eof())
		{
			char buffer[ BUFFER_SIZE ];
			unsigned long n = file->read(buffer, BUFFER_SIZE);
			if (XML_Parse(parser, buffer, n, file->eof()) == 0)
			{
				string error = XML_ErrorString(XML_GetErrorCode(parser));
				error += " at line " + XML_GetCurrentLineNumber(parser);
				throw ParseException( error );
			}
		}
	}
	catch (...)
	{
		XML_ParserFree(parser);
		throw;
	}
	XML_ParserFree(parser);
}

XMLTree::XMLTree()
{
	root    = NULL;
	current = NULL;
}

XMLTree::~XMLTree()
{
	delete root;
}