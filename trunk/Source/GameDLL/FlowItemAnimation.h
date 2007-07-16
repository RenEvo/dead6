#ifndef __FLOWITEMANIMATIOn_H__
#define __FLOWITEMANIMATIOn_H__

#include "Game.h"
#include "Item.h"
#include "Nodes/G2FlowBaseNode.h"

class CFlowItemAnimation : public CFlowBaseNode
{
public:
	CFlowItemAnimation( SActivationInfo * pActInfo )
	{
	}

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowItemAnimation(pActInfo);
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Activate", _HELP("Play the animation.")),
			InputPortConfig<EntityId>("ItemId", _HELP("Set item that will play the animation.")),
			InputPortConfig<bool>("Busy", true, _HELP("Set item as busy while the animation is playing.")),
			InputPortConfig<string>("Animation", _HELP("Set the animation to be played.")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.sDescription = _HELP("Plays an animation on an item.");
		config.SetCategory(EFLN_WIP);
	}

	struct FlowPlayItemAnimationAction
	{
		FlowPlayItemAnimationAction(const char *_anim, bool _busy): anim(_anim), busy(_busy) {};
		string anim;
		bool busy;

		// nested action ;o
		struct FlowPlayItemAnimationActionUnbusy
		{
			void execute(CItem *pItem)
			{
				pItem->SetBusy(false);
			}
		};


		void execute(CItem *pItem)
		{
			pItem->PlayAnimation(anim.c_str());

			if (busy)
			{
				pItem->SetBusy(true);

				pItem->GetScheduler()->TimerAction(pItem->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<FlowPlayItemAnimationActionUnbusy>::Create(), false);
			}
		}
	};

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, 0))
			{
				CItem *pItem = static_cast<CItem*>(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetPortEntityId(pActInfo, 1)));
				
				if (pItem)
				{
					pItem->GetScheduler()->ScheduleAction(CSchedulerAction<FlowPlayItemAnimationAction>::Create(FlowPlayItemAnimationAction(GetPortString(pActInfo, 3), GetPortBool(pActInfo, 2))));
				}
			}
			break;
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

#endif //__FLOWITEMANIMATIOn_H__
