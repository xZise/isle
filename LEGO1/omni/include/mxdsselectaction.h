#ifndef MXDSSELECTACTION_H
#define MXDSSELECTACTION_H

#include "decomp.h"
#include "mxdsparallelaction.h"
#include "mxstringlist.h"

// VTABLE: LEGO1 0x100dcfc8
// VTABLE: BETA10 0x101c29d0
// SIZE 0xb0
class MxDSSelectAction : public MxDSParallelAction {
public:
	MxDSSelectAction();
	MxDSSelectAction(MxDSSelectAction& p_dsSelectAction);
	~MxDSSelectAction() override;

	void CopyFrom(MxDSSelectAction& p_dsSelectAction);
	MxDSSelectAction& operator=(MxDSSelectAction& p_dsSelectAction);

	// FUNCTION: LEGO1 0x100cb6f0
	// FUNCTION: BETA10 0x1015b460
	const char* ClassName() const override // vtable+0x0c
	{
		// STRING: LEGO1 0x1010261c
		return "MxDSSelectAction";
	}

	// FUNCTION: LEGO1 0x100cb700
	// FUNCTION: BETA10 0x1015b480
	MxBool IsA(const char* p_name) const override // vtable+0x10
	{
		return !strcmp(p_name, MxDSSelectAction::ClassName()) || MxDSParallelAction::IsA(p_name);
	}

	MxU32 GetSizeOnDisk() override;                              // vtable+0x18
	void Deserialize(MxU8*& p_source, MxS16 p_unk0x24) override; // vtable+0x1c
	MxDSAction* Clone() override;                                // vtable+0x2c

	// SYNTHETIC: LEGO1 0x100cb840
	// SYNTHETIC: BETA10 0x1015b4e0
	// MxDSSelectAction::`scalar deleting destructor'

private:
	MxString m_unk0x9c;
	MxStringList* m_unk0xac;
};

// SYNTHETIC: LEGO1 0x100cbbd0
// SYNTHETIC: BETA10 0x1015bb60
// MxStringListCursor::`scalar deleting destructor'

// TEMPLATE: LEGO1 0x100cbc40
// TEMPLATE: BETA10 0x1015bba0
// MxListCursor<MxString>::~MxListCursor<MxString>

// SYNTHETIC: LEGO1 0x100cbc90
// SYNTHETIC: BETA10 0x1015bc00
// MxListCursor<MxString>::`scalar deleting destructor'

// FUNCTION: LEGO1 0x100cbd00
// FUNCTION: BETA10 0x1015bc40
// MxStringListCursor::~MxStringListCursor

#endif // MXDSSELECTACTION_H
