/********************************************************************
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * widgetdata.h:  Include file used for all widget related 
 *                declarations
 *
 ********************************************************************/



char *widgetnametext[34][2]      = { 
			{"BulletinBoard", 	"Bb"},
			{"ControlArea",		"Ca"},
			{"Form",		"F"},
			{"RubberTile",		"Rt"},
			{"TextField",		"Tf"},
			{"TextEdit",		"Te"},
			{"PopupWindowShell",	"Pw"},
			{"NoticeShell",		"N"},
			{"MenuShell",		"M"},
			{"ScrolledWindow",	"Sw"},
			{"ScrollingList",	"Sl"}, 	
			{"NonExclusives",	"Ne"},
			{"Exclusives",		"Ex"},
			{"Checkbox",		"Cb"},
			{"Slider",		"Sd"},
			{"MenuButton",		"Mb-G"},
			{"RectButton",		"Rb"},
			{"Gauge",		"Ga"},
			{"FlatNonExclusives",	"Fn"},
			{"FlatExclusives",	"Fe"},
			{"FlatCheckbox",	"Fc"},
			{"DrawArea",		"Da"},
			{"Stub",		"S"},
			{"OblongButton",	"Ob-G"},
			{"Scrollbar",		"Sb"},
			{"Caption",		"Cp"},
			{"StaticText",		"St"},
			{"DropTarget",		"Dt"},
			{"AbbrevMenuButton",	"Ab-G"},
			{"FooterPanel",		"Fp"},
 			{"FontChooser",		"Fnc"},
 			{"FileChooser",		"Fic"},
 			{"TextLine",		"Tl"},
 			{"NumericField",	"Nf"}
			};


char* widget_class_type[] = {
			 "constraint",	/* BulletinBoard 	*/
			 "constraint",	/* ControlArea		*/
			 "constraint",	/* Form			*/
			 "constraint",	/* RubberTile		*/
			 "constraint",	/* TextField	 	*/
			 "primitive",	/* TextEdit		*/
			 "shell",	/* PopupWindowShell	*/
			 "shell",	/* NoticeShell		*/
			 "shell",	/* MenuShell		*/	
			 "constraint",	/* ScrolledWindow	*/
			 "constraint",	/* ScrollingList	*/
			 "constraint",	/* Nonexclusives 	*/
			 "constraint",	/* Exclusives		*/
			 "constraint",	/* CheckBox		*/
			 "primitive",	/* Slider		*/
			 "primitive",	/* MenuButton		*/
			 "primitive",	/* RectButton		*/
			 "primitive",	/* Gauge		*/
			 "flat",	/* FlatNonexclusives	*/
			 "flat",	/* Flatexclusives	*/
			 "flat",	/* FlatCheckBox		*/
			 "constraint",	/* DrawArea		*/
			 "primitive",	/* Stub			*/
			 "primitive",	/* OblongButton		*/
			 "primitive",	/* Scrollbar		*/
			 "constraint",	/* Caption		*/
			 "primitive",	/* StaticText		*/
			 "primitive",	/* DropTarget		*/
			 "primitive",	/* AbbrevMenuButton	*/
			 "constraint",	/* FooterPanel		*/
 			 "constraint",	/* FontChooser		*/
 			 "constraint",	/* FileChooser		*/
 			 "primitive",	/* TextLine		*/
 			 "primitive",	/* NumericField		*/
			 };

