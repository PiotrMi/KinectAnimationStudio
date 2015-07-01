#include "FBX_helpers.h"


#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(gSdkManager->GetIOSettings()))
#endif


// a UI file provide a function to print messages
extern void UI_Printf(const char* msg, ...);


// Creates an instance of the SDK manager.
void InitializeSdkManager()
{
	// Create the FBX SDK memory manager object.
	// The SDK Manager allocates and frees memory
	// for almost all the classes in the SDK.
	gSdkManager = FbxManager::Create();

	// create an IOSettings object
	FbxIOSettings * ios = FbxIOSettings::Create(gSdkManager, IOSROOT);
	gSdkManager->SetIOSettings(ios);

}

// Destroys an instance of the SDK manager
void DestroySdkObjects(
	FbxManager* pSdkManager,
	bool pExitStatus
	)
{
	// Delete the FBX SDK manager.
	// All the objects that
	// (1) have been allocated by the memory manager, AND that
	// (2) have not been explicitly destroyed
	// will be automatically destroyed.
	if (pSdkManager) pSdkManager->Destroy();
	if (pExitStatus) FBXSDK_printf("Program Success!\n");
}

// Creates an importer object, and uses it to
// import a file into a scene.
bool LoadScene(
	FbxManager* pSdkManager,  // Use this memory manager...
	FbxScene* pScene,            // to import into this scene
	const char* pFilename         // the data from this file.
	)
{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	int i, lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

	// Get the version number of the FBX files generated by the
	// version of FBX SDK that you are using.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pSdkManager, "");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize(pFilename, -1, pSdkManager->GetIOSettings());

	// Get the version number of the FBX file format.
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)  // Problem with the file to be imported
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		UI_Printf("Call to FbxImporter::Initialize() failed.");
		UI_Printf("Error returned: %s", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			UI_Printf("FBX version number for this FBX SDK is %d.%d.%d",
				lSDKMajor, lSDKMinor, lSDKRevision);
			UI_Printf("FBX version number for file %s is %d.%d.%d",
				pFilename, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	UI_Printf("FBX version number for this FBX SDK is %d.%d.%d",
		lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		UI_Printf("FBX version number for file %s is %d.%d.%d",
			pFilename, lFileMajor, lFileMinor, lFileRevision);

		// In FBX, a scene can have one or more "animation stack". An animation stack is a
		// container for animation data.
		// You can access a file's animation stack information without
		// the overhead of loading the entire file into the scene.

		UI_Printf("Animation Stack Information");

		lAnimStackCount = lImporter->GetAnimStackCount();

		UI_Printf("    Number of animation stacks: %d", lAnimStackCount);
		UI_Printf("    Active animation stack: \"%s\"",
			lImporter->GetActiveAnimStackName());

		for (i = 0; i < lAnimStackCount; i++)
		{
			FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

			UI_Printf("    Animation Stack %d", i);
			UI_Printf("         Name: \"%s\"", lTakeInfo->mName.Buffer());
			UI_Printf("         Description: \"%s\"",
				lTakeInfo->mDescription.Buffer());

			// Change the value of the import name if the animation stack should
			// be imported under a different name.
			UI_Printf("         Import Name: \"%s\"", lTakeInfo->mImportName.Buffer());

			// Set the value of the import state to false
			// if the animation stack should be not be imported.
			UI_Printf("         Import State: %s", lTakeInfo->mSelect ? "true" : "false");
		}

		// Import options determine what kind of data is to be imported.
		// The default is true, but here we set the options explictly.

		IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
		IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);

	if (lStatus == false &&     // The import file may have a password
		lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
	{
		UI_Printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END
			FbxString lString(lPassword);

		IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
		IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);


		lStatus = lImporter->Import(pScene);

		if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
		{
			UI_Printf("Incorrect password: file not imported.");
		}
	}

	// Destroy the importer
	lImporter->Destroy();

	return lStatus;
}

// Exports a scene to a file
bool SaveScene(
	FbxManager* pSdkManager,
	FbxScene* pScene,
	const char* pFilename,
	int pFileFormat,
	bool pEmbedMedia
	)
{
	int lMajor, lMinor, lRevision;
	bool lStatus = true;

	// Create an exporter.
	FbxExporter* lExporter = FbxExporter::Create(pSdkManager, "");

	if (pFileFormat < 0 ||
		pFileFormat >=
		pSdkManager->GetIOPluginRegistry()->GetWriterFormatCount())
	{
		// Write in fall back format if pEmbedMedia is true
		pFileFormat =
			pSdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();

		if (!pEmbedMedia)
		{
			//Try to export in ASCII if possible
			int lFormatIndex, lFormatCount =
				pSdkManager->GetIOPluginRegistry()->
				GetWriterFormatCount();

			for (lFormatIndex = 0; lFormatIndex<lFormatCount; lFormatIndex++)
			{
				if (pSdkManager->GetIOPluginRegistry()->
					WriterIsFBX(lFormatIndex))
				{
					FbxString lDesc = pSdkManager->GetIOPluginRegistry()->
						GetWriterFormatDescription(lFormatIndex);
					if (lDesc.Find("ascii") >= 0)
					{
						pFileFormat = lFormatIndex;
						break;
					}
				}
			}
		}
	}

	// Initialize the exporter by providing a filename.
	if (lExporter->Initialize(pFilename, pFileFormat, pSdkManager->GetIOSettings()) == false)
	{
		UI_Printf("Call to FbxExporter::Initialize() failed.");
		UI_Printf("Error returned: %s", lExporter->GetStatus().GetErrorString());
		return false;
	}

	FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
	//UI_Printf("FBX version number for this FBX SDK is %d.%d.%d",
	//	lMajor, lMinor, lRevision);

	if (pSdkManager->GetIOPluginRegistry()->WriterIsFBX(pFileFormat))
	{
		// Export options determine what kind of data is to be imported.
		// The default (except for the option eEXPORT_TEXTURE_AS_EMBEDDED)
		// is true, but here we set the options explicitly.
		IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
		IOS_REF.SetBoolProp(EXP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(EXP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
	}

	// Export the scene.
	lStatus = lExporter->Export(pScene);

	// Destroy the exporter.
	lExporter->Destroy();

	return lStatus;
}

// Get the filters for the <Open file> dialog
// (description + file extention)
const char *GetReaderOFNFilters()
{
	int nbReaders =
		gSdkManager->GetIOPluginRegistry()->GetReaderFormatCount();

	FbxString s;
	int i = 0;

	for (i = 0; i < nbReaders; i++)
	{
		s += gSdkManager->GetIOPluginRegistry()->
			GetReaderFormatDescription(i);
		s += "|*.";
		s += gSdkManager->GetIOPluginRegistry()->
			GetReaderFormatExtension(i);
		s += "|";
	}

	// replace | by \0
	int nbChar = int(strlen(s.Buffer())) + 1;
	char *filter = new char[nbChar];
	memset(filter, 0, nbChar);

	FBXSDK_strcpy(filter, nbChar, s.Buffer());

	for (i = 0; i < int(strlen(s.Buffer())); i++)
	{
		if (filter[i] == '|')
		{
			filter[i] = 0;
		}
	}

	// the caller must delete this allocated memory
	return filter;
}

// Get the filters for the <Save file> dialog
// (description + file extention)
const char *GetWriterSFNFilters()
{
	int nbWriters =
		gSdkManager->GetIOPluginRegistry()->GetWriterFormatCount();

	FbxString s;
	int i = 0;

	for (i = 0; i < nbWriters; i++)
	{
		s += gSdkManager->GetIOPluginRegistry()->
			GetWriterFormatDescription(i);
		s += "|*.";
		s += gSdkManager->GetIOPluginRegistry()->
			GetWriterFormatExtension(i);
		s += "|";
	}

	// replace | by \0
	int nbChar = int(strlen(s.Buffer())) + 1;
	char *filter = new char[nbChar];
	memset(filter, 0, nbChar);

	FBXSDK_strcpy(filter, nbChar, s.Buffer());

	for (i = 0; i < int(strlen(s.Buffer())); i++)
	{
		if (filter[i] == '|')
		{
			filter[i] = 0;
		}
	}

	// the caller must delete this allocated memory
	return filter;
}

// to get a file extention for a WriteFileFormat
const char *GetFileFormatExt(
	const int pWriteFileFormat
	)
{
	char *buf = new char[10];
	memset(buf, 0, 10);

	// add a starting point .
	buf[0] = '.';
	const char * ext = gSdkManager->GetIOPluginRegistry()->
		GetWriterFormatExtension(pWriteFileFormat);
	FBXSDK_strcat(buf, 10, ext);

	// the caller must delete this allocated memory
	return buf;
}


// Filtering methods

/// <summary>
/// Applies unroll filter to the skeleton tree, recursively
/// </summary>
/// <param name="filter">FBX  Unroll filter instance</param>
/// <param name="fNode">Current FBX  node</param>
void applyUnrollFilterHierarchically(FbxAnimCurveFilterUnroll &filter, FbxNode *fNode) {

	// Make sure fNode is valid
	if (!fNode)
		return;


	// Get Curve node
	FbxAnimCurveNode *fCurveNode = fNode->LclRotation.GetCurveNode();

	// If there is a valid curve node
	if (fCurveNode && (fCurveNode->GetChannelsCount() == 3)) {
		filter.Apply(*fCurveNode);
	}

	// Do it for its children
	int childCount = fNode->GetChildCount();
	for (int i = 0; i < childCount; i++) {
		FbxNode *childNode = fNode->GetChild(i);
		applyUnrollFilterHierarchically(filter, childNode);
	}


}


/// <summary>
/// Get number of keys from rotation curve of a given joints
/// </summary>
/// <param name="tgtNode">Node to have info extracted</param>
/// <param name="lScene">FBX Scene</param>
int getKeyCount(FbxNode *tgtNode, FbxScene *lScene) {
	FbxAnimStack *animStack = lScene->GetCurrentAnimationStack();
	FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();
	FbxAnimCurve *rotCurve = tgtNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);

	int keyTotal = rotCurve->KeyGetCount();

	return keyTotal;
}

/// <summary>
/// Checks if joint is animatable
/// </summary>
/// <param name="tgtNode">Node to be checked</param>
bool isAnimatable(FbxNode *tgtNode) {
	const char *name = tgtNode->GetName();
	FbxAnimCurveNode *rot = tgtNode->LclRotation.GetCurveNode(), *trans = tgtNode->LclTranslation.GetCurveNode();
	return ((rot != NULL) || (trans != NULL));
}


/// <summary>
/// Sets custom ID for a FBX NODE
/// </summary>
void setCustomIdProperty(FbxNode *fNode, int newId) {
	FbxProperty lProperty = fNode->FindProperty(FBX_CUSTOM_ID_PROPERTY_LABEL);

	if (!lProperty.IsValid()) {
		lProperty = FbxProperty::Create(fNode, FbxIntDT, FBX_CUSTOM_ID_PROPERTY_LABEL, FBX_CUSTOM_ID_PROPERTY_LABEL);
	}
	lProperty.Set(newId);

}

/// <summary>
/// Gets custom ID from a FBX NODE
/// </summary>
int getCustomIdProperty(FbxNode *fNode) {
	int id = -1;
	
	FbxProperty lProperty = fNode->FindProperty(FBX_CUSTOM_ID_PROPERTY_LABEL);

	if (lProperty.IsValid()) {
		id = lProperty.Get<FbxInt>();
	}
	return id;
}