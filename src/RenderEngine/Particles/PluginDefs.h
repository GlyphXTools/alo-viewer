#ifndef PARTICLES_PLUGIN_DEFS_H
#define PARTICLES_PLUGIN_DEFS_H
/*
 * Common definitions for plugin specifications.
 */

namespace Alamo
{

#define BEGIN_PARAM_LIST(id) switch (id) {
#define PARAM_INT(id, var)    case id: var = ReadInteger(); break
#define PARAM_FLOAT(id, var)  case id: var = ReadFloat();   break
#define PARAM_FLOAT3(id, var) case id: var = ReadVector3();  break
#define PARAM_FLOAT4(id, var) case id: var = ReadVector4(); break
#define PARAM_COLOR(id, var)  case id: var = ReadColor(); break
#define PARAM_STRING(id, var) case id: var = ReadString();  break
#define PARAM_BOOL(id, var)   case id: var = ReadBool();    break
#define PARAM_CUSTOM(id, var) case id: ReadProperty(var);   break
#define END_PARAM_LIST }

}

#endif