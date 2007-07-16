#include <IItemSystem.h>
#include "Item.h"
#include "IShader.h"

class CFlashlight : public CItem
{
protected:
	typedef struct SFlashlightParams
	{

		string projectTexture;
		string helper;
		float radius;
		float projectFOV;
		float diffusemultiplier;
		Vec3 color;
		Vec3 specular;

	} SFlashlightParams;
public:
	virtual bool Init(IGameObject * pGameObject );
	virtual void PostInit(IGameObject * pGameObject );
	virtual void OnReset();
	virtual void Update(SEntityUpdateContext& ctx, int slot);
	virtual void ActivateLight(bool activate);
	virtual void OnAttach(bool attach);
	virtual void OnParentSelect(bool select);
private:
	uint m_lightID;
	uint m_lightID_tp;
	bool m_activated;
	string m_projectTexture;
	float m_radius;
	float m_projectFOV;
	Vec3 m_color;
	Vec3 m_specular;
	float m_diffuseMultiplier;
};