#include "Effects/SurfaceFX.h"
#include "General/XML.h"
#include "General/Utils.h"
#include "General/Exceptions.h"
#include "General/Log.h"
#include "Assets/Assets.h"
#include <map>
using namespace std;

namespace Alamo {
namespace SurfaceFX {

typedef map<wstring, Effect> EffectMap;

static const char* XML_PREFIX = "Data/XML/";

static EffectMap m_effects;

static void ParseSurfaceFX(const XMLNode* ent, const wstring& name)
{
    if (ent->getNumChildren() > 0)
    {
        // We simply use the first <Surface_Setting>
        ent = ent->getChild(0);

        Effect e;
        for (size_t i = 0; i < ent->getNumChildren(); i++)
        {
            const XMLNode* node = ent->getChild(i);
            const wchar_t* data = node->getData();

            if (node->equals(L"SOUNDFX_NAME") && data != NULL) {
                e.sound = WideToAnsi(data);
            }
        }
        m_effects[Uppercase(name)] = e;
    }
}

void Initialize()
{
    const string filename = "SurfaceFX.xml";
    ptr<IFile> file = Assets::LoadFile(filename, XML_PREFIX);
    if (file)
    {
        XMLTree xml;
        try
        {
            xml.parse(file);
        }
        catch (ParseException& e)
        {
            // Can't parse XML
            Log::WriteError("Parse error in %s%s: %s\n", XML_PREFIX, filename.c_str(), e.what());
            return;
        }

        // Parse XML tree
        const XMLNode* root = xml.getRoot();
        for (size_t i = 0; i < root->getNumChildren(); i++)
        {
            const XMLNode* ent = root->getChild(i);
			wstring name = ent->getName();
            if (!name.empty())
            {
                ParseSurfaceFX(ent, name);
            }
        }
    }
}

void Uninitialize()
{
    m_effects.clear();
}

const Effect* GetEffect(const std::string& name)
{
    EffectMap::const_iterator p = m_effects.find(Uppercase(AnsiToWide(name)));
    return (p != m_effects.end()) ? &p->second : NULL;
}

}
}
