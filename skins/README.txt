If you are using Windows, put the skins in the Free42 directory. By default,
this is the directory containing the Free42 executable, but its location can be
changed in the preferences dialog. Note: If you set the Free42 directory to a
location other than the executable's directory, you may put skins in both
locations.
If you are using Unix, put the skins in $HOME/.free42/.

-------------------------------------------------------------------------------

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
calculator keys for FIX is Shift (28), E (16), Sigma+ (1), so...

Key: 38 <sens_rect> <disp_rect> <active_pt>
Macro: 38 28 16 1
