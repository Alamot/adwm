/* See LICENSE file for copyright and license details. */

/* tags and title appearance */
static const char *fonts[] = {
  "VL Gothic:size=12",
  "Sans:size=12",
  "WenQuanYi Micro Hei:size=12"
};
static const char statusfont[]      = "-*-fixed-*-*-*-*-20-*-*-*-*-*-*-*";
static const char normbordercolor[] = "#444444";
static const char normbgcolor[]     = "#222222";
static const char normfgcolor[]     = "#bbbbbb";
static const char selbordercolor[]  = "#999900";
static const char selbgcolor[]      = "#005577";
static const char selfgcolor[]      = "#eeeeee";
static unsigned int baralpha        = 0x99U;
static unsigned int borderalpha     = OPAQUE;
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const Bool showbar           = True;     /* False means no bar */
static const Bool topbar            = False;     /* False means bottom bar */
static const Bool viewontag         = True;     /* Switch view on tag switch */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const Bool systraypinningfailfirst = True;   /* True: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const Bool showsystray       = True;     /* False means no systray */

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class                  instance    title       tags mask     isfloating   monitor */
	{ "google-chrome",        NULL,       NULL,         1,          False,        -1 },
	{ "Spacefm",              NULL,       NULL,         1 << 1,     False,        -1 },
	{ "Geany",                NULL,       NULL,         1 << 2,     False,        -1 },
	{ "Audacious",            NULL,       NULL,         1 << 8,     True,         -1 },
	{ "Stjerm",               NULL,       NULL,         0,          True,         -1 },
	{ "Shutdown-manager-transparent",     	  NULL,       NULL,         0,          True,         -1 },
};

/* layout(s) */
static const float mfact      = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster      = 1;    /* number of clients in master area */
static const Bool resizehints = True; /* True means respect size hints in tiled resizals */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ KeyPress, 0,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ KeyPress, MODKEY,                  KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ KeyPress, 0|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ KeyPress, Mod1Mask,                KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *hicmd[]  = { "hardinfo", NULL, NULL, NULL, "Hardinfo"};
static const char *taskcmd[]  = { "gnome-system-monitor", NULL, NULL, NULL, "Gnome-system-monitor"};
static const char *concmd[]  = { "sudo", "connmanctl", "scan", "wifi", NULL};
static const char *cmstcmd[]  = { "sudo", "cmst", NULL, NULL, "CMST - Connman System Tray"};
static const char *calcmd[]  = { "google-chrome-stable", "https://calendar.google.com/calendar/render#main_7%7Cmonth", NULL, NULL, "google-chrome"};
static const char *dmenucmd[]   = { "dmenu_run", "-b", "-fn", "Sans:size=12", "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
static const char *termcmd[]  = { "lilyterm", NULL };
static const char *htopcmd[]  = { "lilyterm", "-e", "htop", NULL};
static const char *glancescmd[]  = { "lilyterm", "-e", "glances", NULL};
static const char *editcmd[]  = { "geany",  NULL, NULL, NULL, "Geany"};
static const char *wwwcmd[]  = { "google-chrome-stable", NULL, NULL, NULL, "google-chrome"};
static const char *spacefmcmd[] = { "spacefm",  NULL, NULL, NULL, "Spacefm"};
static const char *musiccmd[] = { "audacious",  NULL, NULL, NULL, "Audacious"};
static const char *systemcmd[] = { "shutdown-manager-transparent",  NULL, NULL, NULL, "Shutdown-manager-transparent"};
static const char *scrotcmd[]  = { "scrot", "-q 90", "/home/alamot/Pictures/Screenshots/%d%b%Y_%H:%M:%S_$wx$h.jpg", NULL };
static const char *scrotscmd[]  = { "scrot", "-q 90", "-s", "/home/alamot/Pictures/Screenshots/%d%b%Y_%H:%M:%S_$wx$h.jpg", NULL };
static const char *scrotspngcmd[]  = { "scrot", "-q 75", "-s", "/home/alamot/Pictures/Screenshots/%d%b%Y_%H:%M:%S_$wx$h.png", NULL };
static const char *habakcmd[]  = { "habak", "-hi", "/home/alamot/Pictures/Wallpapers", NULL };

static Key keys[] = {
	/*Keytype     modifier          key              function        argument */
	{ KeyPress,   MODKEY,           XK_a,            spawn,          {.v = dmenucmd } },
	{ KeyPress,   MODKEY,           XK_grave,        spawn,          {.v = termcmd  } },
	{ KeyPress,   0,                XF86XK_Mail,     spawnorraise,   {.v = wwwcmd   } },
        { KeyPress,   0,                XF86XK_Search,   spawnorraise,   {.v = editcmd  } },
	{ KeyPress,   0,                XF86XK_HomePage, spawnorraise,   {.v = spacefmcmd } },
	{ KeyPress,   0,                XK_Menu,         spawn,          {.v = dmenucmd } },
	{ KeyRelease, 0,                XK_Print,        spawn,          {.v = scrotcmd } },
	{ KeyRelease, 0|ControlMask,    XK_Print,        spawn,          {.v = scrotscmd} },
	{ KeyRelease, 0|ShiftMask,      XK_Print,        spawn,          {.v = scrotspngcmd} },
	{ KeyPress,   MODKEY,           XK_w,            spawn,          {.v = habakcmd } },
	{ KeyPress,   MODKEY,           XK_space,        togglebar,      {0} },
	{ KeyPress,   MODKEY,           XK_bracketleft,  focusstack,     {.i = +1 } },
	{ KeyPress,   MODKEY,           XK_bracketright, focusstack,     {.i = -1 } },
	{ KeyPress,   MODKEY,           XK_Up,           incnmaster,     {.i = +1 } },
	{ KeyPress,   MODKEY,           XK_Down,         incnmaster,     {.i = -1 } },
	{ KeyPress,   MODKEY,           XK_Left,         setmfact,       {.f = -0.05} },
	{ KeyPress,   MODKEY,           XK_Right,        setmfact,       {.f = +0.05} },
	{ KeyPress,   MODKEY,           XK_z,            zoom,           {0} },
	{ KeyPress,   Mod1Mask,         XK_Tab,          view,           {0} },
	{ KeyPress,   MODKEY,           XK_c,            killclient,     {0} },
	{ KeyPress,   MODKEY,           XK_l,            setlayout,      {0} },
	{ KeyPress,   MODKEY,           XK_f,            togglefloating, {0} },
        { KeyPress,   MODKEY,           XK_minus,        alteropacity,   {.i = -1     } },
	{ KeyPress,   MODKEY,           XK_equal,        alteropacity,   {.i = +1     } },
        { KeyPress,   MODKEY,           XK_comma,        focusmon,       {.i = -1     } },
	{ KeyPress,   MODKEY,           XK_period,       focusmon,       {.i = +1     } },
	{ KeyPress,   MODKEY|ShiftMask, XK_comma,        tagmon,         {.i = -1     } },
	{ KeyPress,   MODKEY|ShiftMask, XK_period,       tagmon,         {.i = +1     } },
	{ KeyPress,   MODKEY,           XK_F1,           spawnorraise,   {.v = wwwcmd } },
	{ KeyPress,   MODKEY,           XK_F2,           spawnorraise,   {.v = spacefmcmd } },
	{ KeyPress,   MODKEY,           XK_F3,           spawnorraise,   {.v = editcmd } },
	{ KeyPress,   MODKEY,           XK_F4,           spawnorraise,   {.v = cmstcmd } },
	{ KeyPress,   MODKEY,           XK_F5,           spawn,          {.v = concmd  } },
	{ KeyPress,   MODKEY,           XK_F9,           spawnorraise,   {.v = musiccmd } },
	{ KeyPress,   MODKEY,           XK_F10,          spawn,          {.v = htopcmd } },
	{ KeyPress,   MODKEY,           XK_Escape,       spawnorraise,   {.v = systemcmd  } },
	{ KeyPress,   MODKEY,           XK_r,            quit,           {.ui = 0 } },
        { KeyPress,   MODKEY|ControlMask,XK_Escape,      quit,           {.ui = 1 } },
	{ KeyPress,   MODKEY,           XK_KP_Insert,    view,           {.ui = ~0 } },
	{ KeyPress,   0|ShiftMask,      XK_KP_Insert,    tag,            {.ui = ~0    } },
   /* TAGKEYS(                          XK_F1,                      0)
	TAGKEYS(                        XK_F2,                      1)
	TAGKEYS(                        XK_F3,                      2)
	TAGKEYS(                        XK_F4,                      3)
	TAGKEYS(                        XK_F5,                      4)
	TAGKEYS(                        XK_F6,                      5)
	TAGKEYS(                        XK_F7,                      6)
	TAGKEYS(                        XK_F8,                      7)
	TAGKEYS(                        XK_F9,                      8)*/
    TAGKEYS(                            XK_KP_End,                    0)
	TAGKEYS(                        XK_KP_Down,                   1)
	TAGKEYS(                        XK_KP_Page_Down,              2)
	TAGKEYS(                        XK_KP_Left,                   3)
	TAGKEYS(                        XK_KP_Begin,                  4)
	TAGKEYS(                        XK_KP_Right,                  5)
	TAGKEYS(                        XK_KP_Home,                   6)
	TAGKEYS(                        XK_KP_Up,                     7)
	TAGKEYS(                        XK_KP_Page_Up,                8)
};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            MODKEY,         Button1,        toggleview,     {0} },
	{ ClkTagBar,            0,              Button3,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
        { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {0} },
	{ ClkWinTitle,          0,              Button1,        zoom,           {0} },
        { ClkWinTitle,          0,              Button3,        killclient,     {0} },
        { ClkWinTitle,          0,              Button2,        spawn, {.v = htopcmd  } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
        { ClkStatusTextLeft,    0,              Button1,        switchxkblayout,{.i = +1} },
        { ClkStatusTextLeft,    0,              Button3,        switchxkblayout,{.i = -1} },
        { ClkStatusTextLeft,    0,              Button2,        spawn,        {.v = calcmd  } },
        { ClkStatusTextMiddle,  0,              Button1,        spawnorraise, {.v = taskcmd } },
        { ClkStatusTextMiddle,  0,              Button2,        spawn,        {.v = glancescmd } },
        { ClkStatusTextMiddle,  0,              Button3,        spawnorraise, {.v = hicmd   } },
        { ClkStatusTextRight,   0,              Button1,        spawnorraise, {.v = cmstcmd } },
        { ClkStatusTextRight,   0,              Button2,        spawn,        {.v = concmd  } },
        { ClkStatusTextRight,   0,              Button3,        spawnorraise, {.v = systemcmd  } },
        
        
	
};

