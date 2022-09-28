#
# @(#)Makefile 1.12 91/12/04; SMI
#

SOURCEDIR	= ../../..
TOOLS_DIR	= ${SOURCEDIR}/external/DeskSet/V3_release

include ${SOURCEDIR}/config/Master.cfg

PSFFM.binder	= binder.message.po
PSFFM.calctool	= calctool.label.po \
		  calctool.message.po
PSFFM.ce	= ce-msg.po
PSFFM.clock	= clock.label.po \
		  clock.message.po
PSFFM.cm	= cm.err_msgs.po \
		  cm.labels.po \
		  cm.messages.po
PSFFM.cmdtool	= cmdtool.po
PSFFM.colorchooser = colorchooser.po
PSFFM.fm	= fm.error.po \
		  fm.label.po \
		  fm.message.po
PSFFM.helpviewer= viewer.cat.po
PSFFM.iconedit  = iconedit.messages.po
PSFFM.maillib	= maillib.po
PSFFM.mailtool	= mailtool.text.po
PSFFM.olwm	= olwm_messages.po
PSFFM.pageview	= pageview.error.po \
		  pageview.label.po \
		  pageview.message.po
PSFFM.perfmeter = perfmeter.label.po \
		  perfmeter.message.po
PSFFM.pm	= pm_errors.po \
		  pm_ui.po
PSFFM.printtool = printtool.message.po
PSFFM.props	= props.po
PSFFM.snapshot	= snapshot.label.po \
		  snapshot.message.po
PSFFM.tapetool	= tapetool.message.po
PSFFM.textedit	= textedit.message.po
PSFFM.tooltalk	= Sun_ToolTalk.po
PSFFM.xview	= xv_messages.po

PSFFM_FILES	= ${PSFFM.binder} \
		  ${PSFFM.calctool} \
		  ${PSFFM.ce} \
		  ${PSFFM.clock} \
		  ${PSFFM.cm} \
		  ${PSFFM.cmdtool} \
		  ${PSFFM.colorchooser} \
		  ${PSFFM.fm} \
		  ${PSFFM.helpviewer} \
		  ${PSFFM.iconedit} \
		  ${PSFFM.maillib} \
		  ${PSFFM.mailtool} \
		  ${PSFFM.olwm} \
		  ${PSFFM.pageview} \
		  ${PSFFM.perfmeter} \
		  ${PSFFM.pm} \
		  ${PSFFM.printtool} \
		  ${PSFFM.props} \
		  ${PSFFM.snapshot} \
		  ${PSFFM.tapetool} \
		  ${PSFFM.textedit} \
		  ${PSFFM.tooltalk} \
		  ${PSFFM.xview}
		    

SCRIPTS		= binder_msg \
		  calctool_msg \
		  ce_msg \
		  clock_msg \
		  cm_msg \
		  cmdtool_msg \
		  colorchooser_msg \
		  fm_msg \
		  helpviewer_msg \
		  iconedit_msg \
		  maillib_msg \
		  mailtool_msg \
		  olwm_msg \
		  pageview_msg \
		  perfmeter_msg \
		  pm_msg \
		  printtool_msg \
		  props_msg \
		  snapshot_msg \
		  tapetool_msg \
		  textedit_msg \
		  xview_msg

TARGETS		= ${SCRIPTS:%_msg=%}

MAKE.install.target	= # install_file
INSTALL.file.dir	= #

all:: tooltalk

prep: $$@_msg
	sh $@_msg ${SOURCEDIR} ${XGETTEXT} ${MSGFMT}

${TARGETS}: $$@_msg
	sh $@_msg ${SOURCEDIR} ${XGETTEXT} ${MSGFMT}

tooltalk:
	rm -f ${PSFFM.tooltalk}
	if [ -s ${TOOLS_DIR}/$@/locale/${PSFFM.tooltalk} ]; \
	then install -m 664 ${TOOLS_DIR}/$@/locale/${PSFFM.tooltalk}  .; \
	fi

unique: msguniq
	for i in ${PSFFM_FILES}; do \
	    if test -s $$i; \
	    then msguniq $$i > $$i.new; \
		 mv $$i.new $$i; \
	    fi; \
	done

include ${RULESET.include}

