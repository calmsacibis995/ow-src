#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)vkbd_data.c 1.2 91/06/28";
#endif
#endif

#include "vkbd.h"

String languages[NBR_LANGUAGES] = {
    "US English",
    "UK English",
    "French",
    "Canadian",
    "German",
    "Italian",
    "Dutch",
    "Portugese",
    "Spanish",
    "Swedish",
    "Danish",
    "Norwegian",
    "Sw. French",
    "Sw. German"
};

String language_labels[NBR_LANGUAGES] = {
    "US English",
    "UK English",
    "French",
    "Canadian French",
    "German",
    "Italian",
    "Dutch",
    "Portugese",
    "Spanish",
    "Swedish",
    "Danish",
    "Norwegian",
    "Swiss French",
    "Swiss German"
};

String keyboards[NBR_LANGUAGES][NBR_SHIFTS][NBR_ROWS][NBR_COLS] = {

    /**************************************************************************
     * US English  [Language #1] (Default Keyboard)
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "\\ ", 0, "Delete   ",

    /* Row 2 */
    "Esc ", "1 ", "2 ", "3 ", "4 ", "5 ", "6 ", "7 ", "8 ", "9 ", "0 ",
    "- ", "= ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p ",
    "[ ", "] ", " Return  ", 0,

    /* Row 4*/
    "Control", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "; ",
    "' ", "` ", 0, 0,

    /* Row 5 */
    "Shift    ", 0, "z ", "x ", "c ", "v ", "b ", "n ", "m ", ", ", ". ", "/ ",
    "Shift   ", "LnFd", 0,

    /* Row 6 */
    "Caps", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "| ", 0, "Delete   ",

    /* Row 2 */
    "Esc ", "! ", "@ ", "# ", "$ ", "% ", "^ ", "& ", "* ", "( ", ") ", "_ ",
    "+ ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "{ ", "} ", " Return  ", 0,

    /* Row 4*/
    "Control", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", ": ",
    "\" ", "~ ", 0, 0,

    /* Row 5 */
    "Shift    ", 0, "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "< ", "> ", "? ",
    "Shift   ", "LnFd", 0,

    /* Row 6 */
    "Caps", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * UK English  [Language #2]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "\\ ", 0, "Delete   ",

    /* Row 2 */
    "Esc ", "1\246", "2 ", "3#", "4 ", "5 ", "6 ", "7 ", "8 ", "9 ", "0 ",
    "-\254", "= ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p ",
    "[ ", "] ", " Return  ", 0,

    /* Row 4*/
    "Control", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "; ",
    "' ", "` ", 0, 0,

    /* Row 5 */
    "Shift    ", 0, "z ", "x ", "c ", "v ", "b ", "n ", "m ", ", ", ". ", "/ ",
    "Shift   ", "LnFd", 0,

    /* Row 6 */
    "Caps", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "| ", 0, "Delete   ",

    /* Row 2 */
    "Esc ", "! ", "@ ", "\243 ", "$ ", "% ", "^ ", "& ", "* ", "( ", ") ",
    "_ ", "+ ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "{ ", "} ", " Return  ", 0,

    /* Row 4*/
    "Control", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", ": ",
    "\" ", "~ ", 0, 0,

    /* Row 5 */
    "Shift    ", 0, "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "< ", "> ", "? ",
    "Shift   ", "LnFd", 0,

    /* Row 6 */
    "Caps", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * French  [Language #3]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "[ ", "] ", "Del ",

    /* Row 2 */
    "Esc ", "& ", "\351\262", "\"\263", "' ", "( ", "\247^", "\350 ", "!\243",
    "\347\\", "\340 ", ")~", "-#", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "a ", "z ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p ",
    "  ", "`@", " Return  ", 0,

    /* Row 4*/
    "Control", "q ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "m\265",
    "\371 ", "*\244", 0, 0,

    /* Row 5 */
    "Shift ", "< ", "w ", "x ", "c ", "v ", "b ", "n ", ", ", "; ", ": ", "= ",
    "Shift  ", "LnFd", 0,

    /* Row 6 */
    "AltG", "Alt", "<>", "                                    ", "<>",
    "Compose", "Caps", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "{ ", "} ", "Del ",

    /* Row 2 */
    "Esc ", "1 ", "2 ", "3 ", "4 ", "5 ", "6 ", "7 ", "8 ", "9 ", "0 ",
    "\260 ", "_ ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "A ", "Z ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "  ", "$ ", " Return  ", 0,

    /* Row 4*/
    "Control", "Q ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "M ",
    "% ", "| ", 0, 0,

    /* Row 5 */
    "Shift ", "> ", "W ", "X ", "C ", "V ", "B ", "N ", "? ", ". ", "/ ", "+ ",
    "Shift  ", "LnFd", 0,

    /* Row 6 */
    "AltG", "Alt", "<>", "                                    ", "<>",
    "Compose", "Caps", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Canadian  [Language #4]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "#\\", 0, "Delete   ",

    /* Row 2 */
    "Esc ", "1\261", "2@", "3\243", "4\242", "5\244", "6\254", "7|", "8\262",
    "9\263", "0\274", "-\275", "=\276", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o\247", "p\266",
    " [", " ]", " Return  ", 0,

    /* Row 4*/
    "Control", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", ";~",
    " {", "<}", 0, 0,

    /* Row 5 */
    "Shift    ", 0, "z ", "x ", "c ", "v\253", "b\273", "n\260", "m\265", ",_",
    ". ", "\351 ", "Shift  ", "LnFd", 0,

    /* Row 6 */
    "AltG", "Alt", "<>", "                                   ", "<>",
    "Compose", "Caps", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "| ", 0, "Delete   ",

    /* Row 2 */
    "Esc ", "! ", "\" ", "/ ", "$ ", "% ", "? ", "& ", "* ", "( ", ") ", "_ ",
    "+ ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "^ ", "  ", " Return  ", 0,

    /* Row 4*/
    "Control", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", ": ",
    "` ", "> ", 0, 0,

    /* Row 5 */
    "Shift    ", 0, "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "\264 ", ". ",
    "\312 ", "Shift  ", "LnFd", 0,

    /* Row 6 */
    "AltG", "Alt", "<>", "                                   ", "<>",
    "Compose", "Caps", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * German  [Language #5]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "[\253", "]\273", "Del ",

    /* Row 2 */
    "Esc ", "1 ", "2\262", "3\263", "4 ", "5 ", "6 ", "7\260", "8`", "9\264",
    "0|", "\337\\", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "z ", "u ", "i ", "o ", "p ",
    "\374 ", "+~", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "\366 ",
    "\344 ", "#@", 0, 0,

    /* Row 5 */
    "Shift ", "< ", "y ", "x ", "c ", "v ", "b ", "n ", "m\265", ", ", ". ",
    "- ", "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "AltG", "<>", "                                   ", "<>",
    "Compose", "Alt", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "{ ", "} ", "Del ",

    /* Row 2 */
    "Esc ", "! ", "\" ", "\247 ", "$ ", "% ", "& ", "/ ", "( ", ") ", "= ",
    "? ", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Z ", "U ", "I ", "O ", "P ",
    "\334 ", "* ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\326 ",
    "\304 ", "^ ", 0, 0,

    /* Row 5 */
    "Shift ", "> ", "Y ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "_ ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "AltG", "<>", "                                   ", "<>",
    "Compose", "Alt", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Italian  [Language #6]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "[\253", "]\273", "Del ",

    /* Row 2 */
    "Esc ", "1 ", "2\262", "3\263", "4 ", "5 ", "6\254", "7\247", "8 ", "9\\",
    "0|", "\264`", "\354 ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p ",
    "\350 ", "+~", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "\362@",
    "\340#", "\371 ", 0, 0,

    /* Row 5 */
    "Shift ", "< ", "z ", "x ", "c ", "v ", "b ", "n ", "m ", ", ", ". ", "- ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "{ ", "} ", "Del ",

    /* Row 2 */
    "Esc ", "! ", "\" ", "\243 ", "$ ", "% ", "& ", "/ ", "( ", ") ", "= ",
    "? ", "^ ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "\351 ", "* ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\347 ",
    "\260 ", "\247 ", 0, 0,

    /* Row 5 */
    "Shift ", "> ", "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "_ ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Dutch  [Language #7]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "@\254", "\\ ", "Del ",

    /* Row 2 */
    "Esc ", "1\271", "2\262", "3\263", "4\274", "5\275", "6\276", "7\243",
    "8{", "9}", "0`", "/ ", "\260 ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p ",
    "  ", "*~", " Return  ", 0,

    /* Row 4*/
    "Control", "a ", "s\337", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "+ ",
    "  ", "< ", 0, 0,

    /* Row 5 */
    "Shift ", "] ", "z\253", "x\273", "c\242", "v ", "b ", "n ", "m\265", ", ",
    ". ", "- ", "Shift ", "LnFd", 0,

    /* Row 6 */
    "AltG", "Alt", "<>", "                                   ", "<>",
    "Compose", "Caps", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "\247 ", "| ", "Del ",

    /* Row 2 */
    "Esc ", "! ", "\" ", "# ", "$ ", "% ", "& ", "_ ", "( ", ") ", "\264 ",
    "? ", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "^ ", "\246 ", " Return  ", 0,

    /* Row 4*/
    "Control", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\261 ",
    "  ", "> ", 0, 0,

    /* Row 5 */
    "Shift ", "[ ", "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "= ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "AltG", "Alt", "<>", "                                   ", "<>",
    "Compose", "Caps", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Portugese  [Language #8]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "[\253", "]\273", "Del ",

    /* Row 2 */
    "Esc ", "1 ", "2@", "3\243", "4\247", "5 ", "6\254", "7 ", "8 ", "9\\",
    "0|", "\264`", "\241 ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p ",
    " +", " ~", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "\347 ",
    "\272 ", " ^", 0, 0,

    /* Row 5 */
    "Shift ", "< ", "z ", "x ", "c ", "v ", "b ", "n ", "m ", ", ", ". ", "- ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "{ ", "} ", "Del ",

    /* Row 2 */
    "Esc ", "! ", "\" ", "# ", "$ ", "% ", "& ", "/ ", "( ", ") ", "= ", "? ",
    "\277 ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "* ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\307 ",
    "\252 ", "  ", 0, 0,

    /* Row 5 */
    "Shift ", "> ", "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "_ ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Spanish  [Language #9]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "[\253", "]\273", "Del ",

    /* Row 2 */
    "Esc ", "1 ", "2@", "3#", "4 ", "5\260", "6\254", "7 ", "8 ", "9\\",
    "0|", "\264`", "\241 ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o\272", "p ",
    " ^", "+~", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "a\252", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "\361 ",
    "  ", "\347 ", 0, 0,

    /* Row 5 */
    "Shift ", "< ", "z ", "x ", "c ", "v ", "b ", "n ", "m ", ", ", ". ", "- ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "{ ", "} ", "Del ",

    /* Row 2 */
    "Esc ", "! ", "\" ", "\267 ", "$ ", "% ", "& ", "/ ", "( ", ") ", "= ",
    "? ", "\277 ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "  ", "* ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\321 ",
    "  ", "\307 ", 0, 0,

    /* Row 5 */
    "Shift ", "> ", "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "_ ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Swedish  [Language #10]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "\247 ", "~ ", "Del ",

    /* Row 2 */
    "Esc ", "1 ", "2@", "3\243", "4$", "5 ", "6 ", "7{", "8[", "9]", "0}",
    "+\\", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p ",
    "\345 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "\366 ",
    "\344 ", "\264`", 0, 0,

    /* Row 5 */
    "Shift ", "<|", "z ", "x ", "c ", "v ", "b ", "n ", "m ", ", ", ". ", "- ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "AltG", "Compose", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "\275 ", "^ ", "Del ",

    /* Row 2 */
    "Esc ", "! ", "\" ", "# ", "\244 ", "% ", "& ", "/ ", "( ", ") ", "= ",
    "? ", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "\305 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\326 ",
    "\304 ", "* ", 0, 0,

    /* Row 5 */
    "Shift ", "> ", "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "_ ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "AltG", "Compose", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Danish  [Language #11]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "\275 ", "~ ", "Del ",

    /* Row 2 */
    "Esc ", "1 ", "2@", "3\243", "4$", "5 ", "6 ", "7{", "8[", "9]", "0}",
    "+ ", " |", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p ",
    "\345 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "\346 ",
    "\370 ", "\264`", 0, 0,

    /* Row 5 */
    "Shift ", "<\\", "z ", "x ", "c ", "v ", "b ", "n ", "m ", ", ", ". ", "- ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "AltG", "Compose", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "\247 ", "^ ", "Del ",

    /* Row 2 */
    "Esc ", "! ", "\" ", "# ", "\244 ", "% ", "& ", "/ ", "( ", ") ", "= ",
    "? ", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "\305 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\306 ",
    "\330 ", "* ", 0, 0,

    /* Row 5 */
    "Shift ", "> ", "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "_ ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "AltG", "Compose", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Norwegian  [Language #12]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "| ", "~ ", "Del ",

    /* Row 2 */
    "Esc ", "1 ", "2@", "3\243", "4$", "5 ", "6 ", "7{", "8[", "9]", "0}",
    "+ ", "\\ ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p ",
    "\345 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "\370 ",
    "\346 ", "\264`", 0, 0,

    /* Row 5 */
    "Shift ", "< ", "z ", "x ", "c ", "v ", "b ", "n ", "m ", ", ", ". ", "- ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "\247 ", "^ ", "Del ",

    /* Row 2 */
    "Esc ", "! ", "\" ", "# ", "\244 ", "% ", "& ", "/ ", "( ", ") ", "= ",
    "? ", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P ",
    "\305 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\330 ",
    "\306 ", "* ", 0, 0,

    /* Row 5 */
    "Shift ", "> ", "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "_ ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "Compose", "AltG", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Swiss French  [Language #13]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "< ", "> ", "Del ",

    /* Row 2 */
    "Esc ", "1!", "2@", "3#", "4\242", "5~", "6\247", "7|", "8\260", "9\\",
    "0^", "\264`", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "z ", "u ", "i ", "o ", "p ",
    "\350 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "\351 ",
    "\340 ", "$\243", 0, 0,

    /* Row 5 */
    "Shift ", "]\\", "y ", "x ", "c ", "v ", "b ", "n ", "m\265", ", ", ". ",
    "- ", "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "AltG", "Compose", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "{ ", "} ", "Del ",

    /* Row 2 */
    "Esc ", "+ ", "\" ", "* ", "\347 ", "% ", "& ", "/ ", "( ", ") ", "= ",
    "? ", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Z ", "U ", "I ", "O ", "P ",
    "\374 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\366 ",
    "\344 ", "  ", 0, 0,

    /* Row 5 */
    "Shift ", "[ ", "Y ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "_ ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "AltG", "Compose", 0, 0, 0, 0, 0, 0, 0, 0,


    /**************************************************************************
     * Swiss German  [Language #14]
     **************************************************************************/
    /******  Unshifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "< ", "> ", "Del ",

    /* Row 2 */
    "Esc ", "1!", "2@", "3#", "4\242", "5~", "6\247", "7|", "8\260", "9\\",
    "0^", "\264`", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "q ", "w ", "e ", "r ", "t ", "z ", "u ", "i ", "o ", "p ",
    "\374 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", "\366 ",
    "\344 ", "$\243", 0, 0,

    /* Row 5 */
    "Shift ", "]\\", "y ", "x ", "c ", "v ", "b ", "n ", "m\265", ", ", ". ",
    "- ", "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "AltG", "Compose", 0, 0, 0, 0, 0, 0, 0, 0,

    /******  Shifted ******/
    /* Row 1 */
    "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ", "   ", "   ", "   ",
    "{ ", "} ", "Del ",

    /* Row 2 */
    "Esc ", "+ ", "\" ", "* ", "\347 ", "% ", "& ", "/ ", "( ", ") ", "= ",
    "? ", "  ", "Back Space", 0,

    /* Row 3*/
    "Tab  ", "Q ", "W ", "E ", "R ", "T ", "Z ", "U ", "I ", "O ", "P ",
    "\350 ", "  ", " Return  ", 0,

    /* Row 4*/
    "Caps   ", "A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", "\351 ",
    "\340 ", "  ", 0, 0,

    /* Row 5 */
    "Shift ", "[ ", "Y ", "X ", "C ", "V ", "B ", "N ", "M ", "; ", ": ", "_ ",
    "Shift ", "LnFd", 0,

    /* Row 6 */
    "Ctrl", "Alt", "<>", "                                   ", "<>",
    "AltG", "Compose", 0, 0, 0, 0, 0, 0, 0, 0
};


#ifdef KEYPADS
String left_keypad[NBR_ROWS][2] = {
    /* Row 1 */
    "Stop ", "Again",

    /* Row 2 */
    "Props", "Undo ",

    /* Row 3 */
    "Front", "Copy ",

    /* Row 4 */
    "Open ", "Paste",

    /* Row 5 */
    "Find ", "Cut  ",

    /* Row 6 */
    "Help        ", 0
};

String right_keypad[NBR_ROWS][4] = {
    /* Row 1 */
    "P", "P", "S", "N",

    /* Row 2 */
    "=", "/", "*", "-",

    /* Row 3*/
    "7", "8", "9", "+",

    /* Row 4 */
    "4", "5", "6", 0,

    /* Row 5 */
    "1", "2", "3", "E",

    /* Row 6 */
    "0  ", ".", 0, 0
};
#endif /* KEYPADS */


String property_name[NBR_ATOMS] = {
    "_OL_ENTER_LANG_MODE",
    "_OL_SHOW_SFK_WIN",
    "_OL_EXIT_LANG_MODE",
    "_OL_SOFTKEY_LABELS",
    "_OL_TRANSLATE_KEY",
    "_OL_TRANSLATED_KEY"
};
