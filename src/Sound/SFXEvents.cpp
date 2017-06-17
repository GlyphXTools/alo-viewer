#include "Sound/SFXEvents.h"
#include "Assets/Assets.h"
#include "General/Exceptions.h"
#include "General/Log.h"
#include "General/XML.h"
#include <sstream>
using namespace std;

namespace Alamo {

typedef map<wstring,SFXEvent> SFXEventMap;

static const char* XML_PREFIX = "Data/XML/";

static SFXEventMap m_events;
static SFXEventMap m_presets;

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

static int ParseInt(const wchar_t* str, int def)
{
	wchar_t* endptr;
    long val = wcstol(str, &endptr, 0);
    return (*endptr != '\0') ? def : val;
}

static float ParseFloat(const wchar_t* str, float def)
{
	wchar_t* endptr;
    float val = (float)wcstod(str, &endptr);
    return (*endptr != '\0') ? def : val;
}

static SFXEvent MakeDefaultSFXEvent()
{
    SFXEvent e;
    e.type         = SFXEvent::SFX_2D;
    e.priority     = 10;
    e.maxInstances = 1;
    e.probability  = 1.0f;
    e.volume       = range<float>(1.0f, 1.0f);
    e.pitch        = range<float>(1.0f, 1.0f);
    e.predelay     = range<float>(0.0f, 0.0f);
    e.localize     = false;
    e.playCount    = 1;
    e.playSequentially         = false;
    e.killsPreviousObjectSFX   = false;
    e.volumeSaturationDistance = 0;
    e.loopFadeOutSeconds       = 0;
    e.loopFadeInSeconds        = 0;
    return e;
}

static void CopyPresetData(SFXEvent& e, const wchar_t* name)
{
    // Find and use the preset
    SFXEventMap::const_iterator p = m_presets.find(Uppercase(name));
    if (p != m_presets.end()) {
        const SFXEvent& preset = p->second;

        e.type         = preset.type;
        e.priority     = preset.priority;
        e.maxInstances = preset.maxInstances;
        e.probability  = preset.probability;
        e.volume       = preset.volume;
        e.pitch        = preset.pitch;
        e.predelay     = preset.predelay;
        e.localize     = preset.localize;
        e.playCount    = preset.playCount;
        e.chainedSFXEvent          = preset.chainedSFXEvent;
        e.playSequentially         = preset.playSequentially;
        e.killsPreviousObjectSFX   = preset.killsPreviousObjectSFX;
        e.volumeSaturationDistance = preset.volumeSaturationDistance;
        e.loopFadeOutSeconds       = preset.loopFadeOutSeconds;
        e.loopFadeInSeconds        = preset.loopFadeInSeconds;
    } else {
        Log::WriteError("Invalid SFX preset \"%s\"\n", name);
    }
}

static void ParseSampleList(const wstring& data, vector<string>& samples)
{
    static const wchar_t* whitespace = L" \t\r\n\t\v";
    string::size_type start, sp = 0;
    while ((start = data.find_first_not_of(whitespace, sp)) != string::npos) {
        sp = data.find_first_of(whitespace, start);
        if (sp == string::npos) {
            sp = data.size();
        }
        samples.push_back( Uppercase(WideToAnsi(data.substr(start, sp - start))) );
    }
}

static void ParseSFXEvent(const XMLNode* ent, const wchar_t* name)
{
    SFXEvent e = MakeDefaultSFXEvent();
    
    bool preset = false, is2D = false, is3D = false, isGUI = false;
    bool isUnitResponseVO = false, isAmbientVO = false, isHudVO = false;

    for (size_t i = 0; i < ent->getNumChildren(); i++)
    {
        const XMLNode* node = ent->getChild(i);
        const wchar_t* data = node->getData();
        if (data != NULL)
        {
                 if (node->equals(L"IS_PRESET"))           preset           = ParseBool(data, false);
            else if (node->equals(L"IS_2D"))               is2D             = ParseBool(data, false);
            else if (node->equals(L"IS_3D"))               is3D             = ParseBool(data, false);
            else if (node->equals(L"IS_GUI"))              isGUI            = ParseBool(data, false);
            else if (node->equals(L"IS_UNIT_RESPONSE_VO")) isUnitResponseVO = ParseBool(data, false);
            else if (node->equals(L"IS_AMBIENT_VO"))       isAmbientVO      = ParseBool(data, false);
            else if (node->equals(L"IS_HUD_VO"))           isHudVO          = ParseBool(data, false);
            else if (node->equals(L"MAX_INSTANCES")) e.maxInstances = max(1, ParseInt(data, e.maxInstances) );
            else if (node->equals(L"PRIORITY"))      e.priority     = max(1, ParseInt(data, e.priority) );
            else if (node->equals(L"PROBABILITY"))   e.probability  = max(0, min(1, ParseFloat(data, 100) / 100.0f));
            else if (node->equals(L"MIN_VOLUME"))    e.volume.min   = max(0, min(1, ParseFloat(data, 100) / 100.0f));
            else if (node->equals(L"MAX_VOLUME"))    e.volume.max   = max(0, min(1, ParseFloat(data, 100) / 100.0f));
            else if (node->equals(L"MIN_PITCH"))     e.pitch.min    = max(0, min(1, ParseFloat(data, 100) / 100.0f));
            else if (node->equals(L"MAX_PITCH"))     e.pitch.max    = max(0, min(1, ParseFloat(data, 100) / 100.0f));
            else if (node->equals(L"MIN_PREDELAY"))  e.predelay.min = max(0, ParseFloat(data, 0) / 1000.0f);
            else if (node->equals(L"MAX_PREDELAY"))  e.predelay.max = max(0, ParseFloat(data, 0) / 1000.0f);
            else if (node->equals(L"PLAY_COUNT"))    e.playCount    = ParseInt(data, 1);
            else if (node->equals(L"LOCALIZE"))                   e.localize                 = ParseBool(data, e.localize);
            else if (node->equals(L"KILLS_PREVIOUS_OBJECT_SFX"))  e.killsPreviousObjectSFX   = ParseBool(data, e.localize);
            else if (node->equals(L"PLAY_SEQUENTIALLY"))          e.playSequentially         = ParseBool(data, e.localize);
            else if (node->equals(L"VOLUME_SATURATION_DISTANCE")) e.volumeSaturationDistance = ParseFloat(data, 0);
            else if (node->equals(L"LOOP_FADE_IN_SECONDS"))       e.loopFadeOutSeconds       = max(0, ParseFloat(data, 0));
            else if (node->equals(L"LOOP_FADE_OUT_SECONDS"))      e.loopFadeInSeconds        = max(0, ParseFloat(data, 0));
            else if (node->equals(L"USE_PRESET"))       CopyPresetData(e, data);
            else if (node->equals(L"SAMPLES"))          ParseSampleList(data, e.samples);
            else if (node->equals(L"PRE_SAMPLES"))      ParseSampleList(data, e.preSamples);
            else if (node->equals(L"POST_SAMPLES"))     ParseSampleList(data, e.postSamples);
            else if (node->equals(L"CHAINED_SFXEVENT")) e.chainedSFXEvent = WideToAnsi(data);
            // We don't care about these
            //else if (name == "TEXT_ID");
            //else if (name == "OVERLAP_TEST");
        }
    }
    
    // Sanitize and normalize
    e.volume.normalize();
    e.pitch.normalize();
    e.type = isGUI ? SFXEvent::SFX_GUI : (is3D ? SFXEvent::SFX_3D : SFXEvent::SFX_2D);
    
    if (preset) m_presets[Uppercase(name)] = e;
    else        m_events [Uppercase(name)] = e;
}

static void ParseSFXEventFile(const wstring& filename)
{
    ptr<IFile> file = Assets::LoadFile(WideToAnsi(filename), XML_PREFIX);
    if (file != NULL)
    {
        // Parse file
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
            const wchar_t* name = ent->getAttribute(L"Name");
            if (name != NULL && wcscmp(name, L"") != 0)
            {
                ParseSFXEvent(ent, name);
            }
        }
    }
}

void SFXEvents::Initialize()
{
    string filename = "SFXEventFiles.xml";
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
            ParseSFXEventFile( root->getChild(i)->getData() );
        }
    }
}   

void SFXEvents::Uninitialize()
{
    m_events.clear();
    m_presets.clear();
}

const SFXEvent* SFXEvents::GetEvent(const string& name)
{
    SFXEventMap::const_iterator p = m_events.find(Uppercase(AnsiToWide(name)));
    return (p != m_events.end()) ? &p->second : NULL;
}

}