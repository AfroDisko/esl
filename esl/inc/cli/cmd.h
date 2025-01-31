#ifndef CMD_H
#define CMD_H

static const char gCmdResponseUnknownCmd[] = "Unknown command (enter 'help' for help)\r\n";

static const char gCmdHelp[]               = "help";

static const char gCmdResponseHelp[]       = "Available commands:\r\n"
                                             "help                             -- prints this message\r\n"
                                             "rgb <r> <g> <b>                  -- sets LED2 state according to RGB input (0 <= <i> <= 255)\r\n"
                                             "hsv <h> <s> <v>                  -- sets LED2 state according to HSV input (0 <= <i> <= 255)\r\n"
                                             "color_add_rgb <r> <g> <b> <name> -- memorizes LED2 state according to RGB input (0 <= <i> <= 255)\r\n"
                                             "color_add_cur <name>             -- memorizes current LED2 state\r\n"
                                             "color_set <name>                 -- sets LED2 state according to prev. memorized state named <name>,\r\n"
                                             "color_del <name>                 -- deletes LED2 state named <name>\r\n";

static const char gCmdRgb[]                = "rgb";

static const char gCmdHsv[]                = "hsv";

static const char gCmdColorAddRgb[]        = "color_add_rgb";

static const char gCmdResponseNoSpace[]    = "There is no space left to save that record! Delete something first\r\n";

static const char gCmdColorAddCur[]        = "color_add_cur";

static const char gCmdColorSet[]           = "color_set";

static const char gCmdResponseNoColor[]    = "There is no color named like that!\r\n";

static const char gCmdColorDel[]           = "color_del";

#endif
