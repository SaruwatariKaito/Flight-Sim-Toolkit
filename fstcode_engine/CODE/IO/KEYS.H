/* Key codes for PC */
/* Value 0 is no key */
/* Values 0 - 127 are standard codes */
/* add SHIFT_OFFSET or CTRL_OFFSET or ALT_OFFSET if needed */

#define LEFT_SHIFT_KEY 42
#define RIGHT_SHIFT_KEY 54
#define CTRL_KEY 29
#define ALT_KEY 56

#define SHIFT(X) (X + SHIFT_OFFSET)
#define CTRL(X) (X + CTRL_OFFSET)
#define ALT(X) (X + ALT_OFFSET)

#define KEY_A 30
#define KEY_B 48
#define KEY_C 46
#define KEY_D 32
#define KEY_E 18
#define KEY_F 33
#define KEY_G 34
#define KEY_H 35
#define KEY_I 23
#define KEY_J 36
#define KEY_K 37
#define KEY_L 38
#define KEY_M 50
#define KEY_N 49
#define KEY_O 24
#define KEY_P 25
#define KEY_Q 16
#define KEY_R 19
#define KEY_S 31
#define KEY_T 20
#define KEY_U 22
#define KEY_V 47
#define KEY_W 17
#define KEY_X 45
#define KEY_Y 21
#define KEY_Z 44

#define KEY_1 2
#define KEY_2 3
#define KEY_3 4
#define KEY_4 5
#define KEY_5 6
#define KEY_6 7
#define KEY_7 8
#define KEY_8 9
#define KEY_9 10
#define KEY_0 11

#define ESCAPE 1
#define SPACE 57
#define BACKSLASH 43
#define MINUS 12
#define EQUALS 13
#define UNDERSCORE (MINUS + SHIFT_OFFSET)
#define PLUS (EQUALS + SHIFT_OFFSET)
#define LEFTSQUARE 26
#define RIGHTSQUARE 27
#define SEMICOLON 39
#define QUOTE 40
#define COMMA 51
#define DOT 52
#define SLASH 53
#define DELETE 83
#define AMPERSAND (40 + SHIFT_OFFSET)
#define PAGE_UP 73
#define PAGE_DOWN 81

#define LEFTARROW 75
#define RIGHTARROW 77
#define UPARROW 72
#define DOWNARROW 80
#define BACKSPACE 14
#define ENTER 28

#define CTRL_A (KEY_A + CTRL_OFFSET)
#define CTRL_B (KEY_B + CTRL_OFFSET)
#define CTRL_D (KEY_D + CTRL_OFFSET)
#define CTRL_E (KEY_E + CTRL_OFFSET)
#define CTRL_I (KEY_I + CTRL_OFFSET)
#define CTRL_J (KEY_J + CTRL_OFFSET)
#define CTRL_K (KEY_K + CTRL_OFFSET)
#define CTRL_L (KEY_L + CTRL_OFFSET)
#define CTRL_M (KEY_M + CTRL_OFFSET)
#define CTRL_S (KEY_S + CTRL_OFFSET)
#define CTRL_V (KEY_V + CTRL_OFFSET)
#define CTRL_Z (KEY_Z + CTRL_OFFSET)

#define PAD_0 82
#define PAD_1 79
#define PAD_2 80
#define PAD_3 81
#define PAD_4 75
#define PAD_5 76
#define PAD_6 77
#define PAD_7 71
#define PAD_8 72
#define PAD_9 73
#define PAD_DOT 83
#define PAD_PLUS 78
#define PAD_MINUS 74
#define PAD_TIMES 55

#define F1 59
#define F2 60
#define F3 61
#define F4 62
#define F5 63
#define F6 64
#define F7 65
#define F8 66
#define F9 67
#define F10 68

#define ALT_0 (KEY_0 + ALT_OFFSET)
#define ALT_1 (KEY_1 + ALT_OFFSET)
#define ALT_2 (KEY_2 + ALT_OFFSET)
#define ALT_3 (KEY_3 + ALT_OFFSET)
#define ALT_4 (KEY_4 + ALT_OFFSET)
#define ALT_5 (KEY_5 + ALT_OFFSET)

