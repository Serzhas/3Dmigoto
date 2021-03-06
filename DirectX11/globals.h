#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <ctime>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "util.h"

enum HuntingMode {
	HUNTING_MODE_DISABLED = 0,
	HUNTING_MODE_ENABLED = 1,
	HUNTING_MODE_SOFT_DISABLED = 2,
};

const int MARKING_MODE_SKIP = 0;
const int MARKING_MODE_MONO = 1;
const int MARKING_MODE_ORIGINAL = 2;
const int MARKING_MODE_ZERO = 3;
const int MARKING_MODE_PINK = 4;

// Key is index/vertex buffer, value is hash key.
typedef std::unordered_map<ID3D11Buffer *, UINT64> DataBufferMap;

// Source compiled shaders.
typedef std::unordered_map<UINT64, std::string> CompiledShaderMap;

// Strategy: This OriginalShaderInfo record and associated map is to allow us to keep track of every
//	pixelshader and vertexshader that are compiled from hlsl text from the ShaderFixes
//	folder.  This keeps track of the original shader information using the ID3D11VertexShader*
//	or ID3D11PixelShader* as a master key to the key map.
//	We are using the base class of ID3D11DeviceChild* since both descend from that, and that allows
//	us to use the same structure for Pixel and Vertex shaders both.

// Info saved about originally overridden shaders passed in by the game in CreateVertexShader or
// CreatePixelShader that have been loaded as HLSL
//	shaderType is "vs" or "ps" or maybe later "gs" (type wstring for file name use)
//	shaderModel is only filled in when a shader is replaced.  (type string for old D3 API use)
//	linkage is passed as a parameter, seems to be rarely if ever used.
//	byteCode is the original shader byte code passed in by game, or recompiled by override.
//	timeStamp allows reloading/recompiling only modified shaders
//	replacement is either ID3D11VertexShader or ID3D11PixelShader
//  found is used to revert shaders that are deleted from ShaderFixes
struct OriginalShaderInfo
{
	UINT64 hash;
	std::wstring shaderType;
	std::string shaderModel;
	ID3D11ClassLinkage* linkage;
	ID3DBlob* byteCode;
	FILETIME timeStamp;
	ID3D11DeviceChild* replacement;
	bool found;
};

// Key is the overridden shader that was given back to the game at CreateVertexShader (vs or ps)
typedef std::unordered_map<ID3D11DeviceChild *, OriginalShaderInfo> ShaderReloadMap;

// Key is vertexshader, value is hash key.
typedef std::unordered_map<ID3D11VertexShader *, UINT64> VertexShaderMap;
typedef std::unordered_map<UINT64, ID3D11VertexShader *> PreloadVertexShaderMap;
typedef std::unordered_map<ID3D11VertexShader *, ID3D11VertexShader *> VertexShaderReplacementMap;

// Key is pixelshader, value is hash key.
typedef std::unordered_map<ID3D11PixelShader *, UINT64> PixelShaderMap;
typedef std::unordered_map<UINT64, ID3D11PixelShader *> PreloadPixelShaderMap;
typedef std::unordered_map<ID3D11PixelShader *, ID3D11PixelShader *> PixelShaderReplacementMap;

typedef std::unordered_map<ID3D11ComputeShader *, UINT64> ComputeShaderMap;
typedef std::unordered_map<ID3D11ComputeShader *, ID3D11ComputeShader *> ComputeShaderReplacementMap;

typedef std::unordered_map<ID3D11HullShader *, UINT64> HullShaderMap;
typedef std::unordered_map<ID3D11HullShader *, ID3D11HullShader *> HullShaderReplacementMap;

typedef std::unordered_map<ID3D11DomainShader *, UINT64> DomainShaderMap;
typedef std::unordered_map<ID3D11DomainShader *, ID3D11DomainShader *> DomainShaderReplacementMap;

typedef std::unordered_map<ID3D11GeometryShader *, UINT64> GeometryShaderMap;
typedef std::unordered_map<ID3D11GeometryShader *, ID3D11GeometryShader *> GeometryShaderReplacementMap;

enum class FrameAnalysisOptions {
	INVALID      = 0,
	DUMP_RT      = 0x00000001,
	DUMP_RT_JPS  = 0x00000002,
	DUMP_RT_DDS  = 0x00000004,
	DUMP_RT_MASK = 0x00000007,
	CLEAR_RT     = 0x00000008,
	PERSIST      = 0x00000010, // Used by shader/texture triggers
	STEREO       = 0x00000020,
	MONO         = 0x00000040,
	STEREO_MASK  = 0x00000060,
};
SENSIBLE_ENUM(FrameAnalysisOptions);
static EnumName_t<wchar_t *, FrameAnalysisOptions> FrameAnalysisOptionNames[] = {
	{L"dump_rt", FrameAnalysisOptions::DUMP_RT},
	{L"dump_rt_jps", FrameAnalysisOptions::DUMP_RT_JPS},
	{L"dump_rt_dds", FrameAnalysisOptions::DUMP_RT_DDS},
	{L"clear_rt", FrameAnalysisOptions::CLEAR_RT},
	{L"persist", FrameAnalysisOptions::PERSIST},
	{L"stereo", FrameAnalysisOptions::STEREO},
	{L"mono", FrameAnalysisOptions::MONO},
	// TODO: More options on the way: Dump texture inputs
	{NULL, FrameAnalysisOptions::INVALID} // End of list marker
};

enum class DepthBufferFilter {
	INVALID = -1,
	NONE,
	DEPTH_ACTIVE,
	DEPTH_INACTIVE,
};
static EnumName_t<wchar_t *, DepthBufferFilter> DepthBufferFilterNames[] = {
	{L"none", DepthBufferFilter::NONE},
	{L"depth_active", DepthBufferFilter::DEPTH_ACTIVE},
	{L"depth_inactive", DepthBufferFilter::DEPTH_INACTIVE},
	{NULL, DepthBufferFilter::INVALID} // End of list marker
};

struct ShaderOverride {
	float separation;
	float convergence;
	bool skip;
	std::vector<int> iterations; // Only for separation changes, not shaders.
	std::vector<UINT64> indexBufferFilter;
	DepthBufferFilter depth_filter;
	UINT64 partner_hash;
	FrameAnalysisOptions analyse_options;
	bool fake_o0;

	int depth_input;
	ID3D11Texture2D *depth_resource = NULL;
	ID3D11ShaderResourceView *depth_view = NULL;
	UINT depth_width, depth_height;

	ShaderOverride() :
		separation(FLT_MAX),
		convergence(FLT_MAX),
		skip(false),
		depth_filter(DepthBufferFilter::NONE),
		partner_hash(0),
		analyse_options(FrameAnalysisOptions::INVALID),
		fake_o0(false),
		depth_input(0),
		depth_resource(NULL),
		depth_view(NULL),
		depth_width(0),
		depth_height(0)
	{}

	~ShaderOverride()
	{
		if (depth_resource)
			depth_resource->Release();

		if (depth_view)
			depth_view->Release();
	}
};
typedef std::unordered_map<UINT64, struct ShaderOverride> ShaderOverrideMap;

struct TextureOverride {
	int stereoMode;
	int format;
	std::vector<int> iterations;
	FrameAnalysisOptions analyse_options;

	TextureOverride() :
		stereoMode(-1),
		format(-1),
		analyse_options(FrameAnalysisOptions::INVALID)
	{}
};
typedef std::unordered_map<UINT64, struct TextureOverride> TextureOverrideMap;

struct ShaderInfoData
{
	// All are std::map or std::set so that ShaderUsage.txt is sorted - lookup time is O(log N)
	std::map<int, std::set<void *>> ResourceRegisters;
	std::set<UINT64> PartnerShader;
	std::vector<std::set<void *>> RenderTargets;
	std::set<void *> DepthTargets;
};
struct SwapChainInfo
{
	int width, height;
};

struct ResourceInfo
{
	D3D11_RESOURCE_DIMENSION type;
	union {
		D3D11_TEXTURE2D_DESC tex2d_desc;
		D3D11_TEXTURE3D_DESC tex3d_desc;
	};

	ResourceInfo() :
		type(D3D11_RESOURCE_DIMENSION_UNKNOWN)
	{}

	struct ResourceInfo & operator= (D3D11_TEXTURE2D_DESC desc)
	{
		type = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
		tex2d_desc = desc;
		return *this;
	}

	struct ResourceInfo & operator= (D3D11_TEXTURE3D_DESC desc)
	{
		type = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
		tex3d_desc = desc;
		return *this;
	}
};

struct Globals
{
	bool gInitialized;
	bool gReloadConfigPending;
	bool gLogInput;

	wchar_t SHADER_PATH[MAX_PATH];
	wchar_t SHADER_CACHE_PATH[MAX_PATH];
	wchar_t CHAIN_DLL_PATH[MAX_PATH];

	int SCREEN_WIDTH;
	int SCREEN_HEIGHT;
	int SCREEN_REFRESH;
	bool SCREEN_FULLSCREEN;
	int FILTER_REFRESH[11];
	bool SCREEN_ALLOW_COMMANDS;

	int marking_mode;
	bool gForceStereo;
	bool gCreateStereoProfile;
	int gSurfaceCreateMode;
	int gSurfaceSquareCreateMode;
	bool gForceNoNvAPI;

	UINT hunting;
	bool fix_enabled;
	bool config_reloadable;
	bool show_original_enabled;
	bool show_pink_enabled;
	time_t huntTime;

	bool deferred_enabled;

	unsigned analyse_frame;
	bool analyse_next_frame;
	wchar_t ANALYSIS_PATH[MAX_PATH];
	FrameAnalysisOptions def_analyse_options, cur_analyse_options;
	std::unordered_set<void*> frame_analysis_seen_rts;

	int EXPORT_HLSL;		// 0=off, 1=HLSL only, 2=HLSL+OriginalASM, 3= HLSL+OriginalASM+recompiledASM
	bool EXPORT_SHADERS, EXPORT_FIXED, EXPORT_BINARY, CACHE_SHADERS, PRELOAD_SHADERS, SCISSOR_DISABLE;
	char ZRepair_DepthTextureReg1, ZRepair_DepthTextureReg2;
	std::string ZRepair_DepthTexture1, ZRepair_DepthTexture2;
	std::vector<std::string> ZRepair_Dependencies1, ZRepair_Dependencies2;
	std::string ZRepair_ZPosCalc1, ZRepair_ZPosCalc2;
	std::string ZRepair_PositionTexture, ZRepair_WorldPosCalc;
	std::vector<std::string> InvTransforms;
	std::string BackProject_Vector1, BackProject_Vector2;
	std::string ObjectPos_ID1, ObjectPos_ID2, ObjectPos_MUL1, ObjectPos_MUL2;
	std::string MatrixPos_ID1, MatrixPos_MUL1;
	UINT64 ZBufferHashToInject;
	bool FIX_SV_Position;
	bool FIX_Light_Position;
	bool FIX_Recompile_VS;
	bool DumpUsage;
	bool ENABLE_TUNE;
	float gTuneValue[4], gTuneStep;

	DirectX::XMFLOAT4 iniParams;

	SwapChainInfo mSwapChainInfo;

	CRITICAL_SECTION mCriticalSection;
	bool ENABLE_CRITICAL_SECTION;

	DataBufferMap mDataBuffers;
	std::set<UINT64> mVisitedIndexBuffers;					// std::set is sorted for consistent order while hunting
	UINT64 mSelectedIndexBuffer;
	int mSelectedIndexBufferPos;
	std::set<UINT64> mSelectedIndexBuffer_VertexShader;		// std::set so that shaders used with an index buffer will be sorted in log when marked
	std::set<UINT64> mSelectedIndexBuffer_PixelShader;		// std::set so that shaders used with an index buffer will be sorted in log when marked

	CompiledShaderMap mCompiledShaderMap;

	VertexShaderMap mVertexShaders;							// All shaders ever registered with CreateVertexShader
	PreloadVertexShaderMap mPreloadedVertexShaders;			// All shaders that were preloaded as .bin
	VertexShaderReplacementMap mOriginalVertexShaders;		// When MarkingMode=Original, switch to original
	VertexShaderReplacementMap mZeroVertexShaders;			// When MarkingMode=zero.
	std::set<UINT64> mVisitedVertexShaders;					// Only shaders seen since last hunting timeout; std::set for consistent order while hunting
	UINT64 mSelectedVertexShader;				 			// Hash.  -1 now for unselected state. The shader selected using Input object.
	int mSelectedVertexShaderPos;							// -1 for unselected state.
	std::set<UINT64> mSelectedVertexShader_IndexBuffer;		// std::set so that index buffers used with a shader will be sorted in log when marked

	PixelShaderMap mPixelShaders;							// All shaders ever registered with CreatePixelShader
	PreloadPixelShaderMap mPreloadedPixelShaders;
	PixelShaderReplacementMap mOriginalPixelShaders;
	PixelShaderReplacementMap mZeroPixelShaders;
	std::set<UINT64> mVisitedPixelShaders;					// std::set is sorted for consistent order while hunting
	UINT64 mSelectedPixelShader;							// Hash.  -1 now for unselected state.
	int mSelectedPixelShaderPos;							// -1 for unselected state.
	std::set<UINT64> mSelectedPixelShader_IndexBuffer;		// std::set so that index buffers used with a shader will be sorted in log when marked
	ID3D11PixelShader* mPinkingShader;						// Special pixels shader to mark a selection with hot pink.

	ShaderReloadMap mReloadedShaders;						// Shaders that were reloaded live from ShaderFixes

	ComputeShaderMap mComputeShaders;
	ComputeShaderReplacementMap mOriginalComputeShaders;
	std::set<UINT64> mVisitedComputeShaders;
	UINT64 mSelectedComputeShader;
	int mSelectedComputeShaderPos;

	GeometryShaderMap mGeometryShaders;
	GeometryShaderReplacementMap mOriginalGeometryShaders;
	std::set<UINT64> mVisitedGeometryShaders;
	UINT64 mSelectedGeometryShader;
	int mSelectedGeometryShaderPos;

	DomainShaderMap mDomainShaders;
	DomainShaderReplacementMap mOriginalDomainShaders;
	std::set<UINT64> mVisitedDomainShaders;
	UINT64 mSelectedDomainShader;
	int mSelectedDomainShaderPos;

	HullShaderMap mHullShaders;
	HullShaderReplacementMap mOriginalHullShaders;
	std::set<UINT64> mVisitedHullShaders;
	UINT64 mSelectedHullShader;
	int mSelectedHullShaderPos;

	ShaderOverrideMap mShaderOverrideMap;
	TextureOverrideMap mTextureOverrideMap;

	// Statistics
	std::unordered_map<void *, UINT64> mRenderTargets;
	std::map<UINT64, struct ResourceInfo> mRenderTargetInfo; // std::map so that ShaderUsage.txt is sorted - lookup time is O(log N)
	std::map<UINT64, struct ResourceInfo> mDepthTargetInfo;	// std::map so that ShaderUsage.txt is sorted - lookup time is O(log N)
	std::set<void *> mVisitedRenderTargets;					// std::set is sorted for consistent order while hunting
	void *mSelectedRenderTarget;
	int mSelectedRenderTargetPos;
	// Snapshot of all targets for selection.
	void *mSelectedRenderTargetSnapshot;
	std::set<void *> mSelectedRenderTargetSnapshotList;		// std::set so that render targets will be sorted in log when marked
	// Relations
	std::unordered_map<ID3D11Texture2D *, UINT64> mTexture2D_ID;
	std::unordered_map<ID3D11Texture3D *, UINT64> mTexture3D_ID;
	std::map<UINT64, ShaderInfoData> mVertexShaderInfo;		// std::map so that ShaderUsage.txt is sorted - lookup time is O(log N)
	std::map<UINT64, ShaderInfoData> mPixelShaderInfo;		// std::map so that ShaderUsage.txt is sorted - lookup time is O(log N)

	bool mBlockingMode;

	Globals() :
		mBlockingMode(false),
		mSelectedRenderTargetSnapshot(0),
		mSelectedRenderTargetPos(-1),
		mSelectedRenderTarget((void *)-1),
		mSelectedPixelShader(-1),
		mSelectedPixelShaderPos(-1),
		mSelectedVertexShader(-1),
		mSelectedVertexShaderPos(-1),
		mSelectedIndexBuffer(1),
		mSelectedIndexBufferPos(-1),
		mSelectedComputeShader(-1),
		mSelectedComputeShaderPos(-1),
		mSelectedGeometryShader(-1),
		mSelectedGeometryShaderPos(-1),
		mSelectedDomainShader(-1),
		mSelectedDomainShaderPos(-1),
		mSelectedHullShader(-1),
		mSelectedHullShaderPos(-1),
		mPinkingShader(0),

		hunting(HUNTING_MODE_DISABLED),
		fix_enabled(true),
		config_reloadable(false),
		show_original_enabled(false),
		show_pink_enabled(false),
		huntTime(0),

		deferred_enabled(true),

		analyse_frame(0),
		analyse_next_frame(false),
		def_analyse_options(FrameAnalysisOptions::INVALID),
		cur_analyse_options(FrameAnalysisOptions::INVALID),

		EXPORT_SHADERS(false),
		EXPORT_HLSL(0),
		EXPORT_FIXED(false),
		EXPORT_BINARY(false),
		CACHE_SHADERS(false),
		PRELOAD_SHADERS(false),
		FIX_SV_Position(false),
		FIX_Light_Position(false),
		FIX_Recompile_VS(false),
		DumpUsage(false),
		ENABLE_TUNE(false),
		gTuneStep(0.001f),

		iniParams{ FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX },

		ENABLE_CRITICAL_SECTION(false),
		SCREEN_WIDTH(-1),
		SCREEN_HEIGHT(-1),
		SCREEN_REFRESH(-1),
		SCREEN_FULLSCREEN(false),
		SCREEN_ALLOW_COMMANDS(false),

		marking_mode(-1),
		gForceStereo(false),
		gCreateStereoProfile(false),
		gSurfaceCreateMode(-1),
		gSurfaceSquareCreateMode(-1),
		gForceNoNvAPI(false),
		ZBufferHashToInject(0),
		SCISSOR_DISABLE(0),

		gInitialized(false),
		gReloadConfigPending(false),
		gLogInput(false)

	{
		SHADER_PATH[0] = 0;
		SHADER_CACHE_PATH[0] = 0;
		CHAIN_DLL_PATH[0] = 0;

		ANALYSIS_PATH[0] = 0;

		mSwapChainInfo.width = -1;
		mSwapChainInfo.height = -1;

		for (int i = 0; i < 4; i++)
			gTuneValue[i] = 1.0f;

		for (int i = 0; i < 11; i++)
			FILTER_REFRESH[i] = 0;
	}
};

extern Globals *G;
