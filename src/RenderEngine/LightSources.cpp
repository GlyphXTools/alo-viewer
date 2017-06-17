#include "RenderEngine/LightSources.h"
#include "Assets/Assets.h"
#include "General/Utils.h"
#include "General/Log.h"
#include "General/XML.h"
#include "General/Exceptions.h"
using namespace std;

static const char* XML_PREFIX = "Data/XML/";

namespace Alamo {
namespace LightSources {

typedef map<wstring, LightFieldSource> LightFieldSourceMap;
static LightFieldSourceMap m_lightsources;

static bool equals(const wchar_t* s1, const wchar_t* s2)
{
    while (*s1 != '\0' && isspace(*s1)) s1++;
    size_t len = wcslen(s2);
    if (_wcsnicmp(s1, s2, len) == 0)
    {
        s1 += len;
        while (*s1 != '\0' && isspace(*s1)) s1++;
        return *s1 == '\0';
    }
    return false;
}

static float ParseFloat(const wchar_t* str, float def)
{
	wchar_t* endptr;
    float val = (float)wcstod(str, &endptr);
    return (*endptr != '\0') ? def : val;
}

static Vector3 ParseVector(const wchar_t* str, const Vector3& def)
{
	wchar_t* endptr;
    float x = (float)wcstod(str, &endptr);
    if (*endptr == ',')
    {
        float y = (float)wcstod(endptr + 1, &endptr);
        if (*endptr == ',')
        {
            float z = (float)wcstod(endptr + 1, &endptr);
            if (*endptr == '\0')
            {
                return Vector3(x,y,z);
            }
        }
    }
    return def;
}

// We accept "Yes", "True" or "1" as true
static bool ParseBool(const wchar_t* str, bool def)
{
    if (equals(str, L"Yes"))   return true;
    if (equals(str, L"No"))    return false;
    if (equals(str, L"True"))  return true;
    if (equals(str, L"False")) return false;
 
	wchar_t* endptr;
    long val = wcstol(str, &endptr, 0);
    return (*endptr != '\0') ? def : (val != 0);
}

static LightFieldSourceType ParseType(const wchar_t* data)
{
    static const struct {
        const wchar_t*       name;
        LightFieldSourceType type;
    } Types[] = {
        {L"POINT",              LFT_POINT},
        {L"HCONE",              LFT_HCONE},
        {L"DUAL_HCONE",         LFT_DUAL_HCONE},
        {L"DUAL_OPPOSED_HCONE", LFT_DUAL_OPPOSED_HCONE},
        {L"LINE",               LFT_LINE},
        {NULL}
    };

    for (int i = 0; Types[i].name != NULL; i++)
    {
        if (_wcsicmp(data, Types[i].name) == 0)
        {
            return Types[i].type;
        }
    }
    return LFT_POINT;
}

static LightFieldSource MakeDefaultLightFieldSource()
{
    LightFieldSource info;
    info.m_type      = LFT_POINT;
    info.m_fadeType  = LFFT_TIME;
    info.m_particles = 0;
    info.m_diffuse   = Color(1,1,1,1);
    info.m_intensity = 1.0f;
    info.m_width     = 50.0f; 
    info.m_length    = 50.0f;
    info.m_height    = 1.0f;
    info.m_autoDestructTime        = 0.0f;
    info.m_autoDestructFadeTime    = 0.0f;
    info.m_intensityNoiseScale     = 0.0f;
    info.m_intensityNoiseTimeScale = 0.0f;
    info.m_angularVelocity         = 0.0f;
    info.m_affectedByGlobalIntensity = false;
    return info;
}

static void ParseLightFieldSource(const XMLNode* ent)
{
	const wchar_t* name = ent->getName();
    if (name == NULL)
    {
        throw ParseException("Ignoring unnamed light field source");
    }

    LightFieldSource info;
    for (size_t i = 0; i < ent->getNumChildren(); i++)
    {
        const XMLNode* node = ent->getChild(i);
        const wchar_t* data = node->getData();
        if (data != NULL)
        {
                 if (node->equals(L"TYPE"))                       info.m_type                    = ParseType(data);
            else if (node->equals(L"DIFFUSE"))                    info.m_diffuse                 = Color(Vector4(ParseVector(data, Vector3(1,1,1)), 1.0f));
            else if (node->equals(L"INTENSITY"))                  info.m_intensity               = ParseFloat(data, 1.0f);
            else if (node->equals(L"WIDTH"))                      info.m_width                   = ParseFloat(data, 50.0f); 
            else if (node->equals(L"LENGTH"))                     info.m_length                  = ParseFloat(data, 50.0f);
			else if (node->equals(L"HEIGHT"))                     info.m_height = ParseFloat(data, 1.0f);
            else if (node->equals(L"AUTO_DESTRUCT_TIME"))         info.m_autoDestructTime        = ParseFloat(data, 0.0f);
            else if (node->equals(L"AUTO_DESTRUCT_FADE_TIME"))    info.m_autoDestructFadeTime    = ParseFloat(data, 0.0f);
            else if (node->equals(L"INTENSITY_NOISE_SCALE"))      info.m_intensityNoiseScale     = ParseFloat(data, 0.0f);
            else if (node->equals(L"INTENSITY_NOISE_TIME_SCALE")) info.m_intensityNoiseTimeScale = ParseFloat(data, 0.0f);
            else if (node->equals(L"ANGULAR_VELOCITY"))           info.m_angularVelocity         = ParseFloat(data, 0.0f);
            else if (node->equals(L"IS_AFFECTED_BY_GLOBAL_LIGHT_FIELD_INTENSITY")) info.m_affectedByGlobalIntensity = ParseBool(data, false);
        }
    }
    m_lightsources.insert(make_pair(Uppercase(name), info));
}

void Initialize()
{
    string filename = "LightSources.xml";
    ptr<IFile> file = Assets::LoadFile(filename, XML_PREFIX);
    if (file != NULL)
    {
        // Parse index file
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

        const XMLNode* root = xml.getRoot();
        for (size_t i = 0; i < root->getNumChildren(); i++)
        {
            ParseLightFieldSource( root->getChild(i) );
        }
    }
}

void Uninitialize()
{
    m_lightsources.clear();
}

}

using namespace LightSources;

const LightFieldSource* GetLightFieldSource(string name)
{
    transform(name.begin(), name.end(), name.begin(), toupper);
    LightFieldSourceMap::const_iterator p = m_lightsources.find(AnsiToWide(name));
    return (p != m_lightsources.end()) ? &p->second : NULL;
}

}