#ifndef ASSETS_H
#define ASSETS_H

#include "Assets/Files.h"
#include "Assets/Models.h"
#include "Assets/Animations.h"
#include "RenderEngine/Particles/ParticleSystem.h"

namespace Alamo
{

/*
 * General note for all Assets::LoadXxxxx() functions:
 *
 * All filenames are case-insensitive.
 * Returns NULL if the asset could not be found.
 * Throws an exception if the asset could not be loaded.
 */
namespace Assets
{
    /* Initializes the asset manager. Call this once at program startup.
     *  @basepaths: List of paths to consider when loading files.
     *
     * During initialization, the manager will look for Data/MegaFiles.xml in all
     * paths in the order in the vector, load the MegaFiles references by the XML and
     * construct an internal table of filenames and files. As such, if a file appears
     * in more than one MegaFile, the last one processed will be used.
     */
    void Initialize(const std::vector<std::wstring> &basepaths);

    /* Frees up all resources. Call this at program termination. */
    void Uninitialize();

    /* Loads a file. The manager will first look if the file exists physically in
     * any of the base paths specified during initialization. If not, it will
     * load the file based on its internal table.
     * It will try all of the extensions in the passed array if the file doesn't exist.
     */
    ptr<IFile> LoadFile(const std::string& _filename, const char* prefix = NULL, const char* const* extensions = NULL);

    /* These functions load the various assets. Shaders and textures are returned
     * as raw data, to be used for creating the RenderEngine resources.
     */
    ptr<Model>          LoadModel(const std::string& filename);
    ptr<Animation>      LoadAnimation(const std::string& filename, const Model& model);
    ptr<IFile>          LoadShader(const std::string& filename);
    ptr<IFile>          LoadTexture(const std::string& filename);
    ptr<ParticleSystem> LoadParticleSystem(const std::string& filename);
}

}
#endif