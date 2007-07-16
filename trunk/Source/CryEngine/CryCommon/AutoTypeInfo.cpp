#include <platform.h>

#ifdef ENABLE_TYPE_INFO

#include <CryTypeInfo.h>

// Includes needed for Auto-defined types.
#include "Cry_Math.h"
#include "primitives.h"
using namespace primitives;
#include "physinterface.h"
#include "CryHeaders.h"
#include "IXml.h"
#include "I3DEngine.h"
#include "ISystem.h"
#include "CryName.h"
#include "IRenderAuxGeom.h"
#include "IEntityRenderState.h"
#include "IIndexedMesh.h"
#include "CGFContent.h"
#include "Force.h"
#include "I3DSampler.h"
#include "XMLBinaryHeaders.h"

#include "../CrySystem/ZipFileFormat.h"
#include "../Cry3DEngine/dds.h"
#include "../Cry3DEngine/VoxMan.h"
#include "../Cry3DEngine/LMSerializationManager.h"
#include "../Cry3DEngine/SkyLightNishita.h"
#include "../Cry3DEngine/3DSamplerOctree.h"
#include "../CryPhysics/bvtree.h"
#include "../CryPhysics/aabbtree.h"
#include "../CryPhysics/obbtree.h"
#include "../CryPhysics/geoman.h"
#include "../CryAction/IAnimationGraph.h"
#include "../CryAISystem/AutoTypeStructs.h"
#include "../CrySoundSystem/MusicSystem/Decoder/ADPCMDecoder.h"
#include "../CrySoundSystem/MusicSystem/Decoder/PatternDecoder.h"
#include "../RenderDll/Common/ResFile.h"
//#ifdef PS3
//#include "../RenderDll/Common/Shaders/CShaderBin.h"
//#endif
#include "../CryAction/PlayerProfiles/RichSaveGameTypes.h"

// The auto-generated info file.
#include "../../Solutions/AutoTypeInfo.h"


// Manually implement type infos as needed.

TYPE_INFO_PLAIN(primitives::getHeightCallback)
TYPE_INFO_PLAIN(primitives::getSurfTypeCallback)

#undef STRUCT_INFO_T_INSTANTIATE
#define STRUCT_INFO_T_INSTANTIATE(T, TArgs)	\
	template const CTypeInfo& T TArgs::TypeInfo();

STRUCT_INFO_T_INSTANTIATE(Color_tpl, <float>)
STRUCT_INFO_T_INSTANTIATE(Ang3_tpl, <float>)
STRUCT_INFO_T_INSTANTIATE(Plane_tpl, <float>)

#endif
