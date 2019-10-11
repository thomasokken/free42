Free42 skin description (*.layout) file format:
Anything from a '#' until the end of the line is a comment
Non-comment lines contain the following information:

(Note: the skin bitmap is assumed to have the same filename as the skin
description, with the 'layout' extension replaced by 'gif'.)
(Note: rectangles are given as "x,y,width,height"; points are "x,y".)

Skin: the portion of the skin bitmap to be rendered as the actual faceplate
Display: describes the location, size, and color of the display; arguments
  are: top-left corner, x magnification, y magnification, background color,
  foreground color. Colors are specified as 6-digit hex numbers in RRGGBB
  format.
Key: describes a clickable key; arguments are: keycode, sensitive rectangle
  (i.e. the rectangle where mouse-down events will cause the key to be
  pressed), display rectangle (i.e. the rectangle that changes when a key is
  pressed or released), and the location of the top-left corner of the active-
  state bitmap (since the active-state bitmap must have the same size as the
  display rectangle, only its position, not its width and height, are
  specified).
  Keycodes in the range 1..37 correspond to actual calculator keys; keycodes
  38..255 can be used to define "macro" keys. For each such keycode, there must
  be a corresponding "Macro:" line in the layout file.
  You may specify two keycodes (two numbers separated by a comma); if you do,
  the first is used when the calculator's shift (indicated by the shift
  annunciator) is inactive, and the second is used when the calculator's shift
  is active. This feature allows you to have a key's shifted function be
  something different than it is on the original HP-42S keyboard.
Macro: for keys with keycodes in the range 38..255, this defines the sequence
  of HP-42S keys (keycodes 1..37) that is to be pressed; arguments are:
  keycode, followed by zero or more keycodes in the range 1..37. See below for
  an example.
Annunciator: describes an HP-42S annunciator; arguments are: code (1=updown,
  2=shift, 3=print, 4=run, 5=battery, 6=g, 7=rad), display rectangle, and the
  location of the top-left corner of the active-state bitmap.

For examples, look at the *.layout and *.gif files in this directory.

Macro example:
To define a key for the FIX command, using key code 38: the sequence of
calculator keys for FIX is Shift (28), E (16), Σ+ (1), so...

Key: 38 <sens_rect> <disp_rect> <active_pt>
Macro: 38 28 16 1

You can also define PC keyboard mappings in the *.layout file. The syntax is
identical to that of the keymap file, preceded by a tag that indicates the
target platform: WinKey for Windows, MacKey for Mac, and GtkKey for Linux and
other Unix-like environments. It is necessary to specify which platform each
key mapping is for, since the key codes are platform-dependent.
If a layout file defines a mapping for a key that is also mapped in the keymap
file, the skin-specific mapping takes precedence.
Note that, while Macro definitions may only contain codes 1..37, a keyboard
mapping may contain codes 38..255 as well, so you could theoretically map a PC
keyboard key to a sequence of macros. This is not recommended, however; for
clarity, it is probably better for key mappings to consist only of one key or
macro number, preceded by Shift (28) if necessary. This will also allow Free42
to match the PC keyboard key to a skin-defined key, which will be highlighted
for visual feedback when the mapping is activated.
