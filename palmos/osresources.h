/* We would prefer to #include <UIResources.h> to get these, but PilRC
   can't parse that file even if it did know where to look for it, so we
   just define the edit menu ids we need from that header ourselves.  */

#define sysEditMenuID		10000
#define sysEditMenuUndoCmd	10000
#define sysEditMenuCutCmd	10001
#define sysEditMenuCopyCmd	10002
#define sysEditMenuPasteCmd	10003
#define sysEditMenuSelectAllCmd	10004
#define sysEditMenuSeparator	10005
#define sysEditMenuKeyboardCmd	10006
#define sysEditMenuGraffitiCmd	10007
