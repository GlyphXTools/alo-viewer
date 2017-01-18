#include "Sound/AnimationSFXMaps.h"
#include "Assets/Assets.h"
#include "General/Utils.h"
#include "General/Log.h"
#include <string>
#include <sstream>
using namespace std;

namespace Alamo {
namespace AnimationSFXMaps {

typedef map<string, SFXMap> SFXMaps;

static SFXMaps m_maps;

const SFXMap* GetEvents(const std::string& animation)
{
    string s = Uppercase(animation);
    SFXMaps::const_iterator p = m_maps.find(Uppercase(animation));
    return (p != m_maps.end()) ? &p->second : NULL;
}

static void ParseLine(const string& line)
{
    // Parse one line
    string animation;
    Event  current;
    float  time;
    int    state = 0;

    string::size_type start, sp = 0;
    static const char* whitespace = " \t\r\n\v\f";
    while ((start = line.find_first_not_of(whitespace, sp)) != string::npos)
    {
        sp = line.find_first_of(whitespace, start);
        if (sp == string::npos) {
            sp = line.size();
        }
        string token = line.substr(start, sp - start);
        switch (state)
        {
            case 0:
                token = Uppercase(token);
                if (token == "SURFACE") {
                    current.type = Event::SURFACE;
                } else if (token == "SOUND") {
                    current.type = Event::SOUND;
                } else {
                    Log::WriteError("Unknown SFX event type \"%s\"\n", token.c_str());
                    return;
                }
                state = 1;
                break;

            case 1:
                animation = Uppercase(token);
                state = 2;
                break;

            case 2:
            {
                stringstream ss;
                ss << token;
                ss >> time;
                if (ss.fail())
                {
                    Log::WriteError("Invalid SFX event time\n");
                    return;
                }
                time /= 3000;
                state = 3;
                break;
            }

            case 3:
                current.name = token;
                if (current.type == Event::SOUND)
                {
                    // We're done
                    m_maps[animation].insert(make_pair(time, current));
                    return;
                }
                state = 4;
                break;

            case 4:
                current.bone = token;
                
                // We're done
                m_maps[animation].insert(make_pair(time, current));
                return;
        }
    }
}

void Initialize()
{
    // Read file
    ptr<IFile> file = Assets::LoadFile("AnimationSFXMaps.txt", "Data/XML/");
    if (file != NULL)
    {
        char* tmp = new char[file->size()];
        file->read(tmp, file->size());
        string data(tmp, file->size());
        delete[] tmp;

        // Parse file
        string::size_type start, sp = 0;
        static const char* newline = "\n";
        while ((start = data.find_first_not_of(newline, sp)) != string::npos)
        {
            sp = data.find_first_of(newline, start);
            if (sp == string::npos) {
                sp = data.size();
            }
            string line = data.substr(start, sp - start);
            ParseLine(line);
        }
    }
}

void Uninitialize()
{
    m_maps.clear();
}

}
}