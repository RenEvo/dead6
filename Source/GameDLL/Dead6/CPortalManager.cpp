////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// cPortalManager.cpp
//
// Purpose: Dynamically updates a texture with 
//	"the world" viewed from a custom camera
//
// File History:
//	- 8/20/07 : File created - KAK
////////////////////////////////////////////////////

#include "Stdafx.h"
#include "CPortalManager.h"

////////////////////////////////////////////////////
unsigned char *SPortalDef::pBuffer = NULL;
SPortalDef::SPortalDef(void) :
	nEntityID(0),
	nCameraEntityID(0),
	nPortalTextureID(0),
	nFrameSkip(0),
	nLastFrameID(0),
	nWidth(0),
	nHeight(0)
{
	
}

////////////////////////////////////////////////////
SPortalDef::~SPortalDef(void)
{

}

////////////////////////////////////////////////////
void SPortalDef::UpdateTextureInfo(void)
{
	// Reset properties
	nPortalTextureID = nWidth = nHeight = 0;

	IEntity *pPortalEntity = gEnv->pEntitySystem->GetEntity(nEntityID);
	if (NULL == pPortalEntity) return;

	// Get the material, custom first
	IMaterial *pMaterial = pPortalEntity->GetMaterial();
	if (NULL == pMaterial)
	{
		// Get the material belonging to the static object of the entity
		SEntitySlotInfo info;
		pPortalEntity->GetSlotInfo(0, info);
		pMaterial = (NULL == info.pStatObj ? NULL : info.pStatObj->GetMaterial());
		if (NULL == pMaterial)
		{
			// No material...
			CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Portal for entity %d (\'%s\') has no defined material!",
				nEntityID, pPortalEntity->GetName());
			return;
		}
	}

	SShaderItem shaderItem;
	SInputShaderResources shaderResources;
	string szTextName;

	// Check the material's shader if it has one
	shaderItem = pMaterial->GetShaderItem();
	if (NULL != shaderItem.m_pShaderResources)
	{
		// Check the textures of the input resources
		shaderItem.m_pShaderResources->ConvertToInputResource(&shaderResources);
		for (int texNum = 0; texNum < EFTT_MAX; texNum++)
		{
			szTextName = shaderResources.m_Textures[texNum].m_Name;
			if (true == szTextName.empty()) continue;

			// Strip path and extension from texture name
			string::size_type nLastBackSlashPos = szTextName.rfind('\\');
			string::size_type nLastFrwdSlashPos = szTextName.rfind('/');
			string::size_type nLastSlashPos = string::npos;
			if (string::npos != nLastBackSlashPos)
				nLastSlashPos = ((string::npos!=nLastFrwdSlashPos) ? max(nLastBackSlashPos, nLastFrwdSlashPos) : nLastBackSlashPos);
			else if (string::npos != nLastFrwdSlashPos)
				nLastSlashPos = nLastFrwdSlashPos;
			if (string::npos != nLastSlashPos)
				szTextName = szTextName.substr(nLastSlashPos+1);
			string::size_type nLastPeriodPos = szTextName.rfind('.');
			if (string::npos != nLastPeriodPos)
				szTextName = szTextName.substr(0, nLastPeriodPos);

			// If it is the requested texture, get the info from it
			if (0 == stricmp(szTextName.c_str(), szPortalTexture.c_str()))
			{
				ITexture *pTexture = shaderResources.m_Textures[texNum].m_Sampler.m_pITex;
				if (NULL == pTexture) continue;

				// Get its info
				nPortalTextureID = pTexture->GetTextureID();
				nWidth = pTexture->GetWidth();
				nHeight = pTexture->GetHeight();
				if (nWidth <= 0 || nHeight <= 0)
				{
					// Not valid
					nPortalTextureID = 0;
				}
				return;
			}
		}
	}

	// Check the sub-materials
	int nSubCount = pMaterial->GetSubMtlCount();
	IMaterial *pSubMat = NULL;
	for (int nSubMat = 0; nSubMat < nSubCount; nSubMat++)
	{
		// Get sub-material
		pSubMat = pMaterial->GetSubMtl(nSubMat);
		if (NULL == pSubMat) continue;

		// Get the shader item, and check its shader resources
		shaderItem = pSubMat->GetShaderItem();
		if (NULL == shaderItem.m_pShaderResources) continue;

		// Check the textures of the input resources
		shaderItem.m_pShaderResources->ConvertToInputResource(&shaderResources);
		for (int texNum = 0; texNum < EFTT_MAX; texNum++)
		{
			szTextName = shaderResources.m_Textures[texNum].m_Name;
			if (true == szTextName.empty()) continue;

			// Strip path and extension from texture name
			string::size_type nLastBackSlashPos = szTextName.rfind('\\');
			string::size_type nLastFrwdSlashPos = szTextName.rfind('/');
			string::size_type nLastSlashPos = string::npos;
			if (string::npos != nLastBackSlashPos)
				nLastSlashPos = ((string::npos!=nLastFrwdSlashPos) ? max(nLastBackSlashPos, nLastFrwdSlashPos) : nLastBackSlashPos);
			else if (string::npos != nLastFrwdSlashPos)
				nLastSlashPos = nLastFrwdSlashPos;
			if (string::npos != nLastSlashPos)
				szTextName = szTextName.substr(nLastSlashPos+1);
			string::size_type nLastPeriodPos = szTextName.rfind('.');
			if (string::npos != nLastPeriodPos)
				szTextName = szTextName.substr(0, nLastPeriodPos);

			// If it is the requested texture, get the info from it
			if (0 == stricmp(szTextName.c_str(), szPortalTexture.c_str()))
			{
				ITexture *pTexture = shaderResources.m_Textures[texNum].m_Sampler.m_pITex;
				if (NULL == pTexture) continue;

				// Get its info
				nPortalTextureID = pTexture->GetTextureID();
				nWidth = pTexture->GetWidth();
				nHeight = pTexture->GetHeight();
				if (nWidth <= 0 || nHeight <= 0)
				{
					// Not valid
					nPortalTextureID = 0;
				}
				return;
			}
		}
	}

	// Failed to find it
	CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Failed to find texture \'%s\' for Portal for entity %d (\'%s\')",
			szPortalTexture.c_str(), nEntityID, pPortalEntity->GetName());
}

////////////////////////////////////////////////////
void SPortalDef::RenderPortal(IRenderer *pRenderer)
{
	// Fill buffer with what's in the frame buffer
	memset(pBuffer, 0, PORTAL_MAX_WIDTH * PORTAL_MAX_HEIGHT * 4);
	pRenderer->ReadFrameBuffer(pBuffer, nWidth, PORTAL_MAX_WIDTH, PORTAL_MAX_HEIGHT, 
		eRB_BackBuffer, true, nWidth, nHeight);

	// Move origin to bottom-left corner
	const int nWholeSize = nWidth * nHeight * 4;
	const int n4Width = nWidth * 4;
	const int nHalfSize = nWholeSize>>1;
	unsigned char pTempBuffer[PORTAL_MAX_WIDTH * PORTAL_MAX_HEIGHT * 2]; // Filled with bottom-half of buffer
	memcpy(pTempBuffer, pBuffer+nHalfSize, nHalfSize);
	// Top half to bottom half
	for (int offset = 0; offset < nHalfSize; offset += n4Width)
	{
		memcpy(pBuffer+nWholeSize-n4Width-offset, pBuffer+offset, n4Width);
	}
	// Bottom half to top half
	for (int offset = 0; offset < nHalfSize; offset += n4Width)
	{
		memcpy(pBuffer+nHalfSize-n4Width-offset, pTempBuffer+offset, n4Width);
	}

	// Finally, update texture with it
	pRenderer->UpdateTextureInVideoMemory(nPortalTextureID, pBuffer, 0, 0, nWidth, nHeight, eTF_X8R8G8B8);

	// Debug print it out
	//static bool bOnce = true;
	//if (bOnce)
	//{
	//	bOnce = false;

	//	// TGA file
	//	FILE *pFile = fopen("Z:\\Test.tga","wb");		if(!pFile)	return;
	//	uint8 header[18];
	//	memset(&header, 0, sizeof(header));
	//	header[2] = 2;								
	//	*(reinterpret_cast<unsigned short*>(&header[12])) = nWidth;
	//	*(reinterpret_cast<unsigned short*>(&header[14])) = nHeight;
	//	header[16] = 0x20;						
	//	header[17] = 0x28;						
	//	fwrite(&header, 1, sizeof(header), pFile);
	//	fwrite(pBuffer, 1, nWholeSize, pFile);	
	//	fclose(pFile);
	//}
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////

////////////////////////////////////////////////////
CPortalManager::CPortalManager(void)
{
	Reset();
}

////////////////////////////////////////////////////
CPortalManager::~CPortalManager(void)
{
	Shutdown();
}

////////////////////////////////////////////////////
void CPortalManager::Initialize(void)
{
	Reset();

	// Create the temp buffer
	SPortalDef::pBuffer = new unsigned char[PORTAL_MAX_WIDTH * PORTAL_MAX_HEIGHT * 4];
}

////////////////////////////////////////////////////
void CPortalManager::Shutdown(void)
{
	// Remove all entries
	for (PortalMap::iterator itPortal = m_Portals.begin(); itPortal != m_Portals.end(); itPortal++)
	{
		SAFE_DELETE(itPortal->second);
	}
	m_Portals.clear();

	// Delete temp buffer
	SAFE_DELETE(SPortalDef::pBuffer);
}

////////////////////////////////////////////////////
void CPortalManager::Reset(void)
{
	IRenderer *pRenderer = g_D6Core->pSystem->GetIRenderer();
	nLastFrameID = (NULL == pRenderer? 0 : pRenderer->GetFrameID()-1);
}

////////////////////////////////////////////////////
void CPortalManager::Update(bool bHaveFocus, unsigned int nUpdateFlags)
{
	if (false == bHaveFocus) return;

	// If in editor, only work if game is running
	if (true == g_D6Core->pSystem->IsEditor() && false == g_D6Core->pD6Game->IsEditorGameStarted())
		return;
	IRenderer *pRenderer = g_D6Core->pSystem->GetIRenderer();
	I3DEngine *p3DEngine = g_D6Core->pSystem->GetI3DEngine();

	// Get frame ID, only parse if we've advanced
	int nCurrFrameID = pRenderer->GetFrameID();
	if (nCurrFrameID == nLastFrameID) return;
	nLastFrameID = nCurrFrameID;

	// Get player's camera
	CCamera PlayerCam = pRenderer->GetCamera();

	// Peek to see who needs updating
	PortalMap UpdateMap;
	for (PortalMap::iterator itPortal = m_Portals.begin(); itPortal != m_Portals.end(); itPortal++)
	{
		// Must have a valid texture ID ready
		if (0 == itPortal->second->nPortalTextureID)
			continue;

		// Check frame count
		if (itPortal->second->nFrameSkip > nCurrFrameID - itPortal->second->nLastFrameID)
			continue;

		// Check if its visible
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(itPortal->first);
		if (NULL == pEntity) continue;
		AABB aabb; pEntity->GetWorldBounds(aabb);
		if (false == PlayerCam.IsAABBVisible_FM(aabb))
			continue;

		// Add it to the map
		UpdateMap.insert(PortalMap::value_type(itPortal->first, itPortal->second));
	}
	if (true == UpdateMap.empty()) return;

	// Retain current camera and viewport settings
	int nOldX, nOldY, nOldWidth, nOldHeight;
	ColorF cClear(Col_Black);
	pRenderer->GetViewport(&nOldX, &nOldY, &nOldWidth, &nOldHeight);
	pRenderer->SetViewport(0, 0, PORTAL_MAX_WIDTH, PORTAL_MAX_HEIGHT);

	// Update portals that need it
	for (PortalMap::iterator itPortal = UpdateMap.begin(); itPortal != UpdateMap.end(); itPortal++)
	{
		itPortal->second->nLastFrameID = nCurrFrameID;

		// Get the camera from the camera entity
		IEntity *pCameraEntity = gEnv->pEntitySystem->GetEntity(itPortal->second->nCameraEntityID);
		if (NULL == pCameraEntity) continue;
		IEntityCameraProxy *pProxy;
		if (NULL == (pProxy = (IEntityCameraProxy*)pCameraEntity->GetProxy(ENTITY_PROXY_CAMERA)))
			continue;
		CCamera PortalCam = pProxy->GetCamera();

		// Prepare for rendering
		pRenderer->ClearBuffer(FRT_CLEAR|FRT_CLEAR_IMMEDIATE, &cClear);
		pRenderer->SetCamera(PortalCam);
		
		// Render world using the portal camera
		p3DEngine->RenderWorld(0, PortalCam, "PortalRender");

		// Screen-to-Texture
		itPortal->second->RenderPortal(pRenderer);
	}

	// Reset back to old
	pRenderer->SetViewport(nOldX, nOldY, nOldWidth, nOldHeight);
	pRenderer->SetCamera(PlayerCam);
}

////////////////////////////////////////////////////
void CPortalManager::GetMemoryStatistics(ICrySizer *s)
{
	s->Add(*this);
	s->Add(*(SPortalDef::pBuffer));
	s->AddContainer(m_Portals);
	for (PortalMap::iterator itPortal = m_Portals.begin(); itPortal != m_Portals.end(); itPortal++)
	{
		s->Add(*(itPortal->second));
		s->Add(itPortal->second->szPortalTexture);
	}
}

////////////////////////////////////////////////////
bool CPortalManager::MakeEntityPortal(EntityId nEntityID, char const* szCameraEntity, 
									  char const* szTexture, int nFrameSkip)
{
	// Get the portal entity
	IEntity *pPortalEntity = gEnv->pEntitySystem->GetEntity(nEntityID);
	if (NULL == pPortalEntity) return false;

	// Get the camera entity
	IEntity *pCameraEntity = gEnv->pEntitySystem->FindEntityByName(szCameraEntity);
	if (NULL == pCameraEntity) return false;
	EntityId nCameraEntityID = pCameraEntity->GetId();

	// If entity acting as camera has no camera proxy, create one
	IEntityCameraProxy *pProxy = (IEntityCameraProxy*)pCameraEntity->GetProxy(ENTITY_PROXY_CAMERA);
	if (NULL == pProxy)
	{
		pProxy = (IEntityCameraProxy*)pCameraEntity->CreateProxy(ENTITY_PROXY_CAMERA);
		if (NULL == pProxy) return false;
	}

	// If there is already an entry, update it
	PortalMap::iterator itPortal = m_Portals.find(nEntityID);
	if (itPortal != m_Portals.end())
	{
		// Set the new properties
		itPortal->second->nCameraEntityID = nCameraEntityID;
		itPortal->second->szPortalTexture = szTexture;
		itPortal->second->nFrameSkip = nFrameSkip;
		itPortal->second->UpdateTextureInfo();

		// Update camera proxy
		if (itPortal->second->nPortalTextureID)
		{
			CCamera camera = pProxy->GetCamera();
			camera.SetFrustum(PORTAL_MAX_WIDTH, PORTAL_MAX_HEIGHT, camera.GetFov(), camera.GetNearPlane(), camera.GetFarPlane());
			pProxy->SetCamera(camera);
		}
		pCameraEntity->SetProxy(ENTITY_PROXY_CAMERA, pProxy);
		return true;
	}

	// Add entry
	SPortalDef *pDef = new SPortalDef;
	pDef->nEntityID = nEntityID;
	pDef->nCameraEntityID = nCameraEntityID;
	pDef->szPortalTexture = szTexture;
	pDef->nFrameSkip = nFrameSkip;
	pDef->UpdateTextureInfo();

	// Update camera proxy
	if (pDef->nPortalTextureID)
	{
		CCamera camera = pProxy->GetCamera();
		camera.SetFrustum(PORTAL_MAX_WIDTH, PORTAL_MAX_HEIGHT, camera.GetFov(), camera.GetNearPlane(), camera.GetFarPlane());
		pProxy->SetCamera(camera);
	}
	pCameraEntity->SetProxy(ENTITY_PROXY_CAMERA, pProxy);

	// Add to map
	m_Portals[nEntityID] = pDef;
	return true;
}

////////////////////////////////////////////////////
void CPortalManager::RemoveEntityPortal(EntityId nEntityID)
{
	// Find entry
	PortalMap::iterator itPortal = m_Portals.find(nEntityID);
	if (itPortal == m_Portals.end()) return;

	// Remove entry
	SAFE_DELETE(itPortal->second);
	m_Portals.erase(itPortal);
}