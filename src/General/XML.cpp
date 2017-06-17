#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>
#include "General/XML.h"
#include "General/Exceptions.h"
#include "General/Log.h"
#include "General/Utils.h"
using namespace std;

namespace Alamo
{

static const int BUFFER_SIZE = 32*1024;	// Read this much at once

XMLNode::~XMLNode()
{
	for (vector<XMLNode*>::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		delete *i;
	}
}

static void onStartElement(void* userData, const XML_Char *name, const XML_Char **atts)
{
	XMLTree* tree = (XMLTree*)userData;

    // Create the node
    XMLNode* node = new XMLNode;
    node->m_parent = tree->m_currentNode;
    node->m_name   = tree->m_data.append(name, wcslen(name) + 1);
    node->m_data   = NULL;
	while (*atts != NULL)
	{
		node->m_attributes.push_back(make_pair(
            tree->m_data.append(atts[0], wcslen(atts[0]) + 1),
            tree->m_data.append(atts[1], wcslen(atts[1]) + 1)
        ));
		atts += 2;
	}

	if (tree->m_currentNode == NULL)
	{
        // This is the root
		if (tree->m_root != NULL)
		{
			delete tree->m_root;
		}
		tree->m_root = node;
	}
	else
	{
		tree->m_currentNode->m_children.push_back( node );
	}
	tree->m_currentNode = node;
    tree->m_currentData.resize(0);
}

static void onEndElement(void* userData, const XML_Char *name)
{
	XMLTree* tree = (XMLTree*)userData;
	if (tree->m_currentNode != NULL)
	{
        if (tree->m_currentNode->m_children.empty() && tree->m_currentData.size() > 0)
        {
            tree->m_currentData.append(L"", 1); // Append the NUL terminator

            // Set the collected node data; but trim it first
			wchar_t* begin = tree->m_currentData;
			wchar_t* end   = tree->m_currentData + tree->m_currentData.size() - 1;
            
            while (begin != end && isspace(*begin)) begin++;
            if (begin != end)
            {
                while (end != begin && isspace(*(end - 1))) end--;
                *end = '\0';
                tree->m_currentNode->m_data = tree->m_data.append(begin, end - begin + 1);
            }
        }
        
        // Move up the tree
		tree->m_currentNode = tree->m_currentNode->m_parent;
	}
}

static void onCharacterData(void *userData, const XML_Char *s, int len)
{
	XMLTree* tree = (XMLTree*)userData;
	if (tree->m_currentNode != NULL && tree->m_currentNode->m_children.empty())
	{
        // Only nodes without child nodes can have data
        tree->m_currentData.append(s, len);
	}
}

void XMLTree::parse(IFile* file)
{
	// Reset tree
	delete m_root;
	m_root        = NULL;
	m_currentNode = NULL;

	XML_Parser parser = XML_ParserCreate(NULL);
	if (parser == NULL)
	{
		throw wruntime_error(L"Unable to create XML parser");
	}

	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, onStartElement, onEndElement);
	XML_SetCharacterDataHandler(parser, onCharacterData);

	try
	{
        m_data.reserve(file->size());
		while (!file->eof())
		{
            char buffer [ BUFFER_SIZE ];
			size_t n = file->read(buffer, BUFFER_SIZE);
			if (XML_Parse(parser, buffer, (int)n, file->eof()) == 0)
			{
				stringstream error;
				error << XML_ErrorString(XML_GetErrorCode(parser)) << " at line " << XML_GetCurrentLineNumber(parser);
				throw ParseException( error.str() );
			}
		}

        // Cleanup
        m_currentNode = NULL;
        m_currentData.clear();
		XML_ParserFree(parser);
    }
	catch (...)
	{
		XML_ParserFree(parser);
		throw;
	}
}

XMLTree::XMLTree()
{
	m_root        = NULL;
	m_currentNode = NULL;
}

XMLTree::~XMLTree()
{
	delete m_root;
}

}