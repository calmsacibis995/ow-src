/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_locale_db.h 1.6	96/07/24 SMI"

#include "big5.def"
#include "en_US_UTF8.def"
#include "iso8859_1.def"
#include "iso8859_2.def"
#include "iso8859_4.def"
#include "iso8859_5.def"
#include "iso8859_7.def"
#include "iso8859_9.def"
#include "ja.def"
#include "ja_PCK.def"
#include "ko.def"
#include "ko_UTF8.def"
#include "ru.def"
#include "zh.def"
#include "zh_TW.def"

static LclLocaleDB	locale_db[] = {
	"C", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"POSIX", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"chinese", locale_db_zh, sizeof(locale_db_zh)/sizeof(char *),
	"cz", locale_db_iso8859_5, sizeof(locale_db_iso8859_5)/sizeof(char *),
	"da", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"de", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"de_AT", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"de_CH", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"el", locale_db_iso8859_7, sizeof(locale_db_iso8859_7)/sizeof(char *),
	"en_AU", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"en_CA", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"en_IE", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"en_NZ", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"en_UK", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"en_US", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"en_US.UTF-8", locale_db_en_US_UTF8, sizeof(locale_db_en_US_UTF8)/sizeof(char *),
	"es", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_AR", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_BO", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_CL", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_CO", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_CR", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_EC", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_GT", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_MX", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_NI", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_PA", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_PE", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_PY", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_SV", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_UY", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"es_VE", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"et", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"fr", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"fr_BE", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"fr_CA", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"fr_CH", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"hu", locale_db_iso8859_2, sizeof(locale_db_iso8859_2)/sizeof(char *),
	"iso_8859_1", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"it", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"ja", locale_db_ja, sizeof(locale_db_ja)/sizeof(char *),
	"ja_JP.PCK", locale_db_ja_PCK, sizeof(locale_db_ja_PCK)/sizeof(char *),
	"ja_JP.SJIS", locale_db_ja_PCK, sizeof(locale_db_ja_PCK)/sizeof(char *),
	"japanese", locale_db_ja, sizeof(locale_db_ja)/sizeof(char *),
	"ko", locale_db_ko, sizeof(locale_db_ko)/sizeof(char *),
	"korean", locale_db_ko, sizeof(locale_db_ko)/sizeof(char *),
	"ko.UTF-8", locale_db_ko_UTF8, sizeof(locale_db_ko_UTF8)/sizeof(char *),
	"lt", locale_db_iso8859_4, sizeof(locale_db_iso8859_4)/sizeof(char *),
	"lv", locale_db_iso8859_4, sizeof(locale_db_iso8859_4)/sizeof(char *),
	"nl", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"nl_BE", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"no", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"pl", locale_db_iso8859_2, sizeof(locale_db_iso8859_2)/sizeof(char *),
	"pt", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"pt_BR", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"ru", locale_db_ru, sizeof(locale_db_ru)/sizeof(char *),
	"su", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"sv", locale_db_iso8859_1, sizeof(locale_db_iso8859_1)/sizeof(char *),
	"tchinese", locale_db_zh_TW, sizeof(locale_db_zh_TW)/sizeof(char *),
	"tr", locale_db_iso8859_9, sizeof(locale_db_iso8859_9)/sizeof(char *),
	"zh", locale_db_zh, sizeof(locale_db_zh)/sizeof(char *),
	"zh_TW", locale_db_zh_TW, sizeof(locale_db_zh_TW)/sizeof(char *),
	"zh_TW.BIG5", locale_db_big5, sizeof(locale_db_big5)/sizeof(char *),
};
