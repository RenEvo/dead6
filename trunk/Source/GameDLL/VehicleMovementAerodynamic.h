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
#ifndef __VEHICLEMOVEMENTAERODYNAMIC_H__
#define __VEHICLEMOVEMENTAERODYNAMIC_H__

//-----------------------------------------------------------------------------------------------------

#include <map>
#include "VehicleMovementBase.h"

//-----------------------------------------------------------------------------------------------------

typedef std::map<float,float> TPointsMap;

struct SWing
{
  Vec3 vPos;
  Vec3 vSize;
  float fAngleX;
  float fAngleY;
  float fAngleZ;
  float fMass;
  float fSurface;
  float fCl;
  float fCd;

  TPointsMap *pLiftPointsMap;
  TPointsMap *pDragPointsMap;
};

//-----------------------------------------------------------------------------------------------------

class CVehicleMovementAerodynamic : public CVehicleMovementBase 
{
protected:
  CVehicleMovementAerodynamic();
  virtual	~CVehicleMovementAerodynamic(){}

  virtual EVehicleMovementType GetMovementType() { return eVMT_Air; }

	float GetCoefficient(TPointsMap *,float);

	void ResetTextPos();
	void DumpText(const char *,...) PRINTF_PARAMS(2, 3);
	void DumpVector(string,const Vec3 *);
	void DumpMatrix(const Matrix34 *);
	void DrawLine(Vec3 *,Vec3 *,ColorF);

	void ReadFile(string,TPointsMap *);

	void AddForce(Vec3 *,Vec3 *,ColorF);
	void AddSphere(Vec3 *,float,float,int);
	int AddBox(Vec3 *,Vec3 *,float,int _iID=-1); // -1 => assigned by physics
	void UpdateWing(SWing *,float,float);

private:
  float m_fTextY;
};


//-----------------------------------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------------------------------
