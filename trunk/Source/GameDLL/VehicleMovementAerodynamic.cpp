/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Aerodynamic vehicle base class

-------------------------------------------------------------------------
History:
- 14:07:2005: Created by Julien Darre

*************************************************************************/
#include "StdAfx.h"
#include "VehicleMovementAerodynamic.h"
#include "ICryPak.h"
#include "IRenderAuxGeom.h"

//-----------------------------------------------------------------------------------------------------

CVehicleMovementAerodynamic::CVehicleMovementAerodynamic()
{
	m_pEntity					= NULL;	
}



//-----------------------------------------------------------------------------------------------------

void CVehicleMovementAerodynamic::ResetTextPos()
{
	m_fTextY = 100.0f;
}

//-----------------------------------------------------------------------------------------------------

void CVehicleMovementAerodynamic::DumpText(const char *_strFormat,...)
{
  if (!m_pVehicle || IsProfilingMovement()) 
	{    
		char acBuffer[512];
		va_list args;
		va_start(args,_strFormat);
		vsprintf(acBuffer,_strFormat,args);
		va_end(args);

		static float s_afColor[4] = {1,1,1,1};

		gEnv->pRenderer->Draw2dLabel(5.0f,m_fTextY+=15.0f,1.5f,s_afColor,false,acBuffer);
	}
}

//-----------------------------------------------------------------------------------------------------

void CVehicleMovementAerodynamic::DumpVector(string _strName,const Vec3 *_pv3)
{
	DumpText("%s",_strName.c_str());
	DumpText("X=%f",_pv3->x);
	DumpText("Y=%f",_pv3->y);
	DumpText("Z=%f\n",_pv3->z);
}

//-----------------------------------------------------------------------------------------------------

void CVehicleMovementAerodynamic::DumpMatrix(const Matrix34 *_pmat34)
{
	Vec3 tmp;
	tmp = _pmat34->GetColumn(0);
	DumpVector("Right", &tmp);
	tmp = _pmat34->GetColumn(1);
	DumpVector("Look", &tmp);
	tmp = _pmat34->GetColumn(2);
	DumpVector("Up", &tmp);
	tmp = _pmat34->GetColumn(3);
	DumpVector("Pos", &tmp);
}

//-----------------------------------------------------------------------------------------------------

void CVehicleMovementAerodynamic::ReadFile(string _strFile,TPointsMap *_pPointsMap)
{
	ICryPak *pCryPak = gEnv->pCryPak;
	assert(pCryPak);

	FILE *pFile = pCryPak->FOpen(_strFile.c_str(),"r");
	if(pFile)
	{
		char acBuffer[256];
		while(pCryPak->FGets(acBuffer,256,pFile))
		{
			float fX;
			float fY;
			sscanf(acBuffer,"%f %f\n",&fX,&fY);

			_pPointsMap->insert(std::make_pair(fX,fY));
		}
		pCryPak->FClose(pFile);
	}
}

//-----------------------------------------------------------------------------------------------------

float CVehicleMovementAerodynamic::GetCoefficient(TPointsMap *_pPointsMap,float _fAngleOfAttack)
{
	for(TPointsMap::iterator iter=_pPointsMap->begin(); iter!=_pPointsMap->end(); ++iter)
	{
		float fAngleOfAttack = (*iter).first;
		TPointsMap::iterator iterNext = iter;
		iterNext++;

		if(iterNext == _pPointsMap->end())
		{
			return 0.0f;
		}

		float fNextAngleOfAttack = (*iterNext).first;

		if(	_fAngleOfAttack >= fAngleOfAttack && 
				_fAngleOfAttack <= fNextAngleOfAttack)
		{
			float fCoefficient1 = (*iter).second;
			float fCoefficient2 = (*iterNext).second;

			return fCoefficient1 + (fCoefficient2 - fCoefficient1) * (_fAngleOfAttack - fAngleOfAttack) / (fNextAngleOfAttack - fAngleOfAttack);
		}
	}

	assert(0);

	return 0.0f;
}

//-----------------------------------------------------------------------------------------------------

void CVehicleMovementAerodynamic::AddSphere(Vec3 *_pvPos,float _fRadius,float _fMass,int _iID)
{
	IGeomManager *pGeomManager = gEnv->pPhysicalWorld->GetGeomManager();

	primitives::sphere Sphere;
	Sphere.center.Set(0.0f,0.0f,0.0f);
	Sphere.r = _fRadius;
	IGeometry *pGeometry = pGeomManager->CreatePrimitive(primitives::sphere::type,&Sphere);
	phys_geometry *pPhysGeometry = pGeomManager->RegisterGeometry(pGeometry);
	pGeometry->Release();

	pe_geomparams GeomParams;
	GeomParams.pos = *_pvPos;
	GeomParams.mass = _fMass;
	
  if (IPhysicalEntity* pPhysics = GetPhysics())
    pPhysics->AddGeometry(pPhysGeometry,&GeomParams,_iID);

	pGeomManager->UnregisterGeometry(pPhysGeometry);
}

//-----------------------------------------------------------------------------------------------------

int CVehicleMovementAerodynamic::AddBox(Vec3 *_pvPos,Vec3 *_pvSize,float _fMass,int _iID/*=-1*/)
{
  IPhysicalEntity* pPhysics = GetPhysics();
	IGeomManager *pGeomManager = gEnv->pPhysicalWorld->GetGeomManager();

	primitives::box Box;
	Box.Basis.SetIdentity();
	Box.center.Set(0.0f,0.0f,0.0f);
	Box.size = (*_pvSize) / 2.0f;
	Box.bOriented = 0;
	IGeometry *pGeometry = pGeomManager->CreatePrimitive(primitives::box::type,&Box);
	phys_geometry *pPhysGeometry = pGeomManager->RegisterGeometry(pGeometry);
	pGeometry->Release();

	pe_geomparams partpos;
	partpos.pos = *_pvPos;
	partpos.mass = _fMass;
	int id = pPhysics->AddGeometry(pPhysGeometry,&partpos,_iID);

	pGeomManager->UnregisterGeometry(pPhysGeometry);

  return id;
}

//-----------------------------------------------------------------------------------------------------

void CVehicleMovementAerodynamic::AddForce(Vec3 *_pvForce,Vec3 *_pvPos,ColorF _Color)
{
	Vec3 vPoint = m_pEntity->GetWorldTM().TransformPoint(*_pvPos);

	pe_action_impulse actionImpulse;
	actionImpulse.impulse = *_pvForce;
	actionImpulse.point = vPoint;
	GetPhysics()->Action(&actionImpulse);

//	return;

	float fScale = 4.0f;
	Vec3 p1 = actionImpulse.point;
	Vec3 p2 = p1 + actionImpulse.impulse * fScale;
	DrawLine(&p1,&p2,_Color);
}

//-----------------------------------------------------------------------------------------------------

void CVehicleMovementAerodynamic::DrawLine(Vec3 *_pvPoint1,Vec3 *_pvPoint2,ColorF _Color)
{
  if (IsProfilingMovement())
	  gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(*_pvPoint1,_Color,*_pvPoint2,_Color);
}

//-----------------------------------------------------------------------------------------------------

void CVehicleMovementAerodynamic::UpdateWing(SWing *_pWing,float _fAngle,float _fDeltaTime)
{
	Matrix34 matWing = m_pEntity->GetWorldTM() * Matrix33::CreateRotationXYZ(Ang3(DEG2RAD(_pWing->fAngleX),
																																								DEG2RAD(_pWing->fAngleY),
																																								DEG2RAD(_pWing->fAngleZ))).GetInverted();

	Vec3 vRight	= matWing.GetColumn(0);
	Vec3 vLook	= matWing.GetColumn(1);
	Vec3 vUp		= matWing.GetColumn(2);

	pe_status_dynamics StatusDynamics;
	GetPhysics()->GetStatus(&StatusDynamics);

	// v(relativepoint) = v + w^(relativepoint-center)
	Vec3 vVelocity = StatusDynamics.v + StatusDynamics.w.Cross(m_pEntity->GetWorldTM().TransformVector(_pWing->vPos));
	Vec3 vVelocityNormalized = vVelocity.GetNormalizedSafe(vLook);

	// TODO:

	float fAngleOfAttack = RAD2DEG(asin(vRight.Dot(vVelocityNormalized.Cross(vLook))));

	DumpText("AoA=%f",fAngleOfAttack);

	fAngleOfAttack += _fAngle;

	float Cl = GetCoefficient(_pWing->pLiftPointsMap,fAngleOfAttack) * _pWing->fCl;
	float Cd = GetCoefficient(_pWing->pDragPointsMap,fAngleOfAttack) * _pWing->fCd;

	Vec3 vVelocityNormal = vRight.Cross(vVelocityNormalized).GetNormalized();

	float fVelocitySquared = vVelocity.len2();

	const float c_fDynamicPressure = 1.293f;

	float fLift = 0.5f * c_fDynamicPressure * _pWing->fSurface * Cl * fVelocitySquared * _fDeltaTime;
	float fDrag = 0.5f * c_fDynamicPressure * _pWing->fSurface * Cd * fVelocitySquared * _fDeltaTime;

	Vec3 vLiftImpulse = +fLift * vVelocityNormal;
	Vec3 vDragImpulse = -fDrag * vVelocityNormalized;

	AddForce(&vLiftImpulse,&_pWing->vPos,ColorF(0,1,0,1));
	AddForce(&vDragImpulse,&_pWing->vPos,ColorF(1,0,1,1));
}

//-----------------------------------------------------------------------------------------------------
