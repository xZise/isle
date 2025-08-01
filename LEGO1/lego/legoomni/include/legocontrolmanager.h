#ifndef LEGOCONTROLMANAGER_H
#define LEGOCONTROLMANAGER_H

#include "legoeventnotificationparam.h"
#include "legoinputmanager.h"
#include "mxcore.h"
#include "mxpresenterlist.h"

class MxControlPresenter;

// VTABLE: LEGO1 0x100d6a98
// SIZE 0x2c
class LegoControlManagerNotificationParam : public LegoEventNotificationParam {
public:
	LegoControlManagerNotificationParam() : LegoEventNotificationParam()
	{
		m_clickedObjectId = -1;
		m_clickedAtom = NULL;
	}

	// TODO: Most likely getters/setters are not used according to BETA.

	const char* GetClickedAtom() const { return m_clickedAtom; }

	void SetClickedObjectId(MxS32 p_clickedObjectId) { m_clickedObjectId = p_clickedObjectId; }
	void SetClickedAtom(const char* p_clickedAtom) { m_clickedAtom = p_clickedAtom; }
	void SetEnabledChild(MxS16 p_enabledChild) { m_enabledChild = p_enabledChild; }

	MxS32 m_clickedObjectId;   // 0x20
	const char* m_clickedAtom; // 0x24
	MxS16 m_enabledChild;      // 0x28
};

// SYNTHETIC: LEGO1 0x10028bf0
// LegoControlManagerNotificationParam::`scalar deleting destructor'

// SYNTHETIC: LEGO1 0x10028c60
// LegoControlManagerNotificationParam::~LegoControlManagerNotificationParam

// VTABLE: LEGO1 0x100d6a80
// VTABLE: BETA10 0x101bc610
class LegoControlManager : public MxCore {
public:
	LegoControlManager();
	~LegoControlManager() override; // vtable+0x00

	MxResult Tickle() override; // vtable+0x08

	// FUNCTION: BETA10 0x1008af70
	static const char* HandlerClassName()
	{
		// STRING: LEGO1 0x100f31b8
		return "LegoControlManager";
	}

	// FUNCTION: LEGO1 0x10028cb0
	// FUNCTION: BETA10 0x1008af40
	const char* ClassName() const override // vtable+0x0c
	{
		return HandlerClassName();
	}

	// FUNCTION: LEGO1 0x10028cc0
	MxBool IsA(const char* p_name) const override // vtable+0x10
	{
		return !strcmp(p_name, LegoControlManager::ClassName()) || MxCore::IsA(p_name);
	}

	void SetPresenterList(MxPresenterList* p_presenterList);
	void Register(MxCore* p_listener);
	void Unregister(MxCore* p_listener);
	MxBool HandleButtonDown(LegoEventNotificationParam& p_param, MxPresenter* p_presenter);
	void UpdateEnabledChild(MxU32 p_objectId, const char* p_atom, MxS16 p_enabledChild);
	MxControlPresenter* GetControlAt(MxS32 p_x, MxS32 p_y);
	MxBool HandleButtonDown();
	MxBool HandleButtonUp();
	void Notify();

	MxU32 HandleUpNextTickle() { return m_handleUpNextTickle; }
	MxBool IsSecondButtonDown() { return m_secondButtonDown; }

	// SYNTHETIC: LEGO1 0x10028d40
	// LegoControlManager::`scalar deleting destructor'

private:
	enum {
		e_idle = 0,
		e_waitNextTickle = 1,
		e_tickled = 2,
	};

	MxU32 m_buttonDownState;                     // 0x08
	MxU32 m_handleUpNextTickle;                  // 0x0c
	MxBool m_secondButtonDown;                   // 0x10
	MxPresenter* m_handledPresenter;             // 0x14
	LegoControlManagerNotificationParam m_event; // 0x18
	MxPresenterList* m_presenterList;            // 0x44
	LegoNotifyList m_notifyList;                 // 0x48
};

#endif // LEGOCONTROLMANAGER_H
