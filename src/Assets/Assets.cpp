#include "Assets/Assets.h"
#include "General/Exceptions.h"
#include "General/Utils.h"
#include "General/XML.h"
#include "General/ExactTypes.h"
#include "General/Log.h"
#include <map>
using namespace std;

#ifndef NDEBUG
// Uncomment the following line if you want full location prints for every loaded file
//#define DEBUG_ASSETS
#endif

namespace Alamo {
namespace Assets {

static const wchar_t* MASTER_INDEX_FILENAME     = L"Data\\MegaFiles.xml";
static const wchar_t* PATCH_MEGAFILE            = L"Data\\Patch.meg";
static const wchar_t* SFX2D_LOCALIZED           = L"Data\\Audio\\SFX\\SFX2D_English.meg";
static const wchar_t* SFX2D_NON_LOCALIZED       = L"Data\\Audio\\SFX\\SFX2D_Non_Localized.meg";
static const wchar_t* SFX3D_NON_LOCALIZED       = L"Data\\Audio\\SFX\\SFX3D_Non_Localized.meg";
static const char*    MAPS_BASE_PATH            = "Data\\Art\\Maps\\";
static const char*    MODELS_BASE_PATH          = "Data\\Art\\Models\\";
static const char*    ANIMATIONS_BASE_PATH      = "Data\\Art\\Models\\";
static const char*    PARTICLESYSTEMS_BASE_PATH = "Data\\Art\\Models\\";
static const char*    SHADERS_BASE_PATH         = "Data\\Art\\Shaders\\";
static const char*    TEXTURES_BASE_PATH        = "Data\\Art\\Textures\\";

// MegaFile parsers
#pragma pack(1)
struct MEGHEADER
{
	uint32_t nStrings;
	uint32_t nFiles;
};

// Represents a file in a MegaFile
struct MEGFILEINFO
{
	uint32_t crc;
	uint32_t index;
	uint32_t size;
	uint32_t start;
	uint32_t nameIndex;
};
#pragma pack()

typedef vector<MEGFILEINFO> FileIndex;

struct MegaFileInfo
{
    wstring     m_filename; // For debugging purposes
    string      m_basepath;
	ptr<IFile>	m_file;
    char*       m_data;
    FileIndex   m_files;
};

typedef vector<MegaFileInfo> MegaFileIndex;

static MegaFileIndex    g_megaFiles;
static vector<wstring>  g_basepaths;

static void IndexMegaFile(const wstring& path, const string& basepath = "")
{
#ifdef DEBUG_ASSETS
    printf("Indexing %ls to \\%s\n", path.c_str(), basepath.c_str());
#endif
    try
    {
        g_megaFiles.push_back(MegaFileInfo());
        MegaFileInfo& megfile = g_megaFiles.back();
        
        megfile.m_filename = path;
        megfile.m_data     = NULL;
        megfile.m_basepath = Uppercase(basepath);

        try
        {
            megfile.m_file = new PhysicalFile(path);

            // Add this megafile to the index
	        MEGHEADER header;
            if (megfile.m_file->read(&header, sizeof(header)) != sizeof(header))
	        {
		        throw ReadException();
	        }

	        //
	        // Read filenames
	        //
    	    unsigned long nStrings = letohl(header.nStrings);
	        vector<unsigned long> filenames(nStrings);
	        unsigned long size = 0;
	        for (unsigned long i = 0; i < nStrings; i++)
	        {
		        uint16_t leLength;
		        if (megfile.m_file->read(&leLength, sizeof(uint16_t)) != sizeof(uint16_t))
		        {
			        throw ReadException();
		        }
                unsigned short length = letohs(leLength);
                filenames[i] = size;
                size += length + 1;

                char* tmp = (char*)realloc(megfile.m_data, size);
                if (tmp == NULL)
                {
                    throw bad_alloc();
                }
                megfile.m_data = tmp;

                if (megfile.m_file->read(&megfile.m_data[filenames[i]], length) != length)
                {
                    throw ReadException();
                }
                megfile.m_data[size - 1] = '\0';
            }

	        //
	        // Read master index table
	        //
	        unsigned long nFiles = letohl(header.nFiles);
            megfile.m_files.resize(nFiles);
            if (megfile.m_file->read(&megfile.m_files[0], nFiles * sizeof(MEGFILEINFO)) != nFiles * sizeof(MEGFILEINFO))
	        {
		        throw ReadException();
	        }

            // Unpack
	        for (unsigned long i = 0; i < nFiles; i++)
	        {
                MEGFILEINFO& fi = megfile.m_files[i];
                fi.crc       = letohl(fi.crc);
                fi.nameIndex = filenames[letohl(fi.nameIndex)];
		        fi.start     = letohl(fi.start);
		        fi.size      = letohl(fi.size);
	        }
        }
        catch (...)
        {
            delete[] megfile.m_data;
            g_megaFiles.resize(g_megaFiles.size() - 1);
            throw;
        }
    }
    catch (IOException&)
    {
    }
}

// Initialize the asset manager
// Basepaths are in the order in which they are to be searched for files
void Initialize(const vector<wstring>& basepaths)
{
    // First clean up the current state
    Uninitialize();

    g_basepaths = basepaths;
    for (vector<wstring>::reverse_iterator path = g_basepaths.rbegin(); path != g_basepaths.rend(); path++)
    {
        // Ensure a trailing backslash on the paths
        if (*path->rbegin() != L'/' && *path->rbegin() != L'\\')
            *path += L"\\";

#ifdef DEBUG_ASSETS
        printf("Asset search path: %ls\n", path->c_str());
#endif

        // Load the master index file
        try
        {
            XMLTree xml;
            ptr<IFile> file = new PhysicalFile(*path + MASTER_INDEX_FILENAME);
            xml.parse(file);

            // Create a file index from all mega files
            const XMLNode* root = xml.getRoot();
            for (size_t i = 0; i < root->getNumChildren(); i++)
            {
                const XMLNode* child = root->getChild(i);
                IndexMegaFile(*path + AnsiToWide(child->getData()));
            }
        }
        // Could not find or parse master index file, carry on
        catch (FileNotFoundException&) {}
        catch (ParseException&) {}

        // Add the sound files
        IndexMegaFile(*path + SFX2D_LOCALIZED,     "DATA\\AUDIO\\SFX\\");
        IndexMegaFile(*path + SFX2D_NON_LOCALIZED, "DATA\\AUDIO\\SFX\\");
        IndexMegaFile(*path + SFX3D_NON_LOCALIZED, "DATA\\AUDIO\\SFX\\");

        // Add the patch mega-file last
        IndexMegaFile(*path + PATCH_MEGAFILE);
    }
}

// Clear the master file index
void Uninitialize()
{
    for (MegaFileIndex::const_iterator p = g_megaFiles.begin(); p != g_megaFiles.end(); p++)
    {
        delete[] p->m_data;
    }
    g_megaFiles.clear();
    g_basepaths.clear();
}

static ptr<IFile> LoadSpecificFile(const MegaFileInfo& megfile, const string& filename)
{
    // The base of the path must match this MegaFile's base path
    const char* name = filename.c_str();
    if (strncmp(name, megfile.m_basepath.c_str(), megfile.m_basepath.length()) == 0)
    {
        name = name + megfile.m_basepath.length();
        unsigned long crc = crc32(name, filename.length() - megfile.m_basepath.length());
        
        int low = 0, high = (int)megfile.m_files.size() - 1;
        while (high >= low)
        {
            int mid = (low + high) / 2;
            if (crc == megfile.m_files[mid].crc)
            {
                const MEGFILEINFO* fi = &megfile.m_files[mid];
                while (mid > 0 && (fi - 1)->crc == crc) { mid--; fi--; }
                while (mid != megfile.m_files.size() && fi->crc == crc)
                {
                    if (strcmp(megfile.m_data + fi->nameIndex, name) == 0)
                    {
                        // Found it
#ifdef DEBUG_ASSETS
                        printf("Loaded file: %ls:%s\n", megfile.m_filename.c_str(), name);
#endif
                        return new SubFile(megfile.m_file, name, fi->start, fi->size);
                    }
                    fi++;
                    mid++;
                }
                break;
            }
            if (crc < megfile.m_files[mid].crc) high = mid - 1;
            else                                low  = mid + 1;
        }
    }
    // File not found in this MegaFile
    return NULL;
}

static ptr<IFile> LoadSpecificFile(string filename, const char* prefix)
{
    // First check if the file physically exists
    if (filename.find_first_of(":") != string::npos)
    {
        // Absolute path on Windows
        try
        {
            ptr<IFile> file = new PhysicalFile(AnsiToWide(filename));
#ifdef DEBUG_ASSETS
            printf("Loaded file: %%s\n", filename.c_str());
#endif
            return file;
        }
        catch (FileNotFoundException&)
        {
        }

        // Strip everything before the last backslash, if any
        string::size_type ofs = filename.find_last_of("\\");
        if (ofs != string::npos)
        {
            filename = filename.substr(ofs + 1);
        }
    }

    // Relative path
    if (prefix != NULL)
    {
        filename = prefix + filename;
    }

    // Replace slash with backslash
    string::size_type ofs = 0;
    while ((ofs = filename.find_first_of("/", ofs)) != string::npos)
    {
        filename[ofs] = '\\';
    }

    // Search physical locations first
    for (vector<wstring>::const_iterator i = g_basepaths.begin(); i != g_basepaths.end(); i++)
    {
        try
        {
            ptr<IFile> file = new PhysicalFile(*i + AnsiToWide(filename));
#ifdef DEBUG_ASSETS
            printf("Loaded file: %ls%s\n", i->c_str(), filename.c_str());
#endif
            return file;
        }
        catch (FileNotFoundException&)
        {
            // Not found here, carry on
        }
    }
    
    // File does not exist physically, check master file index
    transform(filename.begin(), filename.end(), filename.begin(), ::toupper);
    for (MegaFileIndex::const_reverse_iterator p = g_megaFiles.rbegin(); p != g_megaFiles.rend(); p++)
    {
        ptr<IFile> file = LoadSpecificFile(*p, filename);
        if (file != NULL)
        {
            return file;
        }
    }
    return NULL;
}

ptr<IFile> LoadFile(const string& filename, const char* prefix, const char* const* extensions)
{
    if (!filename.empty())
    {
        string _filename = filename;
        do
        {
            ptr<IFile> file = LoadSpecificFile(_filename, prefix);
            if (file != NULL)
            {
                return file;
            }

            // File not found with this extensions
            if (extensions != NULL && *extensions != NULL)
            {
	            // Try another extension
                string::size_type ofs = filename.find_last_of(".\\/");
                _filename = (ofs != string::npos && filename[ofs] == '.') ? filename.substr(0, ofs) : filename;
                _filename = _filename + "." + *extensions;
            }
        } while (extensions != NULL && *extensions++ != NULL);
    }
	// File not found for all extensions
	return NULL;
}

ptr<Model> LoadModel(const string& filename)
{
    static const char* ModelExtensions[] = {"alo", NULL};

    ptr<IFile> file = LoadFile(filename, MODELS_BASE_PATH, ModelExtensions);
    return (file == NULL) ? NULL : new Model(file);
}

ptr<ParticleSystem> LoadParticleSystem(const string& filename)
{
    static const char* ParticleSystemExtensions[] = {"alo", NULL};

    ptr<IFile> file = LoadFile(filename, PARTICLESYSTEMS_BASE_PATH, ParticleSystemExtensions);
    if (file != NULL) try {
        return new ParticleSystem(file, filename);
    } catch (wexception&) {
    }
    return NULL;
}

ptr<Animation> LoadAnimation(const string& filename, const Model& model)
{
    static const char* AnimationExtensions[] = {"ala", NULL};

    ptr<IFile> file = LoadFile(filename, ANIMATIONS_BASE_PATH, AnimationExtensions);
    if (file != NULL) try {
        return new Animation(file, model);
    } catch (wexception&) {
    }
    return NULL;
}

ptr<IFile> LoadShader(const string& filename)
{
    static const char* ShaderExtensions[] = {"fx", "fxo", NULL};

    return LoadFile(filename, SHADERS_BASE_PATH, ShaderExtensions);
}

ptr<IFile> LoadTexture(const string& filename)
{
    static const char* TextureExtensions[] = {"tga", "dds", NULL};

    return LoadFile(filename, TEXTURES_BASE_PATH, TextureExtensions);
}

}
}