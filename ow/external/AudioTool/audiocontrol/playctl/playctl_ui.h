#ifndef	playctl_HEADER
#define	playctl_HEADER

//
// playctl_ui.h - User interface object and function declarations.
// This file was generated by `gxv++' from `playctl.G'.
// DO NOT EDIT BY HAND.
//


extern Attr_attribute	INSTANCE;

Xv_opaque	playctl_PlayctlPanel_menu_create(caddr_t, Xv_opaque);

class playctl_PlayctlPanel_objects {

public:
	Xv_opaque	PlayctlPanel;
	Xv_opaque	PlayctlCanvas;
	Xv_opaque	Outport0;
	Xv_opaque	Outport1;
	Xv_opaque	Outport2;
	Xv_opaque	PlayOutputGroup;
	Xv_opaque	RecpanelButton;
	Xv_opaque	RecordButtonGroup;
	Xv_opaque	MuteButton;
	Xv_opaque	PlayMuteGroup;
	Xv_opaque	PlayVolumeMsg;
	Xv_opaque	PlayVolumeSlider;
	Xv_opaque	PlaySliderGroup;
	Xv_opaque	PlayBalanceMsg;
	Xv_opaque	PlayBalanceLeftMsg;
	Xv_opaque	PlayBalanceSlider;
	Xv_opaque	PlayBalanceLeftSliderGroup;
	Xv_opaque	PlayBalanceRightMsg;
	Xv_opaque	PlayBalanceSliderGroup;
	Xv_opaque	PlayBalanceGroup;

	virtual void	objects_initialize(Xv_opaque);

	virtual Xv_opaque	PlayctlPanel_create(Xv_opaque);
	virtual Xv_opaque	PlayctlCanvas_create(Xv_opaque);
	virtual Xv_opaque	Outport0_create(Xv_opaque);
	virtual Xv_opaque	Outport1_create(Xv_opaque);
	virtual Xv_opaque	Outport2_create(Xv_opaque);
	virtual Xv_opaque	PlayOutputGroup_create(Xv_opaque);
	virtual Xv_opaque	RecpanelButton_create(Xv_opaque);
	virtual Xv_opaque	RecordButtonGroup_create(Xv_opaque);
	virtual Xv_opaque	MuteButton_create(Xv_opaque);
	virtual Xv_opaque	PlayMuteGroup_create(Xv_opaque);
	virtual Xv_opaque	PlayVolumeMsg_create(Xv_opaque);
	virtual Xv_opaque	PlayVolumeSlider_create(Xv_opaque);
	virtual Xv_opaque	PlaySliderGroup_create(Xv_opaque);
	virtual Xv_opaque	PlayBalanceMsg_create(Xv_opaque);
	virtual Xv_opaque	PlayBalanceLeftMsg_create(Xv_opaque);
	virtual Xv_opaque	PlayBalanceSlider_create(Xv_opaque);
	virtual Xv_opaque	PlayBalanceLeftSliderGroup_create(Xv_opaque);
	virtual Xv_opaque	PlayBalanceRightMsg_create(Xv_opaque);
	virtual Xv_opaque	PlayBalanceSliderGroup_create(Xv_opaque);
	virtual Xv_opaque	PlayBalanceGroup_create(Xv_opaque);
};
#endif
