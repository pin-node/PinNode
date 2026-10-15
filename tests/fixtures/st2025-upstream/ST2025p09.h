/**************************************************************************
  This file is part of Bally Star Trek 2025 - Mega

*/

// These are example values - fill these in with your machine's definitions
#define NUM_OF_LAMPS                      77 // 60 before aux

// Lamp Board (sort by schematics)
#define LA_BONUS_1K                  0 //  1K BONUS
#define LA_BONUS_2K                  1 //  2K BONUS
#define LA_BONUS_3K                  2 //  3K BONUS
#define LA_BONUS_4K                  3 //  4K BONUS
#define LA_BONUS_5K                  4 //  5K BONUS
#define LA_BONUS_6K                  5 //  6K BONUS
#define LA_BONUS_7K                  6 //  7K BONUS
#define LA_BONUS_8K                  7 //  8K BONUS
#define LA_BONUS_9K                  8 //  9K BONUS
#define LA_BONUS_10K                 9 // 10K BONUS
#define LA_BONUS_20K                10 // 20K BONUS
#define LA_UNUSED_11                11 // UNUSED
#define LA_UNUSED_12                12 // UNUSED
#define LA_SPEC_3X                  13 // PLANET SPECIAL PLUS 3X
#define LA_3X_2X                    14 // PLANET 3X PLUS 2X
#define LA_2X                       15 // 2X
#define LA_HYPER_SPACE_2K           16 // HYPER SPACE 2K BONUS
#define LA_HYPER_SPACE_4K           17 // HYPER SPACE 4K BONUS
#define LA_HYPER_SPACE_6K           18 // HYPER SPACE 6K BONUS
#define LA_HYPER_SPACE_8K           19 // HYPER SPACE 8K BONUS
#define LA_HYPER_SPACE_10K          20 // HYPER SPACE 10K BONUS
#define LA_HYPER_SPACE_SPEC         21 // HYPER SPACE SPECIAL
#define LA_PLANET_INLANE            22 // PLANETARY BYPASS INLANE
#define LA_PLANET_OUTLANE           23 // PLANETARY BYPASS OUTLANE
#define LA_SAUCER_B                 24 // SAUCER B
#define LA_SAUCER_A                 25 // SAUCER A
#define LA_SAUCER_L1                26 // SAUCER L #1
#define LA_SAUCER_L2                27 // SAUCER L #2
#define LA_SAUCER_Y                 28 // SAUCER Y
#define LA_300_OR_3000_TARGET       29 // 300 / 3000 TARGET
#define LA_RIGHT_SLINGSHOT_INLANE   30 // SPOCK INLANE
#define LA_LEFT_SLINGSHOT_INLANE    31 // ILIA INLANE
#define LA_B_LANE                   32 // B LANE
#define LA_A_LANE                   33 // A LANE
#define LA_KLINGON_L1               34 // KLINGON L #1
#define LA_KLINGON_L2               35 // KLINGON L #2
#define LA_KLINGON_Y                36 // KLINGON Y
#define LA_RIGHT_SLINGSHOT_OUTLANE  37 // SPOCK OUTLANE
#define LA_LEFT_SLINGSHOT_OUTLANE   38 // ILIA OUTLANE
#define LA_MIDDLE_Y                 39 // MIDDLE Y
#define LA_SAME_PLAYER_SHOOTS_AGAIN 40 // SAME PLAYER SHOOTS AGAIN, BACKBOX
#define LA_MATCH                    41 // MATCH
#define LA_SHOOT_AGAIN              42 // SAME PLAYER SHOOTS AGAIN, PLAYFIELD
#define LA_CREDIT_INDICATOR         43 // CREDIT INDICATOR, APRON
#define LA_MIDDLE_10K               44 // MIDDLE 10K
#define LA_MIDDLE_25K               45 // MIDDLE 25K
#define LA_MIDDLE_50K               46 // MIDDLE 50K
#define LA_MIDDLE_SPECIAL           47 // MIDDLE SPECIAL
#define LA_BALL_IN_PLAY             48 // BALL IN PLAY
#define LA_HIGH_SCORE_TO_DATE       49 // HIGH SCORE
#define LA_GAME_OVER                50 // GAME OVER
#define LA_TILT                     51 // TILT
#define LA_MIDDLE_B                 52 // MIDDLE B
#define LA_MIDDLE_A                 53 // MIDDLE A
#define LA_MIDDLE_L1                54 // MIDDLE L #1
#define LA_MIDDLE_L2                55 // MIDDLE L #2
#define LA_1_UP                     56 // PLAYER 1
#define LA_2_UP                     57 // PLAYER 2
#define LA_3_UP                     58 // PLAYER 3
#define LA_4_UP                     59 // PLAYER 4

#define NUM_OF_SWITCHES             39 // 39 max, with several not used

#define SW_DROP_TARGET_D_LOW             0 // LOWEST DROP TARGET
#define SW_DROP_TARGET_C_MID_LOW         1 // LOWER MIDDLE DROP TARGET
#define SW_DROP_TARGET_B_MID_HIGH        2 // UPPER MIDDLE DROP TARGET
#define SW_DROP_TARGET_A_HIGH            3 // HIGHEST DROP TARGET
#define SW_SHOOTER_LANE                  4 // SHOOTER LANE ROLLOVER
#define SW_CREDIT_BUTTON                 5 // CREDIT BUTTON
#define SW_TILT                          6 // TILT
#define SW_OUTHOLE                       7 // CREDIT BUTTON
#define SW_COIN_3                        8 // COIN III (RIGHT)
#define SW_COIN_1                        9 // COIN I (LEFT)
#define SW_COIN_2                       10 // COIN III (MIDDLE)
#define SW_SLAM                         15 // SLAM
#define SW_HYPER_SPACE_LOWER            17 // LOWER HYPER SPACE ROLLOVER BUTTON
#define SW_RIGHT_FLIPPER_LANE           18 // RIGHT FLIPPER LANE
#define SW_LEFT_FLIPPER_LANE            19 // LEFT FLIPPER LANE
#define SW_500_POINT_TARGET             20 // 500 POINT TARGET
#define SW_HYPER_SPACE_UPPER            21 // UPPER HYPER SPACE ROLLOVER BUTTON
#define SW_10_POINT_REBOUND             22 // 10 POINT REBOUND (4)
#define SW_300_OR_3000_TARGET           23 // 300 / 3000 TARGET
#define SW_PLANET_INLANE                24 // PLANETARY BYPASS INLANE
#define SW_PLANET_OUTLANE               25 // PLANETARY BYPASS OUTLANE
#define SW_KLINGON_Y                    26 // KLINGON Y
#define SW_KLINGON_L2                   27 // KLINGON L #2
#define SW_KLINGON_L1                   28 // KLINGON L #1
#define SW_KLINGON_A                    29 // A LANE
#define SW_KLINGON_B                    30 // B LANE
#define SW_SAUCER                       31 // SAUCER
#define SW_RIGHT_OUTLANE                33 // RIGHT OUTLANE
#define SW_LEFT_OUTLANE                 34 // LEFT OUTLANE
#define SW_RIGHT_SLINGSHOT              35 // RIGHT SLINGSHOT
#define SW_LEFT_SLINGSHOT               36 // LEFT SLINGSHOT
#define SW_CENTER_THUMPER_BUMPER        37 // CENTER THUMPER BUMPER
#define SW_RIGHT_THUMPER_BUMPER         38 // RIGHT THUMPER BUMPER
#define SW_LEFT_THUMPER_BUMPER          39 // LEFT THUMPER BUMPER

// Defines for solenoids
// NOTE: Coin lockout and flipper enable are accessed through calls to RPU routines, never through the solenoid stack

#define SO_KNOCKER                  5 // KNOCKER
#define SO_OUTHOLE                  6 // OUTHOLE KICKER
#define SO_SAUCER                   7 // SAUCER
#define SO_POP_LEFT                 8 // LEFT THUMPER BUMPER
#define SO_POP_RIGHT                9 // RIGHT THUMPER BUMPER
#define SO_POP_CENTER              10 // TOP THUMPER BUMPER
#define SO_SLING_LEFT              11 // LEFT SLINGSHOT
#define SO_SLING_RIGHT             12 // RIGHT SLINGSHOT
#define SO_DTARGET_RESET           13 // DROP TARGET RESET

// Game Machine States
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_SKILL_SHOT      3
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 90
#define MACHINE_STATE_MINI_GAME_1     91
#define MACHINE_STATE_MINI_GAME_2     92
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110


// Extended Self-Test Machine States (See also SelfTestAndAudit.h)

#define MACHINE_STATE_TEST_BACKGROUND_ON        -20
#define MACHINE_STATE_TEST_FREEPLAY             -21
#define MACHINE_STATE_TEST_BALL_SAVE            -22
#define MACHINE_STATE_TEST_GAIN                 -23

#define NUM_EXTENDED_SETTINGS                     4

// Storage for Extended Self-Test Game settings
#define RPU_BALL_SAVES           50
#define RPU_BALL_SAVE_SCORE      51
#define RPU_BALL_SAVE_NUM_SECS   52
#define RPU_BG_GAIN              53
#define RPU_SFX_GAIN             54
#define RPU_VOICE_GAIN           55
#define RPU_BACKGROUND_ON        76
#define RPU_HIGHSCORE_AWARD      77
#define RPU_FREEPLAY             78
#define RPU_GOAL_OFFON           79

// see also RPU_Config.h #define RPU_testname, currently up to 82 + 3 = 85




// SWITCHES_WITH_TRIGGERS are for switches that will automatically
// activate a solenoid (like in the case of a chime that rings on a rollover)
// but SWITCHES_WITH_TRIGGERS are fully debounced before being activated
#define NUM_SWITCHES_WITH_TRIGGERS 5

// PRIORITY_SWITCHES_WITH_TRIGGERS are switches that trigger immediately
// (like for pop bumpers or slings) - they are not debounced completely
#define NUM_PRIORITY_SWITCHES_WITH_TRIGGERS 5


// Define automatic solenoid triggers (switch, solenoid, number of 1/120ths of a second to fire)
struct PlayfieldAndCabinetSwitch TriggeredSwitches[] = {
   {SW_RIGHT_SLINGSHOT,        SO_SLING_RIGHT, 4},
   {SW_LEFT_SLINGSHOT,         SO_SLING_LEFT,  4},
   {SW_RIGHT_THUMPER_BUMPER,   SO_POP_RIGHT,   3},
   {SW_LEFT_THUMPER_BUMPER,    SO_POP_LEFT,    3},
   {SW_CENTER_THUMPER_BUMPER,  SO_POP_CENTER,  3}
};


struct FindLamp {  // Data required to locate lamps on playfield. Expressed from zero to 250. 255 indicates backglass.
  byte horizontal; // Distance from lowest lamp
  byte vertical;   // Distance from left-most lamp
  byte distance;   // Straight-line distance from centre of playfield
};

// Distance to every lamp, proportionally adjusted from zero to 250. Horizontal, Vertical, Straight-Line from centre. 255 indicates backglass.
struct FindLamp LampPlacement[60] = {
  { 125,   0, 241}, // 1K BONUS
  { 125,   9, 224}, // 2K BONUS
  { 125,  15, 212}, // 3K BONUS
  { 125,  24, 195}, // 4K BONUS
  { 125,  31, 181}, // 5K BONUS
  { 125,  38, 167}, // 6K BONUS
  { 125,  44, 156}, // 7K BONUS
  { 125,  53, 139}, // 8K BONUS
  { 125,  60, 125}, // 9K BONUS
  { 125,  68, 110}, // 10K BONUS
  { 125,  81,  85}, // 20K BONUS
  {  85, 110,  46}, // LEFT NACELLE
  { 165, 110,  46}, // RIGHT NACELLE
  { 152,  82,  86}, // CENTER 3X, OR { 101, 132,  27}, // PLANET SPECIAL
  {  91,  82,  88}, // CENTER 2X, OR {  79, 147,  60}, // PLANET 3X
  {  70, 121,  52}, // PLANET 2X
  {  61, 185, 130}, // HYPER SPACE 2K BONUS
  {  55, 191, 143}, // HYPER SPACE 4K BONUS
  {  49, 197, 156}, // HYPER SPACE 6K BONUS
  {  43, 203, 168}, // HYPER SPACE 8K BONUS
  {  37, 209, 181}, // HYPER SPACE 10K BONUS
  {  30, 215, 194}, // HYPER SPACE SPECIAL
  {  21, 178, 140}, // PLANETARY BYPASS INLANE
  {   0, 178, 154}, // PLANETARY BYPASS OUTLANE
  { 198, 250, 250}, // SAUCER B
  { 195, 244, 238}, // SAUCER A
  { 165, 213, 174}, // SAUCER L #1
  { 159, 206, 159}, // SAUCER L #2
  { 152, 199, 144}, // SAUCER Y
  { 149, 137,  32}, //  300 / 3000 TARGET
  { 229,  78, 132}, // SPOCK INLANE
  {  21,  78, 132}, // ILIA INLANE
  { 119, 249, 238}, // B LANE
  { 149, 244, 230}, // A LANE
  { 198, 135,  71}, // KLINGON L #1
  { 207, 125,  76}, // KLINGON L #2
  { 220, 116,  89}, // KLINGON Y
  { 250,  78, 147}, // SPOCK OUTLANE
  {   3,  78, 145}, // ILIA OUTLANE
  { 168,  93,  74}, // MIDDLE Y
  { 255, 255, 255}, // SAME PLAYER SHOOTS AGAIN, BACKBOX
  { 255, 255, 255}, // MATCH
  { 146,  35, 174}, // SAME PLAYER SHOOTS AGAIN, PLAYFIELD
  { 255, 255, 255}, // CREDIT INDICATOR, APRON
  {  91, 101,  55}, // MIDDLE 10K
  { 113, 101,  47}, // MIDDLE 25K
  { 137, 101,  47}, // MIDDLE 50K
  { 159, 101,  55}, // MIDDLE SPECIAL
  { 255, 255, 255}, // BALL IN PLAY
  { 255, 255, 255}, // HIGH SCORE
  { 255, 255, 255}, // GAME OVER
  { 255, 255, 255}, // TILT
  {  82,  94,  71}, // MIDDLE B
  { 104,  94,  63}, // MIDDLE A
  { 125,  94,  59}, // MIDDLE L #1
  { 146,  94,  63}, // MIDDLE L #2
  { 255, 255, 255}, // PLAYER 1
  { 255, 255, 255}, // PLAYER 2
  { 255, 255, 255}, // PLAYER 3
  { 255, 255, 255}  // PLAYER 4
};



struct FindLamp AlternativeLamps[4] = { // centre 3x and planet special turn on and off together. Centre 2x and planet 3x turn on and off together.
  { 152,  82,  86}, // CENTER 3X
  {  91,  82,  88}, // CENTER 2X
  { 101, 132,  27}, // PLANET SPECIAL
  {  79, 147,  60}  // PLANET 3X
};



// Sound Data [256 sounds][Sound Type (0 No Sound - 1 Background - 2 Voice - 3 Sound FX), Length (10ths of a second)]
// NOTE: Sound type 0 is considered invalid, and IS NOT PLAYED! If you add sounds, be sure to fill in the sound type!
byte SoundTimings[256][2] = { 
{0,   0}, // 000 
{0,   0}, // 001 
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{3,  10}, // 008 Beep 3- 10 Chime
{3,   5}, // 009 Beep 44 - 100 Chime
{3,   5}, // 010 Computer beep - 1000 Chime
{0,   0}, 
{0,   0}, 
{3,  14}, // 013 Command Sequence 1
{3,  12}, // 014 Command Sequence 2
{3,  15}, // 015 Command Sequence 3
{3,  12}, // 016 Command Sequence 4
{0,   0},
{0,   0},
{3,  45}, // 019 Power Up
{2,  36}, // 020 Computer: Working...
{2,  23}, // 021 Fire all phasers: bonus x 1
{2,  38}, // 022 Fire all phasers: bonus x 2
{2,  53}, // 023 Fire all phasers: bonus x 3
{2,  28}, // 024 One Minute to Auto Destruct
{2,  12}, // 025 Bridge to Captain 
{2,  12}, // 026 Fascinating
{2,  45}, // 027 Fool Me Once...
{2,  35}, // 028 If My Granny had wheels...
{2,  32}, // 029 I Can't Change the Laws of Physics
{2,  10}, // 030 Kirk: Beam me up Scotty!
{2,  10}, // 031 Scotty: Hello Computer
{2,  20}, // 032 McCoy: You're out of your Vulcan mind!
{2,  28}, // 033 Scotty: The warp drive is a hopeless pile of junk
{2,  20}, // 034 Spock: Your logic was impeccable Captain
{2,  18}, // 035 Spock: Vulcans never bluff
{2,  16}, // 036 Kirk: Standard orbit Mr. Chekhov
{0,   0}, // 037 
{0,   0}, // 038 
{0,   0}, // 039 
{0,   0}, // 040 
{3,  11}, // 041 Small explosion 1
{3,  11}, // 042 Small Explosion 2
{3,  11}, // 043 Small explosion 3
{3,  43}, // 044 Enterprise Explodes
{0,   0}, // 045 
{0,   0}, // 046 
{0,   0}, // 047 
{0,   0}, // 048 
{0,   0}, // 049 
{0,   0}, // 050 
{2,  15}, // 051 Kirk: Scotty, Energize
{2,  31}, // 052 Kirk: James Kirk, commanding the Starship Enterprise
{2,  20}, // 053 Kirk: Fully Operable Scotty?
{2,  30}, // 054 Scotty: Prepare to take us out of orbit
{2,  24}, // 055 Uhura: It's a big galaxy, Mr. Scott
{3,  14}, // 056 Transporter (Louder)
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{2,  33}, // 061 Kirk: Ahead warp factor 1
{2,  25}, // 062 Kirk: Full ahead warp factor 1
{2,  26}, // 063 Kirk: Full ahead Mr Sulu
{2,  68}, // 064 Kirk: Mr Spock full ahead warp factor 1
{2,  21}, // 065 Kirk: Warp 1 Mr Sulu
{2,  13}, // 066 Kirk: Arm photon torpedos
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{2,  60}, // 071 Kirk: Photon torpedoes fire, 3 quick
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{2,  20}, // 076 Spock: We may be here for a very long time
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{2,  27}, // 081 Spock: He's intelligent but not experienced
{2,  11}, // 082 McCoy: He's dead captain
{2,  12}, // 083 McCoy: He's dead Jim
{2,  18}, // 084 Spock: Most illlogical
{2,  37}, // 085 McCoy: You can't argue with a machine
{0,   0}, // 086 
{0,   0}, // 087 
{0,   0}, // 088 
{0,   0}, // 089 
{0,   0}, // 090 
{0,   0}, // 091
{0,   0}, // 092
{0,   0}, // 093
{0,   0}, // 094 
{0,   0}, // 095 
{0,   0}, // 096
{0,   0}, // 097
{0,   0}, // 098
{0,   0}, // 099
{0,   0}, // 100
{0,   0}, // 101
{0,   0}, // 102
{0,   0}, // 103
{3,  11}, // 104 Small explosion clipped
{3,  11}, // 105 Small explosion clipped
{3,  11}, // 106 Small explosion clipped
{2,  33}, // 107 Khan!
{3,   7}, // 108 Phaser
{0,   0}, // 109
{0,   0}, // 110 
{0,   0}, // 111
{0,   0}, // 112
{2,  40}, // 113 Khan! Louder
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{2,  49}, // 122 Uhura: Calling Klingon vessel
{2,  21}, // 123 Kirk: Phasers on stun
{2,  18}, // 124 Kirk: Prepare to transport
{2,  26}, // 125 Kirk: Try again
{0,   0}, // 126 
{0,   0}, // 127 
{0,   0}, // 128
{0,   0}, // 129
{2,  47}, // 130 Sulu: Intruder alert
{2,  31}, // 131 Sulu: All hands to battle stations
{2,  24}, // 132 Sulu: All phaser banks fire
{2,  39}, // 133 Uhura: Klingon vessel acknowledge please
{2,  23}, // 134 Uhura: Sensors picking up a klingon battle cruiser
{2,  17}, // 135 Chekov: Number 2 shield is gone
{2,  39}, // 136 Kirk: Beam me aboard
{2,  12}, // 137 Kirk: Beam me up Mr Spock
{2,   9}, // 138 Kirk: How bad is it
{2,  12}, // 139 Kirk: Phasers on stun
{2,  46}, // 140 Kirk: Prepare to attack
{2,  39}, // 141 Kirk: We're running out of time
{2,  22}, // 142 Kirk: Activate tractor beam
{2,  17}, // 143 Kirk: We need power
{2,  37}, // 144 McCoy: I could cure a rainy day
{2,   7}, // 145 McCoy: Fascinating
{2,  21}, // 146 McCoy: I'm a doctor not a bricklayer
{2,  21}, // 147 McCoy: I'm a doctor not a mechanic
{2,  45}, // 148 McCoy: Most cold blooded man I've ever known
{2,  35}, // 149 Scotty: Alright you lovelies
{2,  39}, // 150 Scotty: We'll get more speed out of her
{2,  21}, // 151 Spock: Any chance these are hallucinations
{2,  39}, // 152 Spock: I'm sure there is an answer
{2,  16}, // 153 Spock: Engine systems coming on
{2,  27}, // 154 Spock: Extremely interesting 
{2,   8}, // 155 Spock: Illogical
{2,  43}, // 156 Spock: Irritating one of your earth emotions
{2,  18}, // 157 Spock: Live long and prosper
{2,  78}, // 158 Spock: Can you manage warp 7
{2,  12}, // 159 Spock: This is not a drill
{2, 155}, // 160 Scotty: I'll sit on the warp engines and nurse them
{2,  30}, // 161 Kids: It a big galaxy
{2,  15}, // 162 Kids: Hello, Computer
{2,  25}, // 163 Kids: Scotty, energize
{2,  34}, // 164 Kids: Fully Operable
{2,  27}, // 165 Kids: Warp factor one
{2,  50}, // 166 Kids: Kids: Battle stations
{2,  47}, // 167 Kids: Calling Klingon vessel
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{1,   0}, // 180 Bridge background
{1,   0}, // 181 Bridge background
{1,   0}, // 182 Brass monkeys background
{1,   0}, // 183 Time reverse background
{1,   0}, // 184 Trouble with Tribbles background
{1,   0}, // 185 Silvery orbs background
{1,   0}, // 186 Goodbye Mr. Decker background
{1,   0}, // 187 Series main title background
{3,  70}, // 188 Opening fanfare
{1,   0}, // 189 Kirk's Explosive Reply background
{1,   0}, // 190 Surprise Attack background
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{3,   5}, // 202 Powerup Chime
{3,  18}, // 203 PowerupTune
{3,   5}, // 204 Coinup Noise
{3,  17}, // 205 Coinup Tune
{3,   4}, // 206 Startup Noise
{3,  15}, // 207 Startup Tune
{0,   0}, 
{0,   0}, 
{0,   0}, 
{3,   2}, // 211 10 Noise
{3,   2}, // 212 100 Noise
{3,   2}, // 213 1000 Noise
{3,  10}, // 214 Saucer Pretune
{3,   7}, // 215 Saucer Noise
{3,  37}, // 216 Bally Bonus Tune
{3,  16}, // 217 Tilt Tune
{3,  16}, // 218 Game Over Tune
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0},
{0,   0}
};

// *** Sound File Groups. First value is number of files in group:

// byte BackgroundMusic[8] = {7, 181, 182, 183, 184, 185, 186, 187};
byte BackgroundMusic[8] = {7, 181, 181, 181, 181, 181, 181, 181}; // Use if only bridge sounds available
byte VoiceStartup[4] = {3, 31, 51, 53};
byte VoiceBridge[8] = {7, 52, 61, 65, 122, 123, 133, 135};
byte VoiceFun[4] = {3, 25, 55, 124};
byte VoiceAdventure[11] = {10, 30, 122, 123, 130, 131, 133, 135, 139, 140, 143};
byte VoicePlanet[4] = {3, 36, 54, 113};
byte VoiceWin[4] = {3, 34, 64, 144};
byte VoiceLose[7] = {6, 29, 33, 81, 82, 85, 143};
byte VoiceTilt[4] = {3, 27, 33, 76};
byte VoiceAttract[9] = {8, 28, 32, 52, 53, 55, 65, 84, 131};
byte VoiceKidsMode[8] = {7, 161, 162, 163, 164, 165, 166, 167};
byte VoiceFireAllPhasers[4] = {3, 21, 22, 23}; // Note: This is NOT used in random selection! Firing 1, 2, or 3 phaser blasts is selected using bonus multiplier
byte SFXCommandSequence[5] = {4, 13, 14, 15, 16};

// *** Single sound files

byte VoiceBallSave            = 125; // Try again
byte VoiceTiltEnd             = 83;  // He's dead, Jim
byte VoiceSaucer              = 66;  // Arm Photon Torpedoes
byte VoicePhotonTorpedoesFire = 71;  // Photon Torpedoes, Fire!
byte VoiceMatch               = 20;  // Working...
byte VoiceMiniGame1           = 134; // Sensors picking up a klingon battle cruiser
byte VoiceMiniGame2           = 24;  // One Minute to Auto Destruct
byte VoiceTest                = 160; // Used in self-test volume control

byte SFXSkillShot  = 188; // Opening Fanfare
byte SFXExplosion1 = 104;
byte SFXExplosion2 = 105;
byte SFXExplosion3 = 106;
byte SFXExplosion4 = 41;
byte SFXExplosion5 = 42;
byte SFXExplosion6 = 43;
byte SFXEnterpriseExplodes = 44;
byte SFXSlingshot  = 56;  // Transporter
byte SFXTest       = 9;   // Used in self-test volume control
byte SFXChime10    = 8;   // 10 chime
byte SFXChime100   = 9;   // 100 chime
byte SFXChime1000  = 10;  // 1000 chime
byte SFXAddPlayer  = 56;  // Transporter

// byte BGMiniGame1   = 189; // Music for mini-game 1
// byte BGMiniGame2   = 190; // Music for mini-game 2
byte BGMiniGame1   = 181; // Music for mini-game 1 // Use if only bridge sounds available
byte BGMiniGame2   = 181; // Music for mini-game 2 // Use if only bridge sounds available
byte BGTest        = 181; // Used in self-test volume control
