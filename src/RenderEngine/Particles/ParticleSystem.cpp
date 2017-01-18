#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/CreatorPlugins.h"
#include "RenderEngine/Particles/KillerPlugins.h"
#include "RenderEngine/Particles/TranslaterPlugins.h"
#include "RenderEngine/Particles/RendererPlugins.h"
#include "RenderEngine/Particles/ModifierPlugins.h"
#include "General/Exceptions.h"
#include "General/Log.h"
using namespace std;

namespace Alamo
{

// Plugin types
static const int PLUGIN_CREATOR    = 1;
static const int PLUGIN_TRANSLATER = 2;
static const int PLUGIN_KILLER     = 3;
static const int PLUGIN_RENDERER   = 4;
static const int PLUGIN_MODIFIER   = 5;

bool    Plugin::ReadBool()    { return (m_reader->readByte() != 0); }
float   Plugin::ReadFloat()   { return m_reader->readFloat();   }
Vector3 Plugin::ReadVector3() { return m_reader->readVector3(); }
Vector4 Plugin::ReadVector4() { return m_reader->readVector4(); }
Color   Plugin::ReadColor()   { return m_reader->readColorRGBA(); }
int     Plugin::ReadInteger() { return m_reader->readInteger(); }
string  Plugin::ReadString()  { return m_reader->readString();  }
void    Plugin::ReadProperty(PluginProperty& prop) { prop.Read(*m_reader); }

void Plugin::ReadParameters(Alamo::ChunkReader &reader)
{
    m_reader = &reader;
    if (reader.size() > 0)
    {
        ChunkType type;
        while ((type = reader.next()) == 0)
        {
            int id = reader.readInteger();
            Verify(reader.next() == 1);
            CheckParameter(id);
        }
        Verify(type == -1);
    }
}

Plugin::Plugin(ParticleSystem::Emitter& emitter) : m_emitter(emitter) {}
CreatorPlugin::CreatorPlugin(ParticleSystem::Emitter& emitter) : Plugin(emitter) {}
RendererPlugin::RendererPlugin(ParticleSystem::Emitter& emitter) : Plugin(emitter) {}
TranslaterPlugin::TranslaterPlugin(ParticleSystem::Emitter& emitter) : Plugin(emitter) {}
KillerPlugin::KillerPlugin(ParticleSystem::Emitter& emitter) : Plugin(emitter) {}
ModifierPlugin::ModifierPlugin(ParticleSystem::Emitter& emitter) : Plugin(emitter) {}

//
// Old Emitter properties
//
struct ParticleSystem::Emitter::OldEmitter
{
	// Group types
	static const int GT_EXACT        = 0;
	static const int GT_BOX          = 1;
	static const int GT_CUBE         = 2;
	static const int GT_SPHERE       = 3;
	static const int GT_CYLINDER     = 4;
	static const int NUM_GROUP_TYPES = 5;

	// Group IDs
	static const int GROUP_SPEED    = 0;
	static const int GROUP_LIFETIME = 1;
	static const int GROUP_POSITION = 2;
	static const int NUM_GROUPS     = 3;

	// Track IDs
	static const int TRACK_RED_CHANNEL    = 0;
	static const int TRACK_GREEN_CHANNEL  = 1;
	static const int TRACK_BLUE_CHANNEL   = 2;
	static const int TRACK_ALPHA_CHANNEL  = 3;
	static const int TRACK_SCALE          = 4;
	static const int TRACK_INDEX          = 5;
	static const int TRACK_ROTATION_SPEED = 6;
	static const int NUM_TRACKS           = 7;

    // Blend modes
    static const int BLEND_NONE                = 0;
    static const int BLEND_ADDITIVE            = 1;
    static const int BLEND_TRANSPARENT         = 2;
    static const int BLEND_INVERSE             = 3;
    static const int BLEND_DEPTH_ADDITIVE      = 4;
    static const int BLEND_DEPTH_TRANSPARENT   = 5;
    static const int BLEND_DEPTH_INVERSE       = 6;
    static const int BLEND_DIFFUSE_TRANSPARENT = 7;
    static const int BLEND_STENCIL_DARKEN      = 8;
    static const int BLEND_STENCIL_DARKEN_BLUR = 9;
    static const int BLEND_HEAT                = 10;
    static const int BLEND_BUMP                = 11;
    static const int BLEND_DECAL_BUMP          = 12;
    static const int BLEND_SCANLINES           = 13;
    static const int NUM_BLEND_MODES           = 14;

    // Ground behavior
    static const int GROUND_NONE      = 0;
    static const int GROUND_DISAPPEAR = 1;
    static const int GROUND_BOUNCE    = 2;
    static const int GROUND_STICK     = 3;

    // Emit mode
    static const int EMIT_DISABLE       = 0;
    static const int EMIT_RANDOM_VERTEX = 1;
    static const int EMIT_RANDOM_MESH   = 2;
    static const int EMIT_EVERY_VERTEX  = 3;

	// Emitter hierarchy
	size_t   spawnOnDeath;
	size_t   spawnDuringLife;
	Emitter* parent;

	std::string name;
	std::string colorTexture;
	std::string normalTexture;

	// Random parameter groups
    #pragma pack(1)
	struct Group
	{
		unsigned int type;
		float        minX, minY, minZ;
		float		 maxX, maxY, maxZ;
		float		 sideLength;
		float		 sphereRadius;
		unsigned int sphereEdge;
		float		 cylinderRadius;
		unsigned int cylinderEdge;
		float		 cylinderHeight;
		float		 valX, valY, valZ;
	};
    #pragma pack()
	Group groups[NUM_GROUPS];

    // The tracks
    typedef Alamo::Track<float> Track;
    Track tracks[NUM_TRACKS];

	// Properties
    bool  linkToSystem, objectSpaceAcceleration, doColorAddGrayscale, affectedByWind;
	bool  isHeatParticle, isWeatherParticle, hasTail, noDepthTest, randomRotation;
	bool  randomRotationDirection, isWorldOriented, useBursts;
	int   emitFromMesh;
	float gravity, weatherFadeoutDistance, lifetime, initialDelay, burstDelay;
	float inwardSpeed, inwardAcceleration, randomScalePerc, weatherCubeSize, tailSize;
	float randomLifetimePerc, parentLinkStrength, weatherCubeDistance, randomRotationAverage;
    float randomRotationVariance, bounciness, freezeTime, skipTime, emitFromMeshOffset;
	Vector3 acceleration;
	Color   randomColors;
	unsigned long nBursts, blendMode, textureSize, nParticlesPerSecond;
	unsigned long nTriangles, nParticlesPerBurst, groundBehavior;

    static bool ConvertGroup(PropertyGroup& dest, const Group& src)
    {
        switch (src.type)
        {
            case GT_EXACT:
                dest.m_type  = PropertyGroup::POINT;
                dest.m_point = Vector3(src.valX, src.valY, src.valZ);
                return false;

            case GT_BOX:
                dest.m_type        = PropertyGroup::RANGE;
                dest.m_minPosition = Vector3(src.minX, src.minY, src.minZ);
                dest.m_maxPosition = Vector3(src.maxX, src.maxY, src.maxZ);
                return false;

            case GT_CUBE:
                dest.m_type        = PropertyGroup::RANGE;
                dest.m_minPosition = Vector3(-src.sideLength/2, -src.sideLength/2, -src.sideLength/2);
                dest.m_maxPosition = Vector3( src.sideLength/2,  src.sideLength/2,  src.sideLength/2);
                return false;

            case GT_SPHERE:
                dest.m_type       = PropertyGroup::SPHERE;
                dest.m_radius.min = 0;
                dest.m_radius.max = src.sphereRadius;
                return src.sphereEdge != 0;

            case GT_CYLINDER:
                dest.m_type          = PropertyGroup::CYLINDER;
                dest.m_cylRadius     = src.cylinderRadius;
                dest.m_cylHeight.min = 0;
                dest.m_cylHeight.max = src.cylinderHeight;
                return src.cylinderEdge != 0;
        }
        return false;
    }

    void SetDefaults()
    {
	    spawnOnDeath    = -1;
	    spawnDuringLife = -1;
	    parent          = NULL;

	    colorTexture  = "p_particle_master.tga";
	    normalTexture = "p_particle_depth_master.tga";

	    groups[0].type = 0;
	    groups[0].minX = 0.0f; groups[0].minY = 0.0f; groups[0].minZ = 0.0f;
	    groups[0].maxX = 0.0f; groups[0].maxY = 0.0f; groups[0].maxZ = 0.0f;
	    groups[0].valX = 0.0f; groups[0].valY = 0.0f; groups[0].valZ = 0.0f;
	    groups[0].cylinderEdge   = false;
	    groups[0].cylinderHeight = 0.0f;
	    groups[0].cylinderRadius = 0.0f;
	    groups[0].sphereEdge     = false;
	    groups[0].sphereRadius   = 0.0f;
	    groups[0].sideLength     = 0.0f;
	    groups[2] = groups[1] = groups[0];

	    for (int i = 0; i < NUM_TRACKS; i++)
	    {
            float value;
            switch (i)
            {
                case TRACK_RED_CHANNEL:
                case TRACK_GREEN_CHANNEL:
                case TRACK_BLUE_CHANNEL:
                case TRACK_ALPHA_CHANNEL:   value =  1.0f; break;
                case TRACK_SCALE:           value = 20.0f; break;
                case TRACK_INDEX:           value =  0.0f; break;
                case TRACK_ROTATION_SPEED:  value =  0.0f; break;
            }
            tracks[i].m_interpolation = (i == TRACK_INDEX) ? Track::IT_STEP : Track::IT_LINEAR;
            tracks[i].push_back(Track::Key(0.0f, value));
		    tracks[i].push_back(Track::Key(1.0f, value));
	    }
        // Point Green, Blue and Alpha tracks to Red
        tracks[1] = tracks[0];
        tracks[2] = tracks[0];
        tracks[3] = tracks[0];

        linkToSystem            = false;
	    randomRotationDirection	= false;
	    doColorAddGrayscale		= false;
	    affectedByWind			= false;
	    isHeatParticle			= false;
	    isWeatherParticle		= false;
	    hasTail					= false;
	    noDepthTest				= false;
	    randomRotation			= false;
	    isWorldOriented			= false;
	    useBursts				= false;
	    objectSpaceAcceleration = false;
	    gravity                =   0.0f;
	    lifetime			   =   1.0f;
	    initialDelay		   =   0.0f;
	    burstDelay			   =   1.0f;
	    inwardAcceleration     =   0.0f;
	    inwardSpeed			   =   0.0f;
	    acceleration   		   = Vector3(0,0,0);
	    randomScalePerc		   =   0.0f;
	    randomLifetimePerc	   =   0.0f;
	    weatherCubeSize        = 500.0f;
	    tailSize               =  50.0f;
	    parentLinkStrength    =    0.0f;
	    weatherCubeDistance    =   0.0f;
	    randomRotationVariance =   0.0f;
	    randomRotationAverage  =   0.0f;
	    randomColors   		   = Color(0,0,0,0);
	    bounciness			   =   0.2f;
	    freezeTime			   =   0.0f;
	    skipTime			   =   0.0f;
	    emitFromMeshOffset     =   0.50f;
	    weatherFadeoutDistance = 100.00f;
	    emitFromMesh        = EMIT_DISABLE;
	    blendMode           = 1;
	    textureSize         = 64;
	    nBursts             = 0;
	    nParticlesPerSecond = 1;
	    nTriangles          = 2;
	    nParticlesPerBurst  = 1;
	    groundBehavior      = 0;
    }

    void ReadProperties(ChunkReader& reader)
    {
	    bool useLinkStrength = false;

	    ChunkType type;
	    while ((type = reader.nextMini()) != -1)
	    {
		    switch (type)
		    {
			    case 0x04: blendMode				= reader.readInteger() % NUM_BLEND_MODES; break;
			    case 0x05: nTriangles				= reader.readInteger() + 1; break;
			    case 0x07: useBursts				= reader.readByte() != 0; break;
			    case 0x08: linkToSystem 			= reader.readByte() != 0; break;
			    case 0x09: inwardSpeed				= -reader.readFloat(); break;
			    case 0x0A: acceleration             = reader.readVector3(); break;
			    case 0x0B: inwardAcceleration       = -reader.readFloat();   break;
			    case 0x0C: gravity					= reader.readFloat(); break;
			    case 0x0F: lifetime					= reader.readFloat(); break;
			    case 0x10: textureSize				= reader.readInteger(); break;
			    case 0x12: randomScalePerc			= reader.readFloat(); break;
			    case 0x13: randomLifetimePerc		= reader.readFloat(); break;
			    case 0x14: reader.readInteger(); break; // Read but ignore index
			    case 0x17: randomRotationVariance	= fabs(reader.readFloat()); break;
			    case 0x23: randomRotationDirection	= reader.readByte() != 0; break;
			    case 0x24: initialDelay				= reader.readFloat(); break;
			    case 0x25: burstDelay				= reader.readFloat(); break;
			    case 0x26: nParticlesPerBurst		= reader.readInteger(); break;
			    case 0x27: nBursts					= reader.readInteger(); if (nBursts == -1) nBursts = 0; break;
			    case 0x28: parentLinkStrength		= reader.readFloat(); break;
			    case 0x2A: nParticlesPerSecond		= reader.readInteger(); break;
			    case 0x2C: randomColors             = reader.readColorRGBA(); break;
			    case 0x2D: doColorAddGrayscale		= reader.readByte() != 0; break;
			    case 0x2E: isWorldOriented			= reader.readByte() != 0; break;
			    case 0x2F: groundBehavior			= reader.readInteger(); break;
			    case 0x30: bounciness				= reader.readFloat(); break;
			    case 0x31: affectedByWind			= reader.readByte() != 0; break;
			    case 0x32: freezeTime				= reader.readFloat(); break;
			    case 0x33: skipTime					= reader.readFloat(); break;
			    case 0x34: emitFromMesh             = reader.readInteger(); break;
			    case 0x35: objectSpaceAcceleration  = reader.readByte() != 0; break;
			    case 0x3B: isHeatParticle			= reader.readByte() != 0; break;
			    case 0x3C: emitFromMeshOffset       = reader.readFloat(); break;
			    case 0x3D: isWeatherParticle		= reader.readByte() != 0; break;
			    case 0x3E: weatherCubeSize			= reader.readFloat(); break;
			    case 0x40: weatherFadeoutDistance   = reader.readFloat();   break;
			    case 0x41: hasTail					= reader.readByte() != 0; break;
			    case 0x42: tailSize					= reader.readFloat(); break;
			    case 0x43: useLinkStrength			= reader.readByte() != 0; break;
			    case 0x46: noDepthTest				= reader.readByte() != 0; break;
			    case 0x47: weatherCubeDistance		= reader.readFloat(); break;
			    case 0x48: randomRotation			= reader.readByte() != 0; break;

			    case 0x2B: case 0x06: case 0x11: case 0x15: case 0x3F: case 0x44: case 0x49:
                    break;

			    default:
				    throw BadFileException();
		    }
	    }

	    if (!useLinkStrength) parentLinkStrength  = 0.0f;
    }

    void ReadGroups(ChunkReader& reader)
    {
	    for (int i = 0; i < NUM_GROUPS; i++)
	    {
		    Verify(reader.next() == 0x1100);
		    Verify(reader.next() == 0x1101);
		    reader.read(&groups[i], sizeof(Group));
		    Verify(reader.next() == -1);
	    }

	    Verify(reader.next() == -1);
    }

    void ReadTracks(ChunkReader& reader)
    {
	    // Read channel tracks
	    for (int i = 0; i < 4; i++)
	    {
            tracks[i].clear();

		    Verify(reader.next() == 0x00);
		    Verify(reader.nextMini() == 0x02);
            Track::Key first = make_pair(0.0f, reader.readByte() / 255.0f);

		    Verify(reader.nextMini() == 0x03);
		    Track::Key last = Track::Key(1.0f, reader.readByte() / 255.0f);

		    Verify(reader.nextMini() == 0x04);
            switch (reader.readInteger())
            {
                default:
                case 0: tracks[i].m_interpolation = Track::IT_LINEAR; break;
                case 1: tracks[i].m_interpolation = Track::IT_SMOOTH; break;
                case 2: tracks[i].m_interpolation = Track::IT_STEP; break;
            }
		    Verify(reader.nextMini() == -1);

		    tracks[i].push_back(first);
		    Verify(reader.next() == 0x01);

		    ChunkType type;
		    while ((type = reader.nextMini()) == 5)
		    {
			    Track::Key key;
			    key.second = reader.readInteger() / 255.0f;
			    key.first  = reader.readFloat();
			    Verify(key.second >= 0.0f && key.second <= 1.0f && key.first <= 1.0f && key.first >= tracks[i].rbegin()->first);
			    tracks[i].push_back(key);
		    }
		    Verify(type == -1);
		    tracks[i].push_back(last);
	    }

	    // Read other tracks
	    for (int i = 4; i < 7; i++)
	    {
		    tracks[i].clear();

		    Verify(reader.next() == 0x00);
		    Verify(reader.nextMini() == 0x02);
		    Track::Key first = Track::Key(0.0f, reader.readFloat());

		    Verify(reader.nextMini() == 0x03);
		    Track::Key last = Track::Key(1.0f, reader.readFloat());

		    Verify(reader.nextMini() == 0x04);
            switch (reader.readInteger())
            {
                default:
                case 0: tracks[i].m_interpolation = Track::IT_LINEAR; break;
                case 1: tracks[i].m_interpolation = Track::IT_SMOOTH; break;
                case 2: tracks[i].m_interpolation = Track::IT_STEP; break;
            }
		    Verify(reader.nextMini() == -1);

		    tracks[i].push_back(first);
		    Verify(reader.next() == 0x01);
		    ChunkType type;
		    while ((type = reader.nextMini()) == 5)
		    {
			    Track::Key key;
                key.second = reader.readFloat();
			    key.first  = reader.readFloat();
			    Verify(key.first <= 1.0f && tracks[i].rbegin()->first <= key.first);
			    tracks[i].push_back(key);
		    }
		    Verify(type == -1);
		    tracks[i].push_back(last);
	    }
	    Verify(reader.next() == -1);
    }

    OldEmitter(ChunkReader& reader, Emitter* emitter, DependencyList& dependencies)
    {
        SetDefaults();

        Verify(reader.next() == 0x02); ReadProperties(reader);
        Verify(reader.next() == 0x03); colorTexture = reader.readString();
        Verify(reader.next() == 0x16); name         = reader.readString();
        Verify(reader.next() == 0x29); ReadGroups(reader);
        Verify(reader.next() == 0x01); ReadTracks(reader);

        if (randomRotation)
        {
	        // Sanitize random rotation data
	        randomRotationAverage  = tracks[TRACK_ROTATION_SPEED].begin()->second;
	        if (randomRotationAverage > 0)
	        {
		        randomRotationVariance = randomRotationVariance / randomRotationAverage;
		        randomRotationAverage  = randomRotationAverage - (int)randomRotationAverage;
	        }
	        else
	        {
		        randomRotationVariance = 0.0f;
	        }
        }

        ChunkType type = reader.next();
        if (type == 0x36)
        {
	        Verify(reader.nextMini() == 0x37); spawnOnDeath    = (long)reader.readInteger();
	        Verify(reader.nextMini() == 0x39); spawnDuringLife = (long)reader.readInteger();
	        Verify(reader.nextMini() == -1);
	        type = reader.next();

            if (spawnDuringLife != -1) {
                dependencies.resize(max(dependencies.size(), spawnDuringLife + 1), pair<Emitter*,bool>(NULL,false));
                dependencies[spawnDuringLife] = make_pair(emitter, false);
            }

            if (spawnOnDeath != -1) {
                dependencies.resize(max(dependencies.size(), spawnOnDeath + 1), pair<Emitter*,bool>(NULL,false));
                dependencies[spawnOnDeath] = make_pair(emitter, true);
            }
        }

        if (type == 0x45)
        {
	        normalTexture = reader.readString();
	        type = reader.next();
        }
        Verify(type == -1);
    }
};

//
// Emitter class
//
ParticleSystem::Emitter* ParticleSystem::Emitter::RegisterSpawn(Emitter* emitter, SpawnEvent e)
{
    Emitter* next = m_spawns[e];
    m_spawns[e] = emitter;
    return next;
}

Plugin* ParticleSystem::Emitter::ReadPlugin(ChunkReader& reader, int type)
{
    #define BEGIN_PLUGIN_LIST(id) switch (id) {
    #define DEFINE_PLUGIN(pluginid, classname, plugintype) \
        case (pluginid): if (type == (plugintype)) plugin = new classname(*this); break;
    #define END_PLUGIN_LIST default: reader.skip(); break; }

    // Read the plugin ID
    Verify(reader.next() == 0);
    unsigned long id = reader.readInteger();
    Verify(reader.next() == 1);
    Verify(reader.next() == 0);
    Plugin* plugin = NULL;

    // Create the plugin based on the ID
    BEGIN_PLUGIN_LIST(id)
    DEFINE_PLUGIN( 0, PointCreatorPlugin,                  PLUGIN_CREATOR);
    DEFINE_PLUGIN( 1, TrailCreatorPlugin,                  PLUGIN_CREATOR);
    DEFINE_PLUGIN( 2, SphereCreatorPlugin,                 PLUGIN_CREATOR);
    DEFINE_PLUGIN( 3, BoxCreatorPlugin,                    PLUGIN_CREATOR);
    DEFINE_PLUGIN( 4, TorusCreatorPlugin,                  PLUGIN_CREATOR);
    DEFINE_PLUGIN( 5, MeshCreatorPlugin,                   PLUGIN_CREATOR);
    // "Terrain" (6) is (as of yet) unsupported
    DEFINE_PLUGIN( 7, LinearSizeModifierPlugin,            PLUGIN_MODIFIER);
    DEFINE_PLUGIN( 8, LinearColorModifierPlugin,           PLUGIN_MODIFIER);
    DEFINE_PLUGIN( 9, LinearRotationModifierPlugin,        PLUGIN_MODIFIER);
    DEFINE_PLUGIN(10, AccelerationModifierPlugin,          PLUGIN_MODIFIER);
    DEFINE_PLUGIN(11, AttractAccelerationModifierPlugin,   PLUGIN_MODIFIER);
    DEFINE_PLUGIN(12, TurbulenceModifierPlugin,            PLUGIN_MODIFIER);
    DEFINE_PLUGIN(13, VortexModifierPlugin,                PLUGIN_MODIFIER);
    DEFINE_PLUGIN(14, ConstantUVModifierPlugin,            PLUGIN_MODIFIER);
    DEFINE_PLUGIN(15, KeyedSizeModifierPlugin,             PLUGIN_MODIFIER);
    DEFINE_PLUGIN(16, KeyedRotationModifierPlugin,         PLUGIN_MODIFIER);
    DEFINE_PLUGIN(17, KeyedColorModifierPlugin,            PLUGIN_MODIFIER);
    DEFINE_PLUGIN(18, KeyedUVModifierPlugin,               PLUGIN_MODIFIER);
    DEFINE_PLUGIN(19, AgeKillerPlugin,                     PLUGIN_KILLER);
    DEFINE_PLUGIN(20, RadiusKillerPlugin,                  PLUGIN_KILLER);
    DEFINE_PLUGIN(21, TerrainKillerPlugin,                 PLUGIN_KILLER);
    DEFINE_PLUGIN(22, BillboardRendererPlugin,             PLUGIN_RENDERER);
    DEFINE_PLUGIN(23, LineRendererPlugin,                  PLUGIN_RENDERER);
    DEFINE_PLUGIN(24, ChainRendererPlugin,                 PLUGIN_RENDERER);
    DEFINE_PLUGIN(25, VolumetricRendererPlugin,            PLUGIN_RENDERER);
    DEFINE_PLUGIN(26, EmitterTranslaterPlugin,             PLUGIN_TRANSLATER);
    DEFINE_PLUGIN(27, WorldTranslaterPlugin,               PLUGIN_TRANSLATER);
    DEFINE_PLUGIN(28, XYAlignedRendererPlugin,             PLUGIN_RENDERER);
    DEFINE_PLUGIN(29, VelocityAlignedRendererPlugin,       PLUGIN_RENDERER);
    DEFINE_PLUGIN(30, RandomUVModifierPlugin,              PLUGIN_MODIFIER);
    DEFINE_PLUGIN(31, KeyedAccelerationModifierPlugin,     PLUGIN_MODIFIER);
    DEFINE_PLUGIN(32, KeyedFrictionModifierPlugin,         PLUGIN_MODIFIER);
    DEFINE_PLUGIN(33, SlottedRandomUVModifierPlugin,       PLUGIN_MODIFIER);
    DEFINE_PLUGIN(34, ShapeCreatorPlugin,                  PLUGIN_CREATOR);
    DEFINE_PLUGIN(35, EnhancedMeshCreatorPlugin,           PLUGIN_CREATOR);
    DEFINE_PLUGIN(36, StretchedTextureChainRendererPlugin, PLUGIN_RENDERER);
    DEFINE_PLUGIN(37, BumpMapRendererPlugin,               PLUGIN_RENDERER);
    DEFINE_PLUGIN(38, HeatSaturationRendererPlugin,        PLUGIN_RENDERER);
    DEFINE_PLUGIN(39, EnhancedTrailCreatorPlugin,          PLUGIN_CREATOR);
    DEFINE_PLUGIN(40, DeathCreatorPlugin,                  PLUGIN_CREATOR);
    DEFINE_PLUGIN(41, AttractorModifierPlugin,             PLUGIN_MODIFIER);
    DEFINE_PLUGIN(42, SpeedLimitModifierPlugin,            PLUGIN_MODIFIER);
    DEFINE_PLUGIN(43, AlignedShapeCreatorPlugin,           PLUGIN_CREATOR);
    DEFINE_PLUGIN(44, TargetKillerPlugin,                  PLUGIN_KILLER);
    DEFINE_PLUGIN(45, DistanceKeyTimeModifierPlugin,       PLUGIN_MODIFIER);
    DEFINE_PLUGIN(46, LinearSpeedModifierPlugin,           PLUGIN_MODIFIER);
    DEFINE_PLUGIN(47, XYAlignedChainRendererPlugin,        PLUGIN_RENDERER);
    DEFINE_PLUGIN(48, LinearRotationRateModifierPlugin,    PLUGIN_MODIFIER);
    DEFINE_PLUGIN(49, KeyedRotationRateModifierPlugin,     PLUGIN_MODIFIER);
    DEFINE_PLUGIN(50, OutwardVelocityShapeCreatorPlugin,   PLUGIN_CREATOR);
    DEFINE_PLUGIN(51, TerrainBounceModifierPlugin,         PLUGIN_MODIFIER);
    DEFINE_PLUGIN(52, KitesRendererPlugin,                 PLUGIN_RENDERER);
    DEFINE_PLUGIN(53, ColorVarianceModifierPlugin,         PLUGIN_MODIFIER);
    DEFINE_PLUGIN(54, WindAccelerationModifierPlugin,      PLUGIN_MODIFIER);
    DEFINE_PLUGIN(55, TorqueModifierPlugin,                PLUGIN_MODIFIER);
    DEFINE_PLUGIN(56, AxisAttractorModifierPlugin,         PLUGIN_MODIFIER);
    DEFINE_PLUGIN(57, ConstantSizeModifierPlugin,          PLUGIN_MODIFIER);
    DEFINE_PLUGIN(58, ConstantRotationModifierPlugin,      PLUGIN_MODIFIER);
    DEFINE_PLUGIN(59, ConstantColorModifierPlugin,         PLUGIN_MODIFIER);
    DEFINE_PLUGIN(60, HardwareSpawnerCreatorPlugin,        PLUGIN_CREATOR);
    DEFINE_PLUGIN(61, HardwareBillboardsRendererPlugin,    PLUGIN_RENDERER);
    DEFINE_PLUGIN(62, HardwareStomperKillerPlugin,         PLUGIN_KILLER);
    END_PLUGIN_LIST
    Verify(plugin != NULL);

    // Read the plugin parameters
    plugin->ReadParameters(reader);

    Verify(reader.next() == -1);
    Verify(reader.next() == -1);
    
    return plugin;
}

//
// Emitter class
//
void ParticleSystem::Emitter::Initialize()
{
    m_next = m_creator->Initialize();
}

void ParticleSystem::Emitter::ConvertOldEmitter(const OldEmitter& e, Emitter* dependency, bool onDeath)
{
    // TODO
    // * Bumpmap and depth renderers?
    // * Properties:
	//   - float parentLinkStrength
    //   - float freezeTime;

    static const char* ParticleShaderNames[14] = {
        "Engine/PrimOpaque.fx",
        "Engine/PrimAdditive.fx",
        "Engine/PrimAlpha.fx",
        "Engine/PrimModulate.fx",
        "Engine/PrimDepthSpriteAdditive.fx",
        "Engine/PrimDepthSpriteAlpha.fx",
        "Engine/PrimDepthSpriteModulate.fx",
        "Engine/PrimDiffuseAlpha.fx",
        "Engine/StencilDarken.fx",
        "Engine/StencilDarkenFinalBlur.fx",
        "Engine/PrimHeat.fx",
        "Engine/PrimParticleBumpAlpha.fx",
        "Engine/PrimDecalBumpAlpha.fx",
        "Engine/PrimAlphaScanlines.fx",
    };
    const char* shaderName = ParticleShaderNames[e.blendMode];

    m_name = e.name;

    if (e.hasTail && (e.isHeatParticle || e.isWorldOriented))
    {
        Log::WriteInfo("Emitter in \"%s\" uses multiple exclusive renderers (Kites with Heat or XY Aligned)", this->m_system.GetName().c_str());
    }

    if (e.isHeatParticle) {
        m_renderer = new HeatSaturationRendererPlugin(*this, e.colorTexture, shaderName, e.noDepthTest, e.isWorldOriented);
    } else if (e.isWorldOriented) {
        m_renderer = new XYAlignedRendererPlugin(*this, e.colorTexture, shaderName, e.noDepthTest);
    } else if (e.hasTail) {
        m_renderer = new KitesRendererPlugin(*this, e.colorTexture, shaderName, e.noDepthTest, e.tailSize);
    } else {
        m_renderer = new BillboardRendererPlugin(*this, e.colorTexture, shaderName, e.noDepthTest);
    }  

    ShapeCreator creator;
    creator.m_preSimulateSeconds     = e.skipTime;
    creator.m_particlePerInterval    = (float)(e.useBursts ? e.nParticlesPerBurst : e.nParticlesPerSecond);
    creator.m_spawnInterval          = e.useBursts ? e.burstDelay : 1.0f;
    creator.m_bursting               = e.useBursts;
    creator.m_startDelay             = e.initialDelay;
    creator.m_stopTime               = e.useBursts ? e.nBursts * e.burstDelay : 0;
    creator.m_hollowPosition         = e.ConvertGroup(creator.m_position, e.groups[OldEmitter::GROUP_POSITION]);
    creator.m_hollowVelocity         = e.ConvertGroup(creator.m_velocity, e.groups[OldEmitter::GROUP_SPEED]);
    creator.m_localVelocity          = true;
    creator.m_inheritVelocity        = e.parentLinkStrength != 0.0f;
    creator.m_inheritedVelocityScale = e.parentLinkStrength;
    creator.m_maxInheritedVelocity   = FLT_MAX;
    creator.m_hiresEmission          = false;
    creator.m_burstOnShown           = true;
    
    if (e.emitFromMesh != OldEmitter::EMIT_DISABLE) {
        int spawnLocation;
        switch (e.emitFromMesh)
        {
            case OldEmitter::EMIT_EVERY_VERTEX:  spawnLocation = EnhancedMeshCreatorPlugin::MESH_ALL_VERTICES;   break;
            case OldEmitter::EMIT_RANDOM_MESH:   spawnLocation = EnhancedMeshCreatorPlugin::MESH_RANDOM_SURFACE; break;
            case OldEmitter::EMIT_RANDOM_VERTEX:
            default:                             spawnLocation = EnhancedMeshCreatorPlugin::MESH_RANDOM_VERTEX;  break;
        }
        m_creator = new EnhancedMeshCreatorPlugin(*this, creator, spawnLocation, e.emitFromMeshOffset);
    } else if (dependency == NULL) {
        m_creator = new ShapeCreatorPlugin(*this, creator);
    } else if(onDeath) {
        m_creator = new DeathCreatorPlugin(*this, creator, dependency);
    } else {
        m_creator = new EnhancedTrailCreatorPlugin(*this, creator, dependency);
    }

    if (e.linkToSystem) {
        m_translater = new EmitterTranslaterPlugin(*this);
    } else {
        m_translater = new WorldTranslaterPlugin(*this);
    }

    if (e.groundBehavior == OldEmitter::GROUND_DISAPPEAR) {
        m_killer = new TerrainKillerPlugin(*this, e.lifetime, e.randomLifetimePerc);
    } else {
        m_killer = new AgeKillerPlugin(*this, e.lifetime, e.randomLifetimePerc);
    }

    if (e.isWeatherParticle)
    {
        // For now we don't support this
        // 
    	// float weatherFadeoutDistance, weatherCubeSize, weatherCubeDistance;
        //
        Log::WriteInfo("Emitter uses unsupported weather particles\n");
    }
	
    if (e.inwardSpeed != 0.0f) {
        m_modifiers.push_back(new InwardSpeedModifierPlugin(*this, -e.inwardSpeed));
    }

    if (e.acceleration.length() != 0.0f || e.gravity != 0.0f) {
        m_modifiers.push_back(new AccelerationModifierPlugin(*this, e.acceleration + Vector3(0,0,-e.gravity), e.objectSpaceAcceleration));
    }

    if (e.inwardAcceleration != 0.0f) {
        m_modifiers.push_back(new AttractAccelerationModifierPlugin(*this, e.inwardAcceleration));
    }

    if (e.affectedByWind != 0.0f) {
        m_modifiers.push_back(new WindAccelerationModifierPlugin(*this, 0.01f));
    }

    if (e.groundBehavior == OldEmitter::GROUND_BOUNCE) {
        m_modifiers.push_back(new TerrainBounceModifierPlugin(*this, e.bounciness));
    } else if (e.groundBehavior == OldEmitter::GROUND_STICK) {
        m_modifiers.push_back(new TerrainBounceModifierPlugin(*this, 0.0f));
    }

    // Color tracks
    m_modifiers.push_back(new KeyedChannelColorModifierPlugin(*this, 
        e.tracks[OldEmitter::TRACK_RED_CHANNEL],
        e.tracks[OldEmitter::TRACK_GREEN_CHANNEL],
        e.tracks[OldEmitter::TRACK_BLUE_CHANNEL],
        e.tracks[OldEmitter::TRACK_ALPHA_CHANNEL]));

    if (Vector4(e.randomColors).length() != 0.0f) {
        m_modifiers.push_back(new ColorVarianceModifierPlugin(*this, e.randomColors, e.doColorAddGrayscale));
    }

    // Size track
    {
        const OldEmitter::Track& track = e.tracks[OldEmitter::TRACK_SCALE];
        float var = e.randomScalePerc / 2;
        if (track.size() == 2) {
            if (track.begin()->second == track.rbegin()->second) {
                // Constant
                m_modifiers.push_back(new ConstantSizeModifierPlugin(*this, track.begin()->second / 2 / (1 + var), var));
            } else {
                // Linear
                m_modifiers.push_back(new LinearSizeModifierPlugin(*this, track.begin()->second / 2 / (1 + var), track.rbegin()->second / 2 / (1 + var), var, track.m_interpolation == OldEmitter::Track::IT_SMOOTH));
            }
        }
        else {
            // Keyed
            Track<float> sizes(track);
            for (size_t i = 0; i < sizes.size(); i++)
            {
                sizes[i].second = sizes[i].second / 2 / (1 + var);
            }
            m_modifiers.push_back(new KeyedSizeModifierPlugin(*this, sizes, var));
        }
    }

    // UV track
    {
        const OldEmitter::Track& track = e.tracks[OldEmitter::TRACK_INDEX];
        int texsize = (int)ceil(sqrtf((float)e.textureSize));
        if (track.size() == 2 && track.begin()->second == track.rbegin()->second) {
            // Constant
            int index = (int)track.begin()->second;
            Vector4 texCoord(((index % texsize) + 0.0f) / texsize, ((index / texsize) + 0.0f) / texsize, 1.0f / texsize, 1.0f / texsize);
            m_modifiers.push_back(new ConstantUVModifierPlugin(*this, texCoord));
        }
        else {
            // Keyed
            Track<Vector4> texcoords;
            texcoords.m_interpolation = track.m_interpolation;
            texcoords.Reserve(track.size());
            for (size_t i = 0; i < track.size(); i++)
            {
                int index = (int)(track[i].second + 0.5f);
                texcoords.push_back(Track<Vector4>::Key(
                    track[i].first,
                    Vector4((float)(index % texsize) / texsize, // x
                            (float)(index / texsize) / texsize, // y
                            1.0f / texsize, 1.0f / texsize)     // w, h
                ));
            }
            m_modifiers.push_back(new KeyedUVModifierPlugin(*this, texcoords));
        }
    }   

    if (e.randomRotation) {
        // Fixed, random rotation
        m_modifiers.push_back(new ConstantRotationModifierPlugin(*this, e.randomRotationAverage, e.randomRotationVariance, e.randomRotationDirection));
    } else {
        // Rotation Speed track
        const OldEmitter::Track& track = e.tracks[OldEmitter::TRACK_ROTATION_SPEED];
        m_modifiers.push_back(new KeyedRotationRateModifierPlugin(*this, track, e.randomRotationDirection));
    }   
}

ParticleSystem::Emitter::Emitter(ChunkReader& reader, ParticleSystem& system, size_t index, DependencyList* dependencies)
    : m_system(system), m_creator(NULL), m_translater(NULL), m_killer(NULL), m_renderer(NULL)
{
    try
    {
        for (int e = 0; e < NUM_SPAWN_EVENTS; e++)
        {
            m_spawns[e] = NULL;
        }

        if (dependencies != NULL)
        {
            // Particle format V1
            OldEmitter e(reader, this, *dependencies);
            if (index < dependencies->size()) {
                ConvertOldEmitter(e, (*dependencies)[index].first, (*dependencies)[index].second);
            } else {
                ConvertOldEmitter(e, NULL, false);
            }
        }
        else
        {
            // Particle format V2
            Verify(reader.next() == 0);
            m_name = reader.readString();

            Verify(reader.next() == 1);
            m_creator = dynamic_cast<CreatorPlugin*>(ReadPlugin(reader, PLUGIN_CREATOR));

            Verify(reader.next() == 2);
            m_translater = dynamic_cast<TranslaterPlugin*>(ReadPlugin(reader, PLUGIN_TRANSLATER));

            Verify(reader.next() == 3); 
            m_killer = dynamic_cast<KillerPlugin*>(ReadPlugin(reader, PLUGIN_KILLER));

            Verify(reader.next() == 4);
            m_renderer = dynamic_cast<RendererPlugin*>(ReadPlugin(reader, PLUGIN_RENDERER));

            ChunkType type;
            while ((type = reader.next()) == 5)
            {
                m_modifiers.push_back(NULL);
                m_modifiers.back() = dynamic_cast<ModifierPlugin*>(ReadPlugin(reader, PLUGIN_MODIFIER));
            }
            Verify(type == -1);
        }
    }
    catch (...)
    {
        Cleanup();
        throw;
    }
}

void ParticleSystem::Emitter::Cleanup()
{
    for (size_t i = 0; i < m_modifiers.size(); i++)
    {
        delete m_modifiers[i];
    }
    delete m_renderer;
    delete m_killer;
    delete m_translater;
    delete m_creator;
}

ParticleSystem::Emitter::~Emitter()
{
    Cleanup();
}

//
// ParticleSystem class
//
ParticleSystem::Emitter* ParticleSystem::RegisterSpawn(Emitter* emitter)
{
    Emitter* next = m_spawn;
    m_spawn = emitter;
    return next;
}

void ParticleSystem::ReadLightFieldSource(ChunkReader& reader)
{
    m_lightFieldSource.m_type            = LFT_POINT;
    m_lightFieldSource.m_angularVelocity = 0.0f;
    m_lightFieldSource.m_position        = reader.readVector3();
    m_lightFieldSource.m_width           = reader.readFloat();
    m_lightFieldSource.m_length          = reader.readFloat();
    m_lightFieldSource.m_height          = reader.readFloat();
    m_lightFieldSource.m_diffuse         = Color(Vector4(reader.readVector3(), 1.0f));
    m_lightFieldSource.m_intensity       = reader.readFloat();
    m_lightFieldSource.m_affectedByGlobalIntensity = (reader.readByte() != 0);
    m_lightFieldSource.m_intensityNoiseScale       = reader.readFloat();
    m_lightFieldSource.m_intensityNoiseTimeScale   = reader.readFloat();
    m_lightFieldSource.m_fadeType                  = (reader.readByte() != 0) ? LFFT_TIME : LFFT_PARTICLES;
    m_lightFieldSource.m_autoDestructTime          = reader.readFloat();
    m_lightFieldSource.m_autoDestructFadeTime      = reader.readFloat();
    m_lightFieldSource.m_autoDestructTime         += m_lightFieldSource.m_autoDestructFadeTime;
    m_lightFieldSource.m_particles                 = reader.readInteger();
    m_hasLightFieldSource = true;
}

void ParticleSystem::ReadFormat_V1(ChunkReader& reader)
{
    // Read and ignore name
    Verify(reader.next() == 0x0000);
    reader.readString();

    // Ignore 0001 chunk
    Verify(reader.next() == 0x0001 && reader.size() == sizeof(uint32_t));

    // Read emitters
    DependencyList dependencies;

    Verify(reader.next() == 0x0800);
    ChunkType type;
    while ((type = reader.next()) == 0x0700)
    {
        m_emitters.push_back(new Emitter(reader, *this, m_emitters.size(), &dependencies));
    }
    Verify(type == -1);

    // Read leave particles chunk
    type = reader.next();
    if (type == 0x0002)
    {
	    Verify(reader.size() == 1);
	    m_leaveParticles = reader.readByte() != 0;
	    type = reader.next();
    }

    // End of 0900h chunk
    Verify(type == -1);

    /*
    // Post-process
    for (unsigned int i = 0; i < m_emitters.size(); i++)
    {
        Emitter& emitter = m_emitters[i];
	    if (emitter.spawnOnDeath    != -1) m_emitters[emitter.spawnOnDeath]   .parent = &emitter;
	    if (emitter.spawnDuringLife != -1) m_emitters[emitter.spawnDuringLife].parent = &emitter;
    }
    */
}

void ParticleSystem::ReadFormat_V2(ChunkReader& reader)
{
    Verify(reader.next() == 0);
    ChunkType type = reader.next();
    if (type == 1)
    {
        type = reader.next();
        if (type == 2)
        {
            type = reader.next();
        }
    }
    Verify(type == 0x1520);
    
    while ((type = reader.next()) == 0x1540)
    {
        m_emitters.push_back(new Emitter(reader, *this, m_emitters.size()));
    }
    Verify(type == -1);
    
    if ((type = reader.next()) == 3)
    {
        ReadLightFieldSource(reader);
        type = reader.next();
    }
    Verify(type == -1);
}

ParticleSystem::ParticleSystem(ptr<IFile> file, const std::string& name)
    : m_spawn(NULL), m_hasLightFieldSource(false), m_leaveParticles(true), m_name(name)
{
    try
    {
        ChunkReader reader(file);
        ChunkType type = reader.next();
        if (type == 0x900)
        {
            ReadFormat_V1(reader);
        }
        else
        {
            Verify(type == 0x1500);
            ReadFormat_V2(reader);
        }

        for (size_t i = 0; i < m_emitters.size(); i++)
        {
            m_emitters[i]->Initialize();
        }
    }
    catch (...)
    {
        Cleanup();
        throw;
    }
}

void ParticleSystem::Cleanup()
{
    for (size_t i = 0; i < m_emitters.size(); i++)
    {
        delete m_emitters[i];
    }
}

ParticleSystem::~ParticleSystem()
{
    Cleanup();
}

}