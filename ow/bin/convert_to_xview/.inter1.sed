#!/bin/sh
#
cat ${1} | \
    sed -e 's%\<setlocale\>%\
#ifdef I18NXVVIEW_COMMENT\
     I18NXView CONVERSION - Application should not set the locale\.\
     Locale setting should be done through OpenWindows'\'' property sheet, or\
     new I18N XView API. Please refer to programmers guide for detail \
     information\. Application should only query the locale\.\
#endif\
setlocale%g' \
        -e 's%\<MSG_SWITCH%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION -  gettext() and related routines are \
     now avaliable for message handling. Please refer to programmers \
     guide for detail information\.\
#endif\
MSG_SWITCH%g' \
        -e 's%\<gettxt\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION -  gettext() and related routines are \
     now avaliable for message handling. Please refer to programmers \
     guide for detail information\.\
#endif\
gettxt%g' \
        -e 's%\<WIN_USE_CM\>%WIN_USE_IM%g'\
	-e 's%^\(.*\)WIN_DO_CONVERT\(.*\)ON\(.*\)$%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - WIN_DO_CONVERT, ON is now changed to \
     WIN_IC_CONVERSION, TRUE\
\1WIN_DO_CONVERT\2ON\3\
#endif\
%g' \
	-e 's%^\(.*\)WIN_DO_CONVERT\(.*\)OFF\(.*\)$%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - WIN_DO_CONVERT, OFF is now changed to \
     WIN_IC_CONVERSION, TRUE\
\1WIN_DO_CONVERT\2OFF\3\
#endif\
%g' \
        -e 's%^\(.*\)canvas_create_mle\(.*\)$%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - no need to call canvas_create_mle() anymore\.\
     This will be done automatically for Asian XView\.\
\1canvas_create_mle\2\
#endif\
%g' \
	-e 's%\<pf_open\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Use xv_find instead\. If you are trying\
     to open a evfont, now you need to open a fontset\. Please refer to \
     programmers guide for detail information \.\
#endif\
pf_open%g' \
        -e 's%\<pf_close\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. If you want to destroy the font object,\
     use xv_destroy instead\.\
#endif\
pf_close%g' \
        -e 's%\<pf_default%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. If function pf_default use xv_find instead\. \
     If struct value ignore\.\
#endif\
pf_default%g' \
        -e 's%\<pf_text\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Use xv_pf_text instead, remember to extern it\.\
     Note that xv_pf_text is only for ascii font\. Use of this funtion is\
     Strongly discouraged in Internationalized XView\.\
     If you are using non-ascii fontset, you need to convert your data to\
     wide char format and use XwcDrawString to display your text\.\
#endif\
pf_text%g' \
        -e 's%\<pf_textbatch%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Use xv_pf_textbatch instead, remember to extern it\.\
     Note that xv_pf_textbatch is only for ascii font\.Use of this funtion is\
     strongly discouraged in Internationalized XView\.\
#endif\
pf_textbatch%g'\
        -e 's%\<pf_textbound%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Use xv_pf_textbound instead, remember to extern it\.\
     Note that xv_pf_textbound is only for ascii font\.Use of this funtion is\
     strongly discouraged in Internationalized XView\.\
#endif\
pf_textbound%g' \
        -e 's%\<pf_ttext\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Use xv_pf_ttext instead, remember to extern it\.\
     Note that xv_pf_ttext is only for ascii font\.Use of this funtion is\
     strongly discouraged in Internationalized XView\.\
#endif\
pf_ttext%g' \
	-e 's%^\(.*\)evfont\.h\(.*\)$%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Use font set instead of evfont\.\
\1evfont\.h\2\
#endif\
%g' \
	-e 's%^\(.*\)EVFONT\(.*\)$%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Use font set instead of evfont\.\
\1EVFONT\2\
#endif\
%g' \
        -e 's%\<IE_SS2\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\.\
#endif\
IE_SS2%g' \
        -e 's%\<IE_SS3\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\. \
#endif\
IE_SS3%g' \
	-e 's%\<event_is_euc\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Used event_is_string instead\. Please refer\
     to Internationalized XView Programmers guide for detail information\.\
#endif\
event_is_euc%g' \
        -e 's%\<event_is_ascii\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\. \
#endif\
event_is_ascii%g' \
        -e 's%\<eventcode_is_ascii\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\. \
#endif\
eventcode_is_ascii%g' \
        -e 's%\<event_to_wchar\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\. \
#endif\
event_to_wchar%g' \
        -e 's%\<event_to_euc\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\. \
#endif\
event_to_euc%g' \
        -e 's%\<eventcode_lead_byte\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\. \
#endif\
eventcode_lead_byte%g' \
        -e 's%\<eventcode_upper_byte\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\. \
#endif\
eventcode_lead_byte%g' \
        -e 's%\<eventcode_lower_byte\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\. \
#endif\
eventcode_lead_byte%g' \
        -e 's%\<EUC_EVENTCODE_MASK\>%\
#ifdef I18NXVIEW_COMMENT\
     I18NXView CONVERSION - Defunct\. Committed text now comes in as ie_string \
     instead of ie_code\. \
#endif\
EUC_EVENTCODE_MASK%g' 
