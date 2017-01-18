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

typedef map<string, LightFieldSource> LightFieldSourceMap;
static LightFieldSourceMap m_lightsources;

static bool equals(const char* s1, const char* s2)
{
    while (*s1 != '\0' && isspace(*s1)) s1++;
    size_t len = strlen(s2);
    if (_strnicmp(s1, s2, len) == 0)
    {
        s1 += len;
        while (*s1 != '\0' && isspace(*s1)) s1++;
        return *s1 == '\0';
    }
    return false;
}

static float ParseFloat(const char* str, float def)
{
    char* endptr;
    float val = (float)strtod(str, &endptr);
    return (*endptr != '\0') ? def : val;
}

static Vector3 ParseVector(const char* str, const Vector3& def)
{
    char* endptr;
    float x = (float)strtod(str, &endptr);
    if (*endptr == ',')
    {
        float y = (float)strtod(endptr + 1, &endptr);
        if (*endptr == ',')
        {
            float z = (float)strtod(endptr + 1, &endptr);
            if (*endptr == '\0')
            {
                return Vector3(x,y,z);
            }
        }
    }
    return def;
}

// We accept "Yes", "True" or "1" as true
static bool ParseBool(const char* str, bool def)
{
    if (equals(str, "Yes"))   return true;
    if (equals(str, "No"))    return false;
    if (equals(str, "True"))  return true;
    if (equals(str, "False")) return false;
 
    char* endptr;
    long val = strtol(str, &endptr, 0);
    return (*endptr != '\0') ? def : (val != 0);
}

static LightFieldSourceType ParseType(const char* data)
{
    static const struct {
        const char*          name;
        LightFieldSourceType type;
    } Types[] = {
        {"POINT",              LFT_POINT},
        {"HCONE",              LFT_HCONE},
        {"DUAL_HCONE",         LFT_DUAL_HCONE},
        {"DUAL_OPPOSED_HCONE", LFT_DUAL_OPPOSED_HCONE},
        {"LINE",               LFT_LINE},
        {NULL}
    };

    for (int i = 0; Types[i].name != NULL; i++)
    {
        if (_stricmp(data, Types[i].name) == 0)
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
    const char* name = ent->getAttribute("Name");
    if (name == NULL)
    {
        throw ParseException("Ignoring unnamed light field source");
    }

    LightFieldSource info;
    for (size_t i = 0; i < ent->getNumChildren(); i++)
    {
        const XMLNode* node = ent->getChild(i);
        const char*    data = node->getData();
        if (data != NULL)
        {
                 if (node->equals("TYPE"))                       info.m_type                    = ParseType(data);
            else if (node->equals("DIFFUSE"))                    info.m_diffuse                 = Color(Vector4(ParseVector(data, Vector3(1,1,1)), 1.0f));
            else if (node->equals("INTENSITY"))                  info.m_intensity               = ParseFloat(data, 1.0f);
            else if (node->equals("WIDTH"))                      info.m_width                   = ParseFloat(data, 50.0f); 
            else if (node->equals("LENGTH"))                     info.m_length                  = ParseFloat(data, 50.0f);
            else if (node->equals("HEIGHT"))                     info.m_height                  = ParseFloat(data,  1.0f);
            else if (node->equals("AUTO_DESTRUCT_TIME"))         info.m_autoDestructTime        = ParseFloat(data, 0.0f);
            else if (node->equals("AUTO_DESTRUCT_FADE_TIME"))    info.m_autoDestructFadeTime    = ParseFloat(data, 0.0f);
            else if (node->equals("INTENSITY_NOISE_SCALE"))      info.m_intensityNoiseScale     = ParseFloat(data, 0.0f);
            else if (node->equals("INTENSITY_NOISE_TIME_SCALE")) info.m_intensityNoiseTimeScale = ParseFloat(data, 0.0f);
            else if (node->equals("ANGULAR_VELOCITY"))           info.m_angularVelocity         = ParseFloat(data, 0.0f);
            else if (node->equals("IS_AFFECTED_BY_GLOBAL_LIGHT_FIELD_INTENSITY")) info.m_affectedByGlobalIntensity = ParseBool(data, false);
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
    LightFieldSourceMap::const_iterator p = m_lightsources.find(name);
    return (p != m_lightsources.end()) ? &p->second : NULL;
}

}