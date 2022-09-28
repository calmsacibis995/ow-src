##
#
# Makefile for common images, icons, cursors, and pixrects
#
# @(#)Makefile	1.43 92/10/13
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

MAKE.install.target	= install_file postinstall
INSTALL.file.dir	= ${DEST.include.images}

include ${RULESET.include}

postinstall:
	${RM} $(INSTALL.file.dir)/clocktool.icon
	ln -s clock.icon $(INSTALL.file.dir)/clocktool.icon
	${RM} $(INSTALL.file.dir)/clocktool.rom.icon
	ln -s clock.rom.icon $(INSTALL.file.dir)/clocktool.rom.icon

FILE			= \
	abc.pr abstk.pr abstk_up.pr basic_cursor.pr basic_text_cursor.pr \
	bg1.xbm bg2.xbm bg3.xbm black.cursor \
	black.icon blank.pr boggle.icon box.pr busy.cursor \
	bullseye.cursor button_stack.pr canfield.icon canvas_demo.icon \
	cardback.icon \
	chess.clock.icon chesstool.icon choice_12_off.pr choice_12_on.pr \
	choice_14_off.pr choice_14_on.pr choice_16_off.pr choice_16_on.pr \
	choice_19_off.pr choice_19_on.pr choice_24_off.pr choice_24_on.pr \
	choice_32_off.pr choice_32_on.pr choice_stack.pr circle.pr \
	circle_0.pr circle_25.pr circle_50.pr circle_75.pr circle_black.pr \
	circle_root.pr clock.icon clock.rom.icon cmdtool.icon \
	cmdtool_mask.icon \
	cmrb14.pr cmrr14.pr compose.icon confirm.cursor confirm.pr \
	confirm_left.pr confirm_middle.pr confirm_right.pr \
	console.icon console_mask.icon copy.cursor \
	copy_cursor.pr copyletter.cursor copyletter.pr \
	copyletters.cursor copyletters.pr \
	core_eye.icon \
	cursor_demo.icon cycle.pr dbxtool.icon dead.icon default.icon \
	default_pin_out.pr defaultsedit.icon down_arrow.pr dup.cursor \
	dup_cursor.pr empty_letter.icon equal.icon \
	focus_right.cursor focus_up.cursor \
	folder.pr fontedit.icon \
	fv.icon gallant19.pr gallantr19.pr gammon.icon gfxtool.icon \
	glass.cursor gr_25.cursor gr_50.cursor gr_75.cursor gr_root.cursor\
	grid.pr grey_stencil_50.pr \
	hello_world.icon hglass.cursor ibeam.cursor iconedit.icon \
	indentdefaults.icon jump1.icon jump2.icon jump3.icon jump4.icon \
	jump5.icon jump6.icon k1.pr k2.pr k3.pr k4.pr k5.pr letter.pr \
	letters.pr line.pr \
	life.icon lockscreen.icon mail.icon mailseen.icon mglass.icon \
	mglass_mask.icon mouse_left_hand.pr \
	mouse_left_hand_grey.pr mouse_mid_hand.pr mouse_mid_hand_grey.pr \
	move.cursor move_h.cursor \
	move_v.cursor move_cursor.pr \
	moveletter.cursor moveletter.pr moveletters.cursor moveletters.pr \
	nautilus.icon nomail.icon none.cursor off.pr on.pr overview.icon \
	painting_hand.pr panel_button.pr panel_button_arrow.pr \
	panel_check_choice.pr panel_check_off.pr panel_check_on.pr \
	panel_choose_many.pr panel_choose_one.pr panel_flush.pr \
	panel_choice_on.pr panel_choice_off.pr panel_default_button.pr \
	panel_left_arrow.pr panel_right_arrow.pr panel_menu_on.pr \
	panel_pulldown.pr panel_pullright.pr \
	panel_switch.pr panel_text.pr pin_in.pr pin_out.pr \
	point.pr plus.icon PSdoc.icon qmark.cursor quick_move_cursor.pr \
	reply.icon rspeedometer.icon screenb12.pr \
	screenb14.pr screenr11.pr screenr12.pr screenr14.pr screenr7.pr \
	scrollbar_demo.icon shine.pr size_pointer.pr speedometer.icon  \
	square_0.pr square_17.pr square_25.pr square_50.pr square_75.pr \
	square_80.pr square_black.pr square_root.pr stretchNE.cursor \
	stretchNW.cursor stretchSE.cursor stretchSW.cursor stretch_h.cursor \
	stretch_v.cursor sun30.icon sunview_manual.icon switcher.icon \
	tektool.icon template.icon terminal.icon terminal_mask.icon \
	text_move_cursor.pr textedit.icon \
	toggle_12_off.pr toggle_12_on.pr toggle_14_off.pr toggle_14_on.pr \
	toggle_16_off.pr toggle_16_on.pr toggle_19_off.pr toggle_19_on.pr \
	toggle_24_off.pr toggle_24_on.pr toggle_32_off.pr toggle_32_on.pr \
	traffic.icon tri_down.pr tri_left.pr tri_right.pr \
	tri_up.pr undelete.image white.icon white.pr \
	alert_bang32.pr alert_qmark32.pr \
	down_button_10.pr down_button_12.pr down_button_14.pr \
	down_button_19.pr up_button_10.pr up_button_12.pr \
	up_button_14.pr up_button_19.pr \
	bind.icon calctool.icon compress.icon coredoc.icon dos.icon\
	draw.icon drawdoc.icon fm_clock.icon fm_cmdtool.icon fm_dbxtool.icon\
	fm_fontedit.icon fm_iconedit.icon fm_mailtool.icon fm_textedit.icon\
	fm_traffic.icon fontdoc.icon framedoc.icon filemgr.icon icondoc.icon\
	maildoc.icon maze.icon paintdoc.icon perfmeter.icon printer.icon\
	puzzle.icon rolo.icon sched.icon snapshot.icon spreadsheet.icon\
	sundraw.icon sunpaint.icon sunwrite.icon tapetool.icon vitool.icon\
	writedoc.icon xsol.icon \
	fl_arrow.xbm fl_doc.xbm fl_folder.xbm

