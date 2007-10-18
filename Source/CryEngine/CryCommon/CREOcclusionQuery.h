 
#ifndef __CREOCCLUSIONQUERY_H__
#define __CREOCCLUSIONQUERY_H__

//=============================================================

class CREOcclusionQuery : public CRendElement
{
  friend class CRender3D;
public:

  int m_nVisSamples;
  int m_nCheckFrame;
  int m_nDrawFrame;
	Vec3 m_vBoxMin;
	Vec3 m_vBoxMax;
  
  UINT_PTR m_nOcclusionID; // this will carry a pointer LPDIRECT3DQUERY9, so it needs to be 64-bit on WIN64 

	CRenderMesh * m_pRMBox;
	static uint m_nQueriesPerFrameCounter;
	static uint m_nReadResultNowCounter;
	static uint m_nReadResultTryCounter;

  CREOcclusionQuery()
  {
    m_nOcclusionID = 0; 

    m_nVisSamples = 800*600;
    m_nCheckFrame = 0;
    m_nDrawFrame = 0;
		m_vBoxMin=Vec3(0,0,0);
		m_vBoxMax=Vec3(0,0,0);
		m_pRMBox=NULL;

		mfSetType(eDATA_OcclusionQuery);
    mfUpdateFlags(FCEF_TRANSFORM);
  }

  virtual ~CREOcclusionQuery();

  virtual void mfPrepare();
  virtual bool mfDraw(CShader *ef, SShaderPass *sfm);
  virtual void mfReset();
	virtual bool mfReadResult_Try();
	virtual bool mfReadResult_Now();
};

#endif  // __CREOCCLUSIONQUERY_H__
