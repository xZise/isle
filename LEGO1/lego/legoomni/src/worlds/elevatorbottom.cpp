#include "elevatorbottom.h"

#include "elevbott_actions.h"
#include "isle.h"
#include "jukebox.h"
#include "jukebox_actions.h"
#include "legocontrolmanager.h"
#include "legogamestate.h"
#include "legoinputmanager.h"
#include "legomain.h"
#include "legovariables.h"
#include "misc.h"
#include "mxmisc.h"
#include "mxnotificationmanager.h"
#include "mxtransitionmanager.h"
#include "mxvariabletable.h"

DECOMP_SIZE_ASSERT(ElevatorBottom, 0xfc)

// FUNCTION: LEGO1 0x10017e90
ElevatorBottom::ElevatorBottom()
{
	NotificationManager()->Register(this);
	m_destLocation = LegoGameState::e_undefined;
}

// FUNCTION: LEGO1 0x10018060
ElevatorBottom::~ElevatorBottom()
{
	if (InputManager()->GetWorld() == this) {
		InputManager()->ClearWorld();
	}
	ControlManager()->Unregister(this);
	NotificationManager()->Unregister(this);
}

// FUNCTION: LEGO1 0x100180f0
MxResult ElevatorBottom::Create(MxDSAction& p_dsAction)
{
	MxResult result = LegoWorld::Create(p_dsAction);
	if (result == SUCCESS) {
		InputManager()->SetWorld(this);
		ControlManager()->Register(this);
	}

	SetIsWorldActive(FALSE);

	GameState()->m_currentArea = LegoGameState::e_elevbott;
	GameState()->StopArea(LegoGameState::e_previousArea);

	return result;
}

// FUNCTION: LEGO1 0x10018150
// FUNCTION: BETA10 0x10027d60
MxLong ElevatorBottom::Notify(MxParam& p_param)
{
	MxNotificationParam& param = (MxNotificationParam&) p_param;
	MxLong ret = 0;
	LegoWorld::Notify(p_param);

	if (m_worldStarted) {
		switch (param.GetNotification()) {
		case c_notificationControl:
			ret = HandleControl((LegoControlManagerNotificationParam&) p_param);
			break;
		case c_notificationTransitioned:
			assert(m_destLocation != LegoGameState::e_undefined);
			GameState()->SwitchArea(m_destLocation);
			break;
		}
	}

	return ret;
}

// FUNCTION: LEGO1 0x100181b0
void ElevatorBottom::ReadyWorld()
{
	LegoWorld::ReadyWorld();
	PlayMusic(JukeboxScript::c_InformationCenter_Music);
	Disable(FALSE, LegoOmni::c_disableInput | LegoOmni::c_disable3d | LegoOmni::c_clearScreen);
}

// FUNCTION: LEGO1 0x100181d0
MxLong ElevatorBottom::HandleControl(LegoControlManagerNotificationParam& p_param)
{
	MxLong result = 0;

	if (p_param.m_enabledChild == 1) {
		switch (p_param.m_clickedObjectId) {
		case ElevbottScript::c_LeftArrow_Ctl:
			m_destLocation = LegoGameState::e_infodoor;
			TransitionManager()->StartTransition(MxTransitionManager::e_mosaic, 50, FALSE, FALSE);
			result = 1;
			break;
		case ElevbottScript::c_RightArrow_Ctl:
			m_destLocation = LegoGameState::e_infomain;
			TransitionManager()->StartTransition(MxTransitionManager::e_mosaic, 50, FALSE, FALSE);
			result = 1;
			break;
		case ElevbottScript::c_ElevBott_Elevator_Ctl:
			LegoGameState* gs = GameState();
			Act1State* state = (Act1State*) gs->GetState("Act1State");

			if (state == NULL) {
				state = (Act1State*) gs->CreateState("Act1State");
			}

			state->SetElevatorFloor(Act1State::c_floor1);
			m_destLocation = LegoGameState::e_elevride;
			TransitionManager()->StartTransition(MxTransitionManager::e_mosaic, 50, FALSE, FALSE);
			VariableTable()->SetVariable(g_varCAMERALOCATION, "LCAMZI1,90");
			result = 1;
			break;
		}
	}

	return result;
}

// FUNCTION: LEGO1 0x100182c0
void ElevatorBottom::Enable(MxBool p_enable)
{
	LegoWorld::Enable(p_enable);

	if (p_enable) {
		InputManager()->SetWorld(this);
		SetIsWorldActive(FALSE);
	}
	else {
		if (InputManager()->GetWorld() == this) {
			InputManager()->ClearWorld();
		}
	}
}

// FUNCTION: LEGO1 0x10018310
MxBool ElevatorBottom::Escape()
{
	DeleteObjects(&m_atomId, ElevbottScript::c_iica31in_PlayWav, 999);
	m_destLocation = LegoGameState::e_infomain;
	return TRUE;
}
