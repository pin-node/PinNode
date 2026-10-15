/**************************************************************************

Bally Star Trek: Arduino Version 2025.04 by Dave's Think Tank

  Version History*

  Version: 2025.04 by Dave's Think Tank
  Version: 2025.06 by Dave's Think Tank
  Version: 2025.09 by Dave's Think Tank

  * see detailed version notes below

*/

/**************************************************************************

  This file is part of Bally Star Trek 2025 -  for the Arduino Mega 2560

  The author of this program disclaims all copyright in order to make this 
  program freely available in perpetuity to anyone who would like to use 
  it. David McIntosh, 2025/04/19

  Bally Star Trek 2025 is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Bally Star Trek 2025 is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  See <https://www.gnu.org/licenses/>.

*/

/**************************************************************************

Bally Star Trek: Arduino Version 2025.04 by Dave's Think Tank

Documentation of the Project:
- Detailed document created covering all new rules, self tests, accounting values, game settings, DIP switches, and more!

Changes / Additions:

1. Major New Features:

- Added "Ball Save":
  - If ball scores less than 5000 points, or lasts less than 15 seconds, the ball is played over.

- Added Skill Shot:
  - Next required letter in the word "BALLY" flashes. If you hit it, you get 3000 additional points. 
  - Award is also doubled every time 10-point bumper is hit.
  - 10-point bumper advances letters as usual.
  - If no letter is flashing, you need to advance them to the next required letter in the word "BALLY".
  - Double points are awarded even if you miss the required letter. If the required letter is impossible, your skill shot becomes 
    to hit the 10-point rebound (two or more times if you can!) before landing in the saucer.

- Added two new mini-games:

- Mini-Game 1: Klingon Battle
  - Activated by reaching second score level. Receive one extra ball for the purpose of playing the mini-game.
  - Five standing targets represent Klingon ships. Earn 10,000 for hitting each one, plus 50,000 for defeating them all.

- Mini-Game 2: Auto-Destruct Sequence
  - Activated by reaching third score level. Receive one extra ball for the purpose of playing the mini-game.
  - Alien intruders have started the ship's auto-destruct sequence and it must be deactivated.
  - Hit four drop targets in 60 seconds to deactivate auto-destruct.
  - 12,500 points awarded for each drop target hit, plus 50,000 for hitting all four.
  - 60-second countdown timer can be viewed in the credit window.

- Added three new sound and light shows:

- Show #1: Fire Photon Torpedoes!
  - Spell "BALLY", and Captain Kirk will order helm to fire photon torpedoes.
  - Three photon torpedoes are fired from the Enterprise in a display of light and sound.

- Show #2: Fire All Phasers!
  - Earn the bonus by re-entering the shooter lane, or falling into the out hole.
  - Captain Kirk will order helm to fire all phasers.
  - Phasers are fired from the Enterprise in a display of light and sound, one, two, or three times, depending on multiple earned.

- Show #3: Computer... Working...
  - The match result is calculated by the Enterprise computer.
  - The computer announces it is "Working...", while random lights flash off and on, just like the original!

- Added "Kids' Mode":
  - Pressing the credit switch and coin 3 switch at the same time puts you into Kids' Mode. Sound cues let you know what mode you are in.
  - Kids' Mode is intended to be used for children or guests, to set the pinball to all easy DIP switch and self-test game settings.
  - Press the credit switch and coin 3 switches at the same time again to return to regular mode.
  - Turning the machine off will also end Kids' Mode.

2. Added DIP-switch variables to control the game as per the original Bally manual:
- DIP switch 1-5, 9-13, 25-28: Set coins per credit, and credits per coin, for each coin chute. As per the original Bally manual (more or less!)
- DIP switch 6-7: High score to date award setting. As per the original manual.
- DIP switch 8: Ignored (previously handled chimes, tunes, and knocker sounds, but no longer relevant).
- DIP Switch 14-15: Playfield special awards, and scoring threshold awards (applied to scoring level 1 only. See mini-games below.) As per original manual.
- DIP Switch 16: Balls per game. As per original manual.
- DIP Switch 17-19: Maximum credits. As per original manual.
- DIP Switch 20: Number of credits to be displayed, yes or no. As per original manual.
- DIP Switch 21: Match feature on or off. As per original manual.
- DIP Switch 22: Hyperspace starting value. As per original manual.
- DIP Switch 23: BALLY special remains lit, yes or no. As per original manual.
- DIP Switch 24: Center target remains lit or alternates. As per original manual.
- DIP Switch 29: BALLY starts at 10,000 or 25,000. As per original manual.
- DIP Switch 30: Outlane special, both lanes lit or alternate. As per original manual.
- DIP Switch 31: Flipper feed lanes, both lit for hyperspace or alternate. As per original manual.
- DIP switch 32: Ignored (previously handled chimes, tunes, and knocker sounds, but no longer relevant).

3. Many additional features were added to the Self-Test function:
- Lamp test: Added ability to turn on one light at a time.
- Display test: Added ability to turn on a single digit at a time.
- Solenoid test: Added ability to operate a single solenoid, or turn all solenoids off without leaving the test program. Added ability to use flippers.
- Solenoid test: Added feature to warn you when vibration from solenoid is causing a switch to close.
- Stuck switch test: Added ability to view up to four stuck switches at once. Instructions in manual show how to use this to find switch matrix errors.
- Double-hit switch test added. Allows you to check if a switch is "bouncing", causing it to be scored more than once when hit.
- Sound test: Changed to allow cycling through all sounds. Features using game button allow skipping sounds, repeating sounds, fast forward through sounds.
- DIP-switch test added. Shows all DIP switch values. Allows you to temporarily change a DIP switch value, and see the effect in an actual game.
- Added options in self test for all accounting values to be increased, as well as zeroed.
- Added "Personal Goal" to self tests. This is used to provide two different "types" of end-of-game sounds, good or bad, depending on if you beat your personal goal or not.
- Background music / sounds off or on.
- Free play, off or on.
- Ball save options (number of ball saves per game, maximum score and time to activate ball save).
- Relative volume settings for background music, voices, and sound effects.

4. Other Changes and Additions:
- Version number (2025.04) displayed at beginning of game in display #1 (2025) and credit display (04). This is the date I started working on
  the project. Future changes made by other programmers can use the same convention, with less worry about creating multiple versions with the
  same identifier.
- New sounds added, to work with a WAV Trigger board. Background music, sound effects, and voices.
- Gain (sort of like volume) control added to sounds. Defaults for background, sfx, and voices, plus an easy method to override.
- Attract Mode, added rotating planet lights.
- All scoring checked. Conforms with descriptions on playfield, and original game to the extent possible.
- Scrolling displays during attract mode.
- Flashing display to identify next player.

Bug Fixes:
- High scores of a million or more are now maintained, and displayed in the 6-digit displays(!) Previously, your score simply started over again
  at zero.

*/

/**************************************************************************

Version 2025.06 by Dave's Think Tank

Changes / Additions:
- Light show added to Enterprise explosions and mini-game winners in Mini Games 1 and 2
- Lamp self-test has been extended to include six light shows from the game - photon torpedoes, phasers, Enterprise explosions, mini-game winner lights,
    Enterprise computer "Working..." (match), attract mode lights

Bug Fixes:
- Under some circumstances, B, A, L, L, or Y scored during photon torpedoes was being displayed at the switch but not in the middle BALLY lights.
*/

/**************************************************************************

Version 2025.09 by Dave's Think Tank

Changes / Additions:
- Coin lockout and K1 flipper relay have been added to the solenoid self-test.
- Slingshot sound changed to transporter, at regular volume.
- Reduced gain on "Fire all phasers" to sfx gain.
- Self-destruct timer starts at 60 when mini-game begins (rather than blank for 1.7 seconds). Flashes when game ends, to give you some sense of how you did.
- When bonus scored, points awarded will flash in the credit window. Since bonus ladder used for light show, this allows you to know how many points were won.
- When BALLY scored, points awarded will flash in the credit window. No reason, other than consistency with the bonus score!
*/

//======================================= OPERATOR GAME ADJUSTMENTS =======================================
#define VERSION_NUMBER  2025.09        // Version number appears in Display #1 / Credit display at start of game
#define ATTRACT_SPEECH_TIMER 300000    // (300000) Amount of time between attract mode speech callouts. 60000 = 1 minute
#define DEBUG_MODE 0                   // 0 = no debug messages.
                                       // 1 = up to nine switch numbers scrolled through player displays 2, 3, and 4. 
                                       // 2 = display at player 4 the number of calls per second at location of function LoopCount().
#define MAX_TILT_WARNINGS 1            // Set to number of tilt warnings allowed before losing ball. MINIMUM = 1!
//=========================================================================================================

#include "RPU_Config.h"
#include "RPU.h"
#include "ST2025p09.h"
#include "SelfTestAndAudit.h"

// The following required for WAV Trigger sound routines:

#include "SendOnlyWavTrigger.h"

void SetupAudio(byte sound = 0);
void StopAudio();
void StopAllButBG();
void PlaySoundEffect(byte soundEffectNum, int gain = 100);
void ResetGain(byte soundEffectNum, int gain);
void ResumeBackgroundSong();
void PlayBackgroundSong(byte songNum, int gain = 100);
void SetTrackLoop(byte soundNum, boolean enable);
void StopBackgroundSong();
inline void StopSoundEffect(byte soundEffectNum);
void PlaySound(byte soundByte, unsigned long delaySound = 0, int gain = 100);
void PlayQueue();

unsigned long WTWaitingTime = 0;

// End of WAV Trigger sound setup

// Other functions needing to be defined:

boolean BALLYScored(boolean selfTest = 0);

struct Debounce { // define Debounce structure, to be used in eliminating double hits
  unsigned long start;
  byte wait;
};

// If you're having trouble with double hits, set a "wait" time (second column) until a switch can be re-activated (max wait time 255 ms)
struct Debounce SwitchDebounce[40] = {

{0,   0}, // SW_DROP_TARGET_D_LOW             0 // LOWEST DROP TARGET
{0,   0}, // SW_DROP_TARGET_C_MID_LOW         1 // LOWER MIDDLE DROP TARGET
{0,   0}, // SW_DROP_TARGET_B_MID_HIGH        2 // UPPER MIDDLE DROP TARGET
{0,   0}, // SW_DROP_TARGET_A_HIGH            3 // HIGHEST DROP TARGET
{0, 250}, // SW_SHOOTER_LANE                  4 // SHOOTER LANE ROLLOVER
{0,   0}, // SW_CREDIT_BUTTON                 5 // CREDIT BUTTON
{0,   0}, // SW_TILT                          6 // TILT
{0,   0}, // SW_OUTHOLE                       7 // CREDIT BUTTON
{0,   0}, // SW_COIN_3                        8 // COIN III (RIGHT)
{0,   0}, // SW_COIN_1                        9 // COIN I (LEFT)
{0,   0}, // SW_COIN_2                       10 // COIN III (MIDDLE)
{0,   0}, //                                 11
{0,   0}, //                                 12
{0,   0}, //                                 13
{0,   0}, //                                 14
{0,   0}, // SW_SLAM                         15 // SLAM
{0,   0}, //                                 16
{0,   0}, // SW_HYPER_SPACE_LOWER            17 // LOWER HYPER SPACE ROLLOVER BUTTON
{0, 250}, // SW_RIGHT_FLIPPER_LANE           18 // RIGHT FLIPPER LANE
{0, 250}, // SW_LEFT_FLIPPER_LANE            19 // LEFT FLIPPER LANE
{0,   0}, // SW_500_POINT_TARGET             20 // 500 POINT TARGET
{0,   0}, // SW_HYPER_SPACE_UPPER            21 // UPPER HYPER SPACE ROLLOVER BUTTON
{0,   0}, // SW_10_POINT_REBOUND             22 // 10 POINT REBOUND (4)
{0,   0}, // SW_300_OR_3000_TARGET           23 // 300 / 3000 TARGET
{0, 250}, // SW_PLANET_INLANE                24 // PLANETARY BYPASS INLANE
{0, 250}, // SW_PLANET_OUTLANE               25 // PLANETARY BYPASS OUTLANE
{0,   0}, // SW_KLINGON_Y                    26 // KLINGON Y
{0,   0}, // SW_KLINGON_L2                   27 // KLINGON L #2
{0,   0}, // SW_KLINGON_L1                   28 // KLINGON L #1
{0,   0}, // SW_KLINGON_A                    29 // A LANE
{0,   0}, // SW_KLINGON_B                    30 // B LANE
{0,   0}, // SW_SAUCER                       31 // SAUCER
{0,   0}, //                                 32
{0, 250}, // SW_RIGHT_OUTLANE                33 // RIGHT OUTLANE
{0, 250}, // SW_LEFT_OUTLANE                 34 // LEFT OUTLANE
{0,   0}, // SW_RIGHT_SLINGSHOT              35 // RIGHT SLINGSHOT
{0,   0}, // SW_LEFT_SLINGSHOT               36 // LEFT SLINGSHOT
{0, 100}, // SW_CENTER_THUMPER_BUMPER        37 // CENTER THUMPER BUMPER
{0, 100}, // SW_RIGHT_THUMPER_BUMPER         38 // RIGHT THUMPER BUMPER
{0, 100}  // SW_LEFT_THUMPER_BUMPER          39 // LEFT THUMPER BUMPER
};

// if vibration from any solenoid is causing issues (e.g. switches firing), place "wait" time in this structure (max 255ms in 2nd column), and program response in ResetHitFix().
// Wait time can be determined from Solenoid test, which gives number of milliseconds between solenoid and switch. Use this as the minimum wait time.
struct Debounce ResetHits[14] = {
{0,   0}, //                             0
{0,   0}, //                             1
{0,   0}, //                             2
{0,   0}, //                             3
{0,   0}, //                             4
{0,   0}, // SO_KNOCKER                  5 // KNOCKER
{0,   0}, // SO_OUTHOLE                  6 // OUTHOLE KICKER
{0,   0}, // SO_SAUCER                   7 // SAUCER
{0,   0}, // SO_POP_LEFT                 8 // LEFT THUMPER BUMPER
{0,   0}, // SO_POP_RIGHT                9 // RIGHT THUMPER BUMPER
{0,   0}, // SO_POP_CENTER              10 // TOP THUMPER BUMPER
{0,   0}, // SO_SLING_LEFT              11 // LEFT SLINGSHOT
{0,   0}, // SO_SLING_RIGHT             12 // RIGHT SLINGSHOT
{0, 250}  // SO_DTARGET_RESET           13 // DROP TARGET RESET
};

int i, j, k;

// Note: Gain on individual sound files is high. Gain on all sounds reduced to prevent clipping when sounds played together.
int InitBgGain = -10;                  // Initial gain (~volume) to be applied to background music
int InitSfxGain = -14;                 // Initial gain to be applied to sound effects
int InitVoiceGain = -8;                // Initial gain to be applied to sounds identified as voices

int AdjBgGain = 50;                    // Gain adjustment (~volume) to be applied to background music
int AdjSfxGain = 50;                   // Gain adjustment to be applied to sound effects
int AdjVoiceGain = 50;                 // Gain adjustment to be applied to sounds identified as voices

int bgGain = -10;                      // Gain (~volume) to be applied to background music
int sfxGain = -14;                     // Gain to be applied to sound effects
int voiceGain = -8;                    // Gain to be applied to sounds identified as voices

int MachineState = 0;
boolean MachineStateChanged = true;
unsigned long CurrentTime = 0;
unsigned long AttractWait = 0;
byte HoldQuarter = 255;
unsigned long LoopTime = 0;
unsigned long LCount = 0;

unsigned long DisplayTimer = 0;
unsigned long DisplayValue = 0;
unsigned long kidTimer = 0;
unsigned long DropTimer = 0;
unsigned long AttractDisplayTimer = 0;
byte ChimeSpeed = 100;

byte dispSwitch[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
byte switchHit;
byte kidSwitch = 0xFF;

unsigned long TotalPlays = 0;
unsigned long TotalReplays = 0;
unsigned long ScoreAward1 = 0;
unsigned long ScoreAward2 = 0;
unsigned long ScoreAward3 = 0;
unsigned long PersonalGoal = 0;
unsigned long highestScore = 0;

byte ScoreAwardStates[4] = {0, 0, 0, 0};
byte MiniGamePlayed  [4] = {0, 0, 0, 0};
byte MiniGame1Score  [5] = {0, 0, 0, 0, 0}; 
byte MiniGame2Score  [4] = {0, 0, 0, 0}; 
boolean MiniGameStarted  = 0;
boolean MiniGameEnded    = 0;
unsigned long MiniGamePauseTimer;
unsigned long CountdownTimer = 0;
byte DestructTimer = 0;
byte FinalTime = 0;

unsigned long Credits = 0;
byte WholeCredit = 0;
unsigned long CoinsChute1 = 0;
unsigned long CoinsChute2 = 0;
unsigned long CoinsChute3 = 0;

unsigned long AttractStartAnimation = 0;

byte AttractHeadMode = 255;
byte AttractPlayfieldMode = 255;
unsigned long AttractCalloutTimer = 0;

byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
unsigned long CurrentScores[4];
byte scoreChimes[3] = {0, 0, 0};
boolean chimeOff10   = false;
boolean chimeOff100  = false;
boolean chimeOff1000 = false;
boolean BallDrained = false;
boolean FirstBall = 1;

byte NumTiltWarnings = 0;

byte SkillShotHits     =   0;
byte SkillShotState    =   0;        // 0 not started, 1 started, 2 collecting, 3 collected/fail
byte ShootFor          =   0;
unsigned long SkillShotDoubler = 1;
boolean SPSATurnedOff  =   0;
boolean SkillShotMade  =   0;

unsigned long BallTimeInTrough  = 0;
unsigned long BallSaveStartTime = 0;
boolean PFValidated = false;
unsigned long InitialScore = 0;
boolean ballSavePlayed = false;

unsigned long FireDelayTimer = 0;
int LampCounter;

boolean FreeBallCollected[4] = {0, 0, 0, 0}; // Yes or No
boolean FreeGameCollected[4] = {0, 0, 0, 0}; // Yes or No
boolean PlayerShootsAgain    = false;
boolean HyperSpace25000Paid = false;

unsigned long SaucerHitTime = 0;
boolean BallInSaucer        = false;

byte    SaucerBALLY[4]    = {0, 0, 0, 0};     // B = 0, A = 1, ...
boolean ScoreBALLY[4][5]  = {                 // {4 players][5 Letters] B, A, L, L, and Y on or off
                            {0, 0, 0, 0, 0},
                            {0, 0, 0, 0, 0},
                            {0, 0, 0, 0, 0},
                            {0, 0, 0, 0, 0}
                            };
byte    HyperSpace2468    = 0;                // Level of Hyper Space bonus: 2, 4, 6, 8, 10, 12+ = Special
byte    BonusXState[4]    = {1, 1, 1, 1};     // [4 players] Multiple of bonus: 1x, 2x, or 3x
byte    PhotonBonus[4]    = {0, 0, 0, 0};     // Photon torpedo bonus level 10, 25, 50, Special
byte    PhaserBonus       = 1;                // Phaser bonus score, 1-29
boolean LeftRight         = 0;                // Lights left or right
byte    BALLYMiddle[5]    = {LA_MIDDLE_B,     // Non-consecutive letters(!) made to be consecutive
                             LA_MIDDLE_A, 
                             LA_MIDDLE_L1, 
                             LA_MIDDLE_L2, 
                             LA_MIDDLE_Y
                             };
boolean PhotonLightShow   = 0;
boolean PhaserLightShow   = 0;
boolean PhotonsDone       = 0;
boolean EELightShow       = 0;
boolean WinnerLightShow   = 0;
boolean MatchLightShow    = 0;
boolean AttractLightShow  = 0;

byte backgroundSound = 0;

#define MAX_PHASER_BONUS 29
byte PhotonStep                       = 0;
byte PhotonStep2                      = 0;
byte PhaserStep                       = 0;
byte PhotonNumber                     = 0;
unsigned long PhotonLightShowTimer    = 0;
unsigned long PhotonLightShowTimer2   = 0;
unsigned long PhaserLightShowTimer    = 0;
unsigned long PhotonScore             = 0;

unsigned long MatchSequenceStartTime  = 0;
unsigned long MatchDelay              = 300;
byte MatchDigit      = 0;
byte NumMatchSpins   = 0;
byte ScoreMatches    = 0;
byte workingMatch[4] = {0, 0, 0, 0};
byte finalMatch[4]   = {0, 0, 0, 0};

// DIP Switch Variables
byte dipBank[4]; 
byte dipChute1;
byte dipHighScoreAward;
byte dipChute3;
byte dipThresholdAward;
byte dipBallsPerGame;
byte dipMaxCredits;
boolean dipCreditsDisplayed;
boolean dipMatch;
boolean dipHyperSpaceAdjustment;
boolean dipBallySpecialAdjustment;
boolean dipCenterAdjustment;
byte dipChute2;
boolean dipBally10000Adjustment;
boolean dipOutlaneAdjustment;
boolean dipFlipperFeedAdjustment;

byte CoinsperCredit[3]; // Number of coins required to earn one credit, by coin chute
byte CreditsperCoin[3]; // Number of credits received per coin, by coin chute
unsigned long GetValue;

boolean backgroundOn = 1;            // (1) background sound / music set either on or off
boolean FreePlay = 0;                // (0) Turn free play either on or off
boolean PersonalGoalOn = 1;          // (1) Turn personal goal feature on or off
byte    kidMode = 0;                 // (0) Beginner mode settings applied, specifically for kids
boolean OriginalRules = 0;           // (0) Accept or reject certain rules from the original game
byte          ballSaves         = 1;
unsigned long ballSaveScore     = 5000;
unsigned long ballSaveNumMS     = 15000;
byte BallSaveUsed[4]            = {ballSaves, ballSaves, ballSaves, ballSaves};

unsigned long HighScore = 0;
unsigned long HighScoreBeat = 0;

signed long   ExtSavedValue = 0;
unsigned long ExtResetHold = 0;
unsigned long ExtLastResetPress = 0;
byte          dispNum = 0;
int           UpDown = 1;


// #################### SET UP DIP Variables using dipBank[] ####################
void SetupDIPVars() { // Set up all variables derived from the DIP switches
  dipChute1 = dipBank[0] & 31;
  dipHighScoreAward = (dipBank[0] & 96) >> 5;
  //dip switch 8 ignored

  dipChute3 = dipBank[1] & 31;
  dipThresholdAward = (dipBank[1] & 96) >> 5;
  dipThresholdAward = dipThresholdAward + (dipThresholdAward < 2); // 1-2-3 = no award - extra ball - replay
  dipBallsPerGame = ((dipBank[1] & 128) == 128) ? 5 : 3;

  dipMaxCredits = 5 + 5 * (dipBank[2] & 7);
  dipCreditsDisplayed = (dipBank[2] & 8) == 8;
  dipMatch = (dipBank[2] & 16) == 16;
  dipHyperSpaceAdjustment = (dipBank[2] & 32) == 32;
  dipBallySpecialAdjustment = (dipBank[2] & 64) == 64;
  dipCenterAdjustment = (dipBank[2] & 128) == 128;

  dipChute2 = dipBank[3] & 15;
  dipBally10000Adjustment = (dipBank[3] & 16) == 16;
  dipOutlaneAdjustment = (dipBank[3] & 32) == 32;
  dipFlipperFeedAdjustment = (dipBank[3] & 64) == 64;
  // dip switch 32 ignored
  
  // Calculate credits per coin, and coins per credit. Some of the obscure ones are ignored, or modified.

  if (dipChute1 <= 1){
    CreditsperCoin[0] = 3;
    CoinsperCredit[0] = 2;
  }
  else {
    CreditsperCoin[0] = dipChute1 >> 1;
    CoinsperCredit[0] = (dipChute1 & 1) + 1;
  } 

 if (dipChute3 <= 1){
    CreditsperCoin[2] = 3;
    CoinsperCredit[2] = 2;
  }
  else {
    CreditsperCoin[2] = dipChute3 >> 1;
    CoinsperCredit[2] = (dipChute3 & 1) + 1;
  } 
 
  if (dipChute2 == 0){
    CreditsperCoin[1] = CreditsperCoin[0];
    CoinsperCredit[1] = CoinsperCredit[0];
  }
  else 
    CreditsperCoin[1] = dipChute2;
    CoinsperCredit[1] = 1;
}


// #################### SETUP ####################
void setup() {
  // read analog pin for match sequence
  randomSeed(analogRead(A6));

  // Tell the OS about game-specific lights and switches
  RPU_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, TriggeredSwitches);

  CurrentTime = millis();
  // Set up the chips and interrupts
  RPU_InitializeMPU(RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED | RPU_CMD_PERFORM_MPU_TEST, SW_CREDIT_BUTTON);
  RPU_DisableSolenoidStack();
  RPU_SetDisableFlippers(true);

  // Get all dip variables directly from dip switches and store in EEPROM
  for (i = 0; i < 4; ++i) {
    dipBank[i] = RPU_GetDipSwitches(i);
    RPU_WriteByteToEEProm(RPU_DIP_BANK + i, dipBank[i]);
  }
  MachineState = 0;
  SetupDIPVars();
  
  RPU_SetDisplay(0, floor(VERSION_NUMBER), true, 2);
  RPU_SetDisplayCredits(floor(100 * (VERSION_NUMBER + 0.005 - floor(VERSION_NUMBER))));
  RPU_SetDisplayBlank(1, 0x00);
  RPU_SetDisplayBlank(2, 0x00);
  RPU_SetDisplayBlank(3, 0x00);

  delay(4000);

  RPU_SetDisplayBlank(0, 0);
  RPU_SetDisplayBlank(5, 0);
  SetLastSelfTestChangedTime(CurrentTime);

  ReadAllEEProm();  // Update all stored data

  // Check for any invalid data

  if (Credits > dipMaxCredits) Credits = dipMaxCredits;
  RPU_SetLampState(LA_CREDIT_INDICATOR, Credits > 0);

  if ((ballSaves == 0 && ballSaveScore == 0 && ballSaveNumMS == 0) 
          || ballSaves > 25 || ballSaveScore > 25000 || ballSaveNumMS > 25000) { // Likely first time running program! Set defaults.
    ballSaves = 1;
    ballSaveScore = 5000;
    ballSaveNumMS = 15000;
    RPU_WriteByteToEEProm(RPU_BALL_SAVES, ballSaves);
    RPU_WriteByteToEEProm(RPU_BALL_SAVE_SCORE, ballSaveScore / 100);
    RPU_WriteByteToEEProm(RPU_BALL_SAVE_NUM_SECS, ballSaveNumMS / 1000);
  }
  if ((AdjBgGain <= 0 || AdjBgGain > 100) && (AdjSfxGain <= 0 || AdjSfxGain > 100)
                                              && (AdjVoiceGain <= 0 || AdjVoiceGain > 100)) { // Likely first time running program! Set defaults.
    AdjBgGain = 50;
    AdjSfxGain = 50;
    AdjVoiceGain = 50;
    RPU_WriteByteToEEProm(RPU_BG_GAIN, AdjBgGain);
    RPU_WriteByteToEEProm(RPU_SFX_GAIN, AdjSfxGain);
    RPU_WriteByteToEEProm(RPU_VOICE_GAIN, AdjVoiceGain);
  }
  CalculateGain();
  
  SetupAudio(VoiceStartup[random(1, 1 + VoiceStartup[0])]); // Play random startup sound
}



// #################### READ ALL STORED DATA ####################
void ReadAllEEProm() {
  Credits              = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
  TotalPlays           = RPU_ReadULFromEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, 0);
  TotalReplays         = RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, 0);
  ScoreAward1          = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE, 0); // 600,000
  ScoreAward2          = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE, 0); // 1,200,000
  ScoreAward3          = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE, 0); // 2,800,000 
  PersonalGoal         = RPU_ReadULFromEEProm(RPU_PERSONAL_GOAL_EEPROM_START_BYTE, 100000); // 500,000
  HighScore            = RPU_ReadULFromEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, 1000);
  HighScoreBeat        = RPU_ReadULFromEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, 0);
  CoinsChute1          = RPU_ReadULFromEEProm(RPU_CHUTE_1_COINS_START_BYTE, 0);
  CoinsChute2          = RPU_ReadULFromEEProm(RPU_CHUTE_2_COINS_START_BYTE, 0);
  CoinsChute3          = RPU_ReadULFromEEProm(RPU_CHUTE_3_COINS_START_BYTE, 0);
  backgroundOn         = RPU_ReadByteFromEEProm(RPU_BACKGROUND_ON) & 1;
  FreePlay             = RPU_ReadByteFromEEProm(RPU_FREEPLAY) & 1;
  ballSaves            = RPU_ReadByteFromEEProm(RPU_BALL_SAVES);
  ballSaveScore        = 100 * RPU_ReadByteFromEEProm(RPU_BALL_SAVE_SCORE);
  ballSaveNumMS        = 1000 * RPU_ReadByteFromEEProm(RPU_BALL_SAVE_NUM_SECS);
  AdjBgGain            = RPU_ReadByteFromEEProm(RPU_BG_GAIN);
  AdjSfxGain           = RPU_ReadByteFromEEProm(RPU_SFX_GAIN);
  AdjVoiceGain         = RPU_ReadByteFromEEProm(RPU_VOICE_GAIN);

  dipBank[0]           = RPU_ReadByteFromEEProm(RPU_DIP_BANK);
  dipBank[1]           = RPU_ReadByteFromEEProm(RPU_DIP_BANK + 1);
  dipBank[2]           = RPU_ReadByteFromEEProm(RPU_DIP_BANK + 2);
  dipBank[3]           = RPU_ReadByteFromEEProm(RPU_DIP_BANK + 3);
  SetupDIPVars();
  CalculateGain();
}


// #################### ADD CREDIT ####################
void AddCredit(byte Chute, byte CredperCoin, byte CoinperCred, byte creds = 0) {
  if (creds != 0) {
    Credits = Credits + creds;
  }
  else {
    WholeCredit += 1;
    if (WholeCredit < CoinperCred) {
      PlaySound(SFXChime100); // 100 Chime
    } else 
    {
      Credits += CredperCoin;
      PlaySound(SFXAddPlayer); // Energize
      WholeCredit = 0;

    }
  }
  if (Credits > 0) RPU_SetLampState(LA_CREDIT_INDICATOR, 1);
  if (Credits > dipMaxCredits) Credits = dipMaxCredits;
  RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
  RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
  RPU_SetCoinLockout((Credits < dipMaxCredits) ? false:true);
}



// #################### ADD PLAYER ####################
boolean AddPlayer(boolean resetNumPlayers = false) {
  if (Credits < 1 && !FreePlay) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= 4) return false;

  CurrentNumPlayers += 1;
  RPU_SetDisplay(CurrentNumPlayers - 1, 0, true, 2);
  TotalPlays++;
  RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, TotalPlays);

  if (!FreePlay) {
    Credits -= 1;
    if (!resetNumPlayers) PlaySound(SFXAddPlayer); // Transporter
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
    RPU_SetLampState(LA_CREDIT_INDICATOR, Credits > 0);
    RPU_SetCoinLockout((Credits < dipMaxCredits) ? false:true);
  }
  else {
      PlaySound(SFXAddPlayer); // Energize
      if (resetNumPlayers) delay(100 * SoundTimings[SFXAddPlayer][1]); // New game will stop audio
  }
  return true;
}


// #################### Scroll latest 9 switch hits through player 2, 3, and 4 displays ####################
boolean Debugger(byte hit) { 

  if (DEBUG_MODE == 1) {
    if (hit == SW_CREDIT_BUTTON) return true; // credit button does not mean to add a player! Do not execute.
    if (RPU_ReadSingleSwitchState(SW_CREDIT_BUTTON)) return false; // hold current values if game button being held.
    for (i = 0; i < 8; ++i) dispSwitch[i] = dispSwitch[i+1];
    dispSwitch[8] = hit;
    RPU_SetDisplay(1, (((unsigned long) dispSwitch[0] * 100) + dispSwitch[1]) * 100 + dispSwitch[2], true, 6);
    RPU_SetDisplay(2, (((unsigned long) dispSwitch[3] * 100) + dispSwitch[4]) * 100 + dispSwitch[5], true, 6);
    RPU_SetDisplay(3, (((unsigned long) dispSwitch[6] * 100) + dispSwitch[7]) * 100 + dispSwitch[8], true, 6);
  }
  return false;
}



// #################### LoopCount ####################
void LoopCount() { // count number of loops performed per second

  // Results: Results are highly game-and-activity-specific.
  //.         Flash Gordon was up to 1200 during low activity events, like match or bonus score, but generally around 180 - 200 during game play.
  //          Star Trek was usually around 1000 - 1200 almost all the time.

  if (DEBUG_MODE != 2) return;
  LCount += 1;
  if (CurrentTime - LoopTime > 999) {
    LoopTime = CurrentTime;
    RPU_SetDisplay(3, LCount, true, 2);
    LCount = 0;
  }
}



// #################### Double Hit Fix ####################
boolean DoubleHitFix(byte switchnum) { // Eliminate double hits, when current time between start and start + wait
  if (CurrentTime - SwitchDebounce[switchnum].start < SwitchDebounce[switchnum].wait) 
    return true;
  SwitchDebounce[switchnum].start = CurrentTime;
  return false;
}



// #################### Reset Hit Fix ####################
boolean ResetHitFix(byte switchnum) { // Eliminate switch hits caused by solenoid vibrations, when current time between start and start + wait

switch (switchnum) {
  case SW_DROP_TARGET_D_LOW:
  case SW_DROP_TARGET_C_MID_LOW:
  case SW_DROP_TARGET_B_MID_HIGH:
  case SW_DROP_TARGET_A_HIGH:
    if (CurrentTime < ResetHits[SO_DTARGET_RESET].start) return false;
    if (CurrentTime - ResetHits[SO_DTARGET_RESET].start < ResetHits[SO_DTARGET_RESET].wait) return true;
    return false;
}

return false;
}



// #################### My_PushToTimedSolenoidStack ####################
boolean My_PushToTimedSolenoidStack(byte solenoidNumber, byte numPushes, unsigned long whenToFire, boolean disableOverride = false) {
  // Call RPU version, after saving solenoid firing time in ResetHits[].start

  ResetHits[solenoidNumber].start = whenToFire;
  return RPU_PushToTimedSolenoidStack(solenoidNumber, numPushes, whenToFire, disableOverride);
}



// #################### My_PushToSolenoidStack ####################
void My_PushToSolenoidStack(byte solenoidNumber, byte numPushes, boolean disableOverride = false) {
  // Call RPU version, after saving solenoid firing time in ResetHits[].start

  ResetHits[solenoidNumber].start = CurrentTime;
  RPU_PushToSolenoidStack(solenoidNumber, numPushes, disableOverride);
}



// #################### INIT NEW BALL ####################
int InitNewBall(bool curStateChanged, int ballNum) {

  if (curStateChanged) {
    FirstBall = (ballNum == 1 && !PlayerShootsAgain);

    RPU_ClearSwitches(); // Added to clear solenoids and switches, ensure no carryover from previous game/player/ball. Added to RPU.h as well! Line 317.
    RPU_TurnOffAllLamps();
    if (Credits > 0) RPU_SetLampState(LA_CREDIT_INDICATOR, 1); // Credit light stays on
    RPU_SetDisableFlippers(false);
    RPU_EnableSolenoidStack();

    My_PushToTimedSolenoidStack(SO_DTARGET_RESET, 15, CurrentTime + 500);

    BallDrained = false;

    SkillShotHits = 0;
    SkillShotState = 0;

    PFValidated = false;

    BallInSaucer = 0;
    
    for (byte count=0; count<CurrentNumPlayers; count++) {
      RPU_SetDisplay(count, CurrentScores[count], true, 2);
    }
    DisplayValue = CurrentScores[CurrentPlayer];
    DisplayTimer = CurrentTime + 100;

    RPU_SetDisplayBallInPlay(ballNum);
    RPU_SetLampState(LA_BALL_IN_PLAY, 1);
    RPU_SetLampState(LA_1_UP + CurrentPlayer, 1);
    RPU_SetLampState(LA_TILT, 0);
    if (PlayerShootsAgain==true) {
      RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
      RPU_SetLampState(LA_SHOOT_AGAIN, 1);
    }
    InitialScore = CurrentScores[CurrentPlayer];
    chimeOff10 = false;
    chimeOff100 = false;
    chimeOff1000 = false;

    if (FirstBall) {
      SaucerBALLY[CurrentPlayer] = 0;
      for (i = 0; i < 5; ++i) 
        ScoreBALLY[CurrentPlayer][i] = 0;
    }
    HyperSpace2468 = dipHyperSpaceAdjustment; // Start at either 2K or 4K
    HyperSpace25000Paid = false;
    BonusXState[CurrentPlayer] = 1;
    PhaserBonus = 1;
    scoreChimes[0] = scoreChimes[1] = scoreChimes[2] = 0;
    if (FirstBall) PhotonBonus[CurrentPlayer] = dipBally10000Adjustment; // Start at either 10K or 25K
    if (FirstBall) LeftRight = 1;
    PhotonLightShow = 0;
    PhotonLightShowTimer  = 0;
    PhotonLightShowTimer2 = 0;
    EELightShow = 0;
    WinnerLightShow = 0;

    if (FirstBall) backgroundSound = 0;
    else if (++backgroundSound >= BackgroundMusic[0]) backgroundSound = 0;

    if (FirstBall) 
      FreeGameCollected[CurrentPlayer] =  FreeBallCollected[CurrentPlayer] = 0;

    RPU_SetLampState(LA_B_LANE,     !ScoreBALLY[CurrentPlayer][0]);
    RPU_SetLampState(LA_A_LANE,     !ScoreBALLY[CurrentPlayer][1]);
    RPU_SetLampState(LA_KLINGON_L1, !ScoreBALLY[CurrentPlayer][2]);
    RPU_SetLampState(LA_KLINGON_L2, !ScoreBALLY[CurrentPlayer][3]);
    RPU_SetLampState(LA_KLINGON_Y,  !ScoreBALLY[CurrentPlayer][4]);

    RPU_SetLampState(LA_MIDDLE_B,    ScoreBALLY[CurrentPlayer][0]);
    RPU_SetLampState(LA_MIDDLE_A,    ScoreBALLY[CurrentPlayer][1]);
    RPU_SetLampState(LA_MIDDLE_L1,   ScoreBALLY[CurrentPlayer][2]);
    RPU_SetLampState(LA_MIDDLE_L2,   ScoreBALLY[CurrentPlayer][3]);
    RPU_SetLampState(LA_MIDDLE_Y,    ScoreBALLY[CurrentPlayer][4]);

    RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);
    RPU_SetLampState(LA_HYPER_SPACE_2K + HyperSpace2468, 1);

    if (PhotonBonus[CurrentPlayer] == 4) {
      if (dipBallySpecialAdjustment)
        RPU_SetLampState(LA_MIDDLE_SPECIAL, 1);
      else
        RPU_SetLampState(LA_MIDDLE_50K, 1);
    }
    else
      RPU_SetLampState(LA_MIDDLE_10K + PhotonBonus[CurrentPlayer], 1);

    RPU_SetLampState(LA_2X + 1 - BonusXState[CurrentPlayer], 1);
    RPU_SetLampState(LA_RIGHT_SLINGSHOT_INLANE + LeftRight, 1);
    if (dipFlipperFeedAdjustment) RPU_SetLampState(LA_LEFT_SLINGSHOT_INLANE - LeftRight, 1);
    RPU_SetLampState(LA_LEFT_SLINGSHOT_OUTLANE - LeftRight, PhotonBonus[CurrentPlayer] > 2);
    if (dipOutlaneAdjustment) RPU_SetLampState(LA_RIGHT_SLINGSHOT_OUTLANE + LeftRight, PhotonBonus[CurrentPlayer] > 2);
    RPU_SetLampState(LA_PLANET_INLANE + LeftRight, 1);
    RPU_SetLampState(LA_300_OR_3000_TARGET, LeftRight || dipCenterAdjustment);

    NumTiltWarnings = 0;
    My_PushToTimedSolenoidStack(SO_OUTHOLE, 4, CurrentTime + 100);
    BallSaveStartTime = CurrentTime + 100;
  }

  if (!RPU_ReadSingleSwitchState(SW_OUTHOLE)) { // Make sure ball gets out of outhole before proceeding
    BallTimeInTrough = 0;
    if (ScoreAwardStates[CurrentPlayer] >= 2 && MiniGamePlayed[CurrentPlayer] == 0) {
      MiniGamePlayed[CurrentPlayer] = 1;
      return MACHINE_STATE_MINI_GAME_1;
    }
    else if (ScoreAwardStates[CurrentPlayer] >= 3 && MiniGamePlayed[CurrentPlayer] == 1) {
      MiniGamePlayed[CurrentPlayer] = 2;
      return MACHINE_STATE_MINI_GAME_2;
    }
    else 
      return MACHINE_STATE_SKILL_SHOT;
  } else {
    return MACHINE_STATE_INIT_NEW_BALL;
  }
}



// #################### RUN SELF TEST ####################
int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  if (curStateChanged) {
    EELightShow = WinnerLightShow = PhotonLightShow = PhaserLightShow = MatchLightShow = AttractLightShow = false;
    CurrentNumPlayers = 0;
    CurrentPlayer = 0;
    CurrentScores[0] = 0;
    BonusXState[CurrentPlayer] = 3;
  }
  PlayQueue();
  FirePhotonTorpedoes(); // lights and scoring for photon torpedo bonus
  FireAllPhasers();      // lights and scoring for phaser bonus
  if (EELightShow) EnterpriseExplodes();
  if (WinnerLightShow) WinnerLights();
  if (MatchLightShow) MatchMode(0);
  if (AttractLightShow) RunAttractMode(MACHINE_STATE_TEST_LAMPS, 0);
  scoreChimes[0] = scoreChimes[1] = scoreChimes[2] = 0;

  if (CurrentTime > MiniGamePauseTimer) EELightShow = WinnerLightShow = false;
  
  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code.
  // Any that's less is machine specific and is handled below.
  if (curState >= MACHINE_STATE_TEST_DONE) {
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_BUTTON, SW_COIN_3, SW_SLAM);
    if (returnState >= 20000) { // Fudged to play light shows in the main program!
      SelfTestLightShows(returnState - 20000);
      returnState = MACHINE_STATE_TEST_LAMPS;
    } else if (returnState >= 10000) { // Fudged to play sounds in the main program!
      StopAudio();
      PlaySound(returnState - 10000);
      returnState = MACHINE_STATE_TEST_SOUNDS;
    }
    // Quick jump to / out of kid mode: Press credit and coin 3 at same time
    if (RPU_ReadSingleSwitchState(SW_CREDIT_BUTTON) && RPU_ReadSingleSwitchState(SW_COIN_3) && (curState != MACHINE_STATE_TEST_STUCK_SWITCHES)) {
      kidMode = !kidMode;
      returnState = MACHINE_STATE_ATTRACT;
    }   
  }
  else if (curState >= MACHINE_STATE_TEST_DONE - NUM_EXTENDED_SETTINGS) { // Basic self tests are handled above. Self testing can be extended in the following code.
    returnState = RunExtendedSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_BUTTON, SW_COIN_3, SW_SLAM);
    // Quick jump to / out of kid mode: Press credit and coin 3 at same time
    if (RPU_ReadSingleSwitchState(SW_CREDIT_BUTTON) && RPU_ReadSingleSwitchState(SW_COIN_3)) {
      kidMode = !kidMode;
      returnState = MACHINE_STATE_ATTRACT;
    }
  }
  else {
    returnState = MACHINE_STATE_ATTRACT;
  }
  if (returnState == MACHINE_STATE_ATTRACT){
    
    ReadAllEEProm(); // Update all local variables with information updated in self test.
    
    StopAudio();
    SetTrackLoop(SFXTest, false);
    SetTrackLoop(VoiceTest, false);
    CalculateGain();

    // Adjustments to variables to ensure they are within max and min requirements
    if (Credits > dipMaxCredits) {
      Credits = dipMaxCredits;
      RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits & 0x000000FF);
    }
    RPU_SetLampState(LA_CREDIT_INDICATOR, Credits > 0);

    if (kidMode) {
      KidSettings();
      PlaySound(VoiceKidsMode[random(1, 1 + VoiceKidsMode[0])], 500);
    } 
    else {
      // EEProm values already set above
      // Reset operator game adjustments that are changed in KidSettings():
      //  (currently none!)
      PlaySound(VoiceAttract[random(1, 1 + VoiceAttract[0])]); 
    }
    SetLastSelfTestChangedTime(CurrentTime); // Ensure self-test switch not hit on entering attract mode from self-test!
    RPU_ClearSwitches();
  }
  return returnState;
}



// #################### RUN ATTRACT MODE ####################
int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  // If this is the first time in the attract mode loop
  if (curStateChanged) {
    AttractCalloutTimer = CurrentTime - ATTRACT_SPEECH_TIMER + 10000;
    RPU_TurnOffAllLamps();
    if (Credits > 0) RPU_SetLampState(LA_CREDIT_INDICATOR, 1); // Credit light stays on
    RPU_DisableSolenoidStack();
    RPU_SetDisableFlippers(true);
    RPU_SetCoinLockout((Credits < dipMaxCredits) ? false:true);
    
    SaucerBALLY[0] = 4;
    HyperSpace2468 = 5;
    BonusXState[0] = 3;
    PhaserBonus = MAX_PHASER_BONUS;
    PhotonBonus[0] = 3;
    LeftRight = 1;

    RPU_SetLampState(LA_A_LANE, 1);
    RPU_SetLampState(LA_B_LANE, 1);
    RPU_SetLampState(LA_KLINGON_L1, 1);
    RPU_SetLampState(LA_KLINGON_L2, 1);
    RPU_SetLampState(LA_KLINGON_Y, 1);
    // RPU_SetLampState(LA_2X, 1);
    RPU_SetLampState(LA_GAME_OVER, 1);

    if (!FreePlay)
      RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
    else
      RPU_SetDisplayCredits(Credits, false);

    if (dipMatch) {
      RPU_SetLampState(LA_MATCH, 1);
      RPU_SetDisplayBallInPlay((int)MatchDigit, 1);
    }
    else
      RPU_SetDisplayBallInPlay(0, 1);

    AttractDisplayTimer = CurrentTime;
    
    AttractHeadMode = 255;
    AttractPlayfieldMode = 255;
    if (CurrentScores[0] == 0) CurrentScores[0] = 200000 + 10 * random(-5000, 5000); // random initial score for display

    kidTimer = 0;
    kidSwitch = 0xFF;

    SaucerHitTime = 0;
  }

  PlayQueue();

  if (CurrentTime >= AttractCalloutTimer + ATTRACT_SPEECH_TIMER + AttractWait) {
    if (kidMode)
      PlaySound(VoiceKidsMode[random(1, 1 + VoiceKidsMode[0])]); 
    else
      PlaySound(VoiceAttract[random(1, 1 + VoiceAttract[0])]); 
    AttractCalloutTimer = CurrentTime;
    AttractWait = 0;
  }

  // Run attract lamps display
  if (CurrentTime >= AttractDisplayTimer) {
    AttractDisplayTimer = CurrentTime + 750;

    RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[0], 0);
    RPU_SetLampState(BALLYMiddle[SaucerBALLY[0]], 0);
    RPU_SetLampState(LA_HYPER_SPACE_2K + HyperSpace2468, 0);
    RPU_SetLampState(LA_2X + 1 - BonusXState[0], 0);
    RPU_SetLampState(LA_MIDDLE_10K + PhotonBonus[0], 0);
    RPU_SetLampState(LA_RIGHT_SLINGSHOT_INLANE + LeftRight, 0);
    RPU_SetLampState(LA_LEFT_SLINGSHOT_OUTLANE - LeftRight, 0);
    RPU_SetLampState(LA_PLANET_INLANE + LeftRight, 0);

    if (++SaucerBALLY[0] > 4) SaucerBALLY[0] = 0;
    if (++HyperSpace2468 > 5) HyperSpace2468 = 0;
    if (++BonusXState[0] > 3) BonusXState[0] = 1;
    if (++PhaserBonus > MAX_PHASER_BONUS) PhaserBonus = 1;
    if (++PhotonBonus[0] > 3) PhotonBonus[0] = 0;
    LeftRight = !LeftRight;

    ShowPhaserBonusOnLadder(PhaserBonus);
    RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[0], 1);
    RPU_SetLampState(BALLYMiddle[SaucerBALLY[0]], 1);
    RPU_SetLampState(LA_HYPER_SPACE_2K + HyperSpace2468, 1);
    RPU_SetLampState(LA_2X + 1 - BonusXState[0], 1);
    RPU_SetLampState(LA_MIDDLE_10K + PhotonBonus[0], 1);
    RPU_SetLampState(LA_RIGHT_SLINGSHOT_INLANE + LeftRight, 1);
    RPU_SetLampState(LA_LEFT_SLINGSHOT_OUTLANE - LeftRight, 1);
    RPU_SetLampState(LA_PLANET_INLANE + LeftRight, 1);
    RPU_SetLampState(LA_300_OR_3000_TARGET, LeftRight);
  }

  if (curState == MACHINE_STATE_TEST_LAMPS) return returnState; // self-test lamp test ends here!

  if ((CurrentTime / 6500) % 2 == 1) { // Alternate displays between high score and latest player scores (reversed order, player scores first)
    if (AttractHeadMode!=1) { // do this once, at the beginning
      RPU_SetLampState(LA_HIGH_SCORE_TO_DATE, 1);
      if (!FreePlay) RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
      if (!dipMatch) RPU_SetDisplayBallInPlay(0, true);
    }
    AttractHeadMode = 1;
      SetDisplayAttract(HighScore, HighScore, HighScore, HighScore, CurrentTime - AttractStartAnimation);
  } else {
    if (AttractHeadMode!=2) { // do this once, at the beginning
      RPU_SetLampState(LA_HIGH_SCORE_TO_DATE, 0);
      if (!FreePlay) RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
      if (!dipMatch) RPU_SetDisplayBallInPlay(0, true);
    }
    AttractHeadMode = 2;
      if (CurrentScores[0] == 0)
        SetDisplayAttract(HighScore, HighScore, HighScore, HighScore, CurrentTime - AttractStartAnimation);
      else
        SetDisplayAttract(CurrentScores[0], CurrentScores[1], CurrentScores[2], CurrentScores[3], CurrentTime - AttractStartAnimation);
  }

  if ((CurrentTime - kidTimer > 200) && (kidSwitch != 0xFF)){                                  // Check if credit button and coin 3 button pressed at same time
    if (RPU_ReadSingleSwitchState(SW_CREDIT_BUTTON) && RPU_ReadSingleSwitchState(SW_COIN_3)) { // if yes, change kid mode, add credit, add player
      kidMode = !kidMode;
      if (!kidMode) {
        ReadAllEEProm(); // Get non-kid settings from memory
        // Replace all changed settings from Operator Game Adjustments!
        //  (None currently)
      }
      else {
        KidSettings();
      }
      if (!FreePlay) {
        AddCredit(2, CreditsperCoin[2], CoinsperCredit[2]);
        RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
        CoinsChute3 += 1;
        RPU_WriteULToEEProm(RPU_CHUTE_3_COINS_START_BYTE, CoinsChute3);
      } 

      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    else if (kidSwitch == SW_CREDIT_BUTTON) {                                                  // if just credit button, just add player
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    else if (kidSwitch == SW_COIN_3) {                                                         // if just coin 3, just add credit
      if (!FreePlay) {
        AddCredit(2, CreditsperCoin[2], CoinsperCredit[2]);
        RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
        CoinsChute3 += 1;
        RPU_WriteULToEEProm(RPU_CHUTE_3_COINS_START_BYTE, CoinsChute3);
      }
    }
    kidTimer = 0;
    kidSwitch = 0xFF;
  }

  while ( (switchHit=RPU_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {
    if (switchHit==SW_CREDIT_BUTTON) {
      kidTimer = CurrentTime;
      kidSwitch = SW_CREDIT_BUTTON;
    }

    if (switchHit==SW_COIN_1) {
      if (!FreePlay) {
        AddCredit(0, CreditsperCoin[0], CoinsperCredit[0]);
        RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
        CoinsChute1 += 1;
        RPU_WriteULToEEProm(RPU_CHUTE_1_COINS_START_BYTE, CoinsChute1);
      }
    }
    if (switchHit==SW_COIN_2) {
      if (!FreePlay) {
        AddCredit(1, CreditsperCoin[1], CoinsperCredit[1]);
        RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
        CoinsChute2 += 1;
        RPU_WriteULToEEProm(RPU_CHUTE_2_COINS_START_BYTE, CoinsChute2);
      }
    }
    if (switchHit==SW_COIN_3) {
      kidTimer = CurrentTime;
      kidSwitch = SW_COIN_3;
    }
    if (switchHit==SW_SELF_TEST_SWITCH && (CurrentTime-GetLastSelfTestChangedTime())>5000) { // Needs l o n g delay for some reason
      returnState = MACHINE_STATE_TEST_LAMPS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }
  if (RPU_ReadSingleSwitchState(SW_SAUCER) && CurrentTime > SaucerHitTime + 750) { // If ball sitting in saucer, kick it out
    My_PushToTimedSolenoidStack(SO_SAUCER, 5, CurrentTime + 500, true);
    SaucerHitTime = CurrentTime;
  }

  return returnState;
}



// #################### ATTRACT DISPLAY ####################
void SetDisplayAttract(unsigned long score1, unsigned long score2, unsigned long score3, unsigned long score4, unsigned long time) {
	unsigned long scores[4] = {score1, score2, score3, score4};
	unsigned long divisor[8] = {10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};

	byte Quarter = (time % 6500) / 125;
  if (Quarter == HoldQuarter) return; // not quarters anymore. Eighths. Oh well...
  HoldQuarter = Quarter;
	for (byte i = 0; i < 4; ++i) {
		if (scores[i] == 0) {
			RPU_SetDisplay(i, 0);
			RPU_SetDisplayBlank(i, 0);
		} else {
			if (Quarter < 8) RPU_SetDisplay(i, scores[i] / divisor[Quarter], true, 0); // scroll left
      else
      if (Quarter >=40 && Quarter < 48) RPU_SetDisplay(i, scores[i] / divisor[47 - Quarter], true, 0); // scroll right
		else
			if (Quarter == 16 || Quarter == 17 || Quarter == 20 || Quarter == 21 || Quarter == 24 || Quarter == 25 || Quarter == 28 || Quarter == 29 || Quarter >= 48) { // Flash off
        RPU_SetDisplay(i, 0);
				RPU_SetDisplayBlank(i, 0);
      }
			else
				RPU_SetDisplay(i, scores[i], true, 1);
		}
	}
	return;
}



// #################### ADD TO SCORE ####################
void AddToScore(unsigned long scoreAmount, boolean noChime = false, byte speed = 100) {
  unsigned long hold;

  CurrentScores[CurrentPlayer] += scoreAmount;

  hold = scoreAmount % 100;
  scoreChimes[0] += hold / 10;
  scoreAmount -= hold;
  if (hold!= 0) chimeOff10 = noChime;

  hold = scoreAmount % 1000;
  scoreChimes[1] += hold / 100;
  if (hold!= 0) chimeOff100 = noChime;

  scoreChimes[2] += (scoreAmount - hold) / 1000;
  if (scoreAmount != hold) chimeOff1000 = noChime;

  ChimeSpeed = speed;
}



// #################### DISPLAY COUNTS UP TO CURRENT SCORE ####################
void CountingDisplay(int displayNumber, unsigned long value, boolean blankByMagnitude, byte minDigits, boolean alwaysShow = false) {
  
  if (displayNumber != CurrentPlayer) {
    RPU_SetDisplay(displayNumber, value, blankByMagnitude, minDigits);
    return;
  }
  if (DisplayValue == value && !alwaysShow) return;
  if (CurrentTime < DisplayTimer) return;

  if (scoreChimes[0] != 0) {
    RPU_SetDisplay(displayNumber, DisplayValue += 10, blankByMagnitude, minDigits);
    --scoreChimes[0];
    if (!chimeOff10) PlaySound(SFXChime10);
  }
  else if (scoreChimes[1] != 0) {
    RPU_SetDisplay(displayNumber, DisplayValue += 100, blankByMagnitude, minDigits);
    --scoreChimes[1];
    if (!chimeOff100) PlaySound(SFXChime100);
  }
  else if (scoreChimes[2] != 0) {
    RPU_SetDisplay(displayNumber, DisplayValue += 1000, blankByMagnitude, minDigits);
    --scoreChimes[2];
  if (!chimeOff1000) PlaySound(SFXChime1000);
  }
  else RPU_SetDisplay(displayNumber, DisplayValue, blankByMagnitude, minDigits);

  if (scoreChimes[0] == 0) chimeOff10 = false;
  if (scoreChimes[1] == 0) chimeOff100 = false;
  if (scoreChimes[2] == 0) chimeOff1000 = false;

  DisplayTimer = CurrentTime + ChimeSpeed;
}



// #################### FLASHING DISPLAY COUNTS UP TO CURRENT SCORE ####################
void CountingFlashDisplay(int displayNumber, unsigned long value, unsigned long curTime, int period, byte minDigits) {
  // A period of zero toggles display every other time
  if (period) {
    if ((curTime/period)%2) {
      CountingDisplay(displayNumber, value, true, minDigits, true);
    } else {
      RPU_SetDisplayBlank(displayNumber, 0);
    }
  }
}


// #################### SET DISPLAY TO CURRENT VALUE, ZERO COUNTING AMOUNTS ####################
void ResetCountingDisplay() {
    RPU_SetDisplay(CurrentPlayer, DisplayValue = CurrentScores[CurrentPlayer], true, 2);
    for (i = 0; i < 3; ++i)
      scoreChimes[i] = 0;
}



// #################### LIGHTSHOW FOR ENTERPRISE EXPLODING ####################
void EnterpriseExplodes() {
  unsigned long timer = CurrentTime - (MiniGamePauseTimer - 100 * (unsigned long) SoundTimings[SFXEnterpriseExplodes][1]);
  if (timer <= 1100) {                 // Lights explode outward
    for (i = 0; i < 60; ++i)
      if (timer * 254 / 1100 >= LampPlacement[i].distance) // 250 should be used here and below, except an odd timing issue required me to use 254.
          RPU_SetLampState(i, 1);
  } else if (timer <= 2200) {          // Lights off outward
    for (i = 0; i < 60; ++i)
      if ((timer - 1100) * 254 / 1100 >= LampPlacement[i].distance)
          RPU_SetLampState(i, 0);
  } else if (timer <= 2800) {          // Secondary explosion outward, including backglass
    for (i = 0; i < 60; ++i)
      if ((timer - 2200) * 254 / 600 >= LampPlacement[i].distance || LampPlacement[i].distance == 255)
          RPU_SetLampState(i, 1);
  } else if (timer <= 3000) {          // Remain 100% lit
    // wait...
  } else if (timer <= 4200) {          // lights off inward
    for (i = 0; i < 60; ++i)
      if (254 - (timer - 3000) * 254 / 1200 <= LampPlacement[i].distance || LampPlacement[i].distance == 255)
          RPU_SetLampState(i, 0);
  }
}



// #################### LIGHTSHOW FOR MINI GAME WINNER ####################
void WinnerLights() {
  unsigned long timer = CurrentTime - (MiniGamePauseTimer - 100 * SoundTimings[SFXSkillShot][1]);
  if (timer > 3500) timer -= 3500; // To be executed twice in 7 seconds
  if (timer < 1750) {         // Lights up
    timer = timer * 300 / 1750;
    for (i = 0; i < 60; ++i) {
      RPU_SetLampState(i, 0);
        if (LampPlacement[i].vertical <= timer && LampPlacement[i].vertical >= timer - 50)
          RPU_SetLampState(i, 1);
    }
  } else {                   // Lights down
    timer = (timer - 1750) * 300 / 1750;
    for (i = 0; i < 60; ++i) {
      RPU_SetLampState(i, 0);
        if (LampPlacement[i].vertical <= 300 - timer && LampPlacement[i].vertical >= 250 - timer)
          RPU_SetLampState(i, 1);
    }
  }
}



// #################### LIGHTS AND SOUND FOR PHOTON TORPEDOES ####################
void FirePhotonTorpedoes() {
  if (PhotonLightShow && CurrentTime > PhotonLightShowTimer2) { // Handle light show
    // if (PhotonScore != 0) RPU_SetDisplayFlashCredits(CurrentTime, 400);
    switch (PhotonStep2) {
      case 0: 
        ShowPhaserBonusOnLadder(0);
        RPU_SetLampState(BALLYMiddle[0], 1);
        RPU_SetLampState(BALLYMiddle[1], 1);
        RPU_SetLampState(BALLYMiddle[2], 1);
        RPU_SetLampState(BALLYMiddle[3], 1);
        RPU_SetLampState(BALLYMiddle[4], 1);
        ShowPhaserBonusOnLadder(0);
        PhotonStep2 += 1;
        PhotonLightShowTimer2 += 400;
        break;

      case 1: 
        RPU_SetLampState(BALLYMiddle[0], 0);
        RPU_SetLampState(BALLYMiddle[1], 1);
        RPU_SetLampState(BALLYMiddle[2], 1);
        RPU_SetLampState(BALLYMiddle[3], 1);
        RPU_SetLampState(BALLYMiddle[4], 0);
        PhotonStep2 += 1;
        PhotonLightShowTimer2 += 400;
        break;

      case 2: 
        RPU_SetLampState(BALLYMiddle[0], 0);
        RPU_SetLampState(BALLYMiddle[1], 0);
        RPU_SetLampState(BALLYMiddle[2], 1);
        RPU_SetLampState(BALLYMiddle[3], 0);
        RPU_SetLampState(BALLYMiddle[4], 0);
        PhotonStep2 += 1;
        PhotonLightShowTimer2 += 400;
        break;

      case 3: 
        RPU_SetLampState(BALLYMiddle[0], 0);
        RPU_SetLampState(BALLYMiddle[1], 0);
        RPU_SetLampState(BALLYMiddle[2], 0);
        RPU_SetLampState(BALLYMiddle[3], 0);
        RPU_SetLampState(BALLYMiddle[4], 0);
        RPU_SetLampState(LA_BONUS_20K, 1);
        PhotonStep2 += 1;
        PhotonLightShowTimer2 += 400;
        break;
        
      case 4:
        PhotonLightShowTimer2 += 75;
        PhotonStep2 += 1;
        break;
 
      case 5:
      // does not actually run! PhotonStep2 increased twice :)
      case 6: 
        RPU_SetLampState(BALLYMiddle[0], 0);
        RPU_SetLampState(BALLYMiddle[1], 0);
        RPU_SetLampState(BALLYMiddle[2], 0);
        RPU_SetLampState(BALLYMiddle[3], 0);
        RPU_SetLampState(BALLYMiddle[4], 0);
        break;

      case 38:
        ShowPhaserBonusOnLadder(PhaserBonus);
        PhotonLightShow = false;
        RPU_SetLampState(BALLYMiddle[0], ScoreBALLY[CurrentPlayer][0]);
        RPU_SetLampState(BALLYMiddle[1], ScoreBALLY[CurrentPlayer][1]); 
        RPU_SetLampState(BALLYMiddle[2], ScoreBALLY[CurrentPlayer][2]); 
        RPU_SetLampState(BALLYMiddle[3], ScoreBALLY[CurrentPlayer][3]); 
        RPU_SetLampState(BALLYMiddle[4], ScoreBALLY[CurrentPlayer][4]); 

        if (!FreePlay) RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
        return;
    }
    if (PhotonStep2 > 4) { // Send photon torpedo down the bonus ladder
        PhotonNumber = (PhotonStep2 - 5) % 11;
        RPU_SetLampState(LA_BONUS_20K - PhotonNumber, 1);
        if (PhotonNumber != 0)
          RPU_SetLampState(LA_BONUS_20K - PhotonNumber + 1, 0);
        else
          RPU_SetLampState(LA_BONUS_1K, 0);
        PhotonLightShowTimer2 += 75;
        PhotonStep2 += 1;
    }
  }
  if (PhotonLightShow && CurrentTime > PhotonLightShowTimer) { // Handle scoring and indicator lights
    switch (PhotonStep) {
          case 8:
          case 6:
          case 4: // Score one fifth of total award, five times
          case 2:
          case 0:
            AddToScore(PhotonScore, true, 10);
            if (PhotonScore != 0) RPU_SetDisplayCredits(PhotonScore / 200);
            PhotonStep += 1;
            PhotonLightShowTimer += 400;
            break;

          case 9:
            if ((PhotonBonus[CurrentPlayer] == 3 || (PhotonBonus[CurrentPlayer] == 4 && dipBallySpecialAdjustment)) 
                  && dipThresholdAward == 2 && !PlayerShootsAgain) {
              PlayerShootsAgain = true;
              FreeBallCollected[CurrentPlayer] = true;
              RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
              RPU_SetLampState(LA_SHOOT_AGAIN, 1);
            }
            if ((PhotonBonus[CurrentPlayer] == 3 || (PhotonBonus[CurrentPlayer] == 4 && dipBallySpecialAdjustment)) 
                  && dipThresholdAward == 3 && !FreeGameCollected[CurrentPlayer]) {
              AddCredit(0, CreditsperCoin[0], CoinsperCredit[0]);
              My_PushToSolenoidStack(SO_KNOCKER, 3);
              FreeGameCollected[CurrentPlayer] = true;
            }
          if (PhotonBonus[CurrentPlayer] < 3) {
            RPU_SetLampState(LA_MIDDLE_10K + PhotonBonus[CurrentPlayer], 0);
            ++PhotonBonus[CurrentPlayer];
            RPU_SetLampState(LA_MIDDLE_10K + PhotonBonus[CurrentPlayer], 1);
          } else {
            PhotonBonus[CurrentPlayer] = 4;
            if (!dipBallySpecialAdjustment) {
              FreeBallCollected[CurrentPlayer] = true; // Free ball no longer available. Special no longer lit.
              RPU_SetLampState(LA_MIDDLE_SPECIAL, 0);
              RPU_SetLampState(LA_MIDDLE_50K, 1);
            }
          }
            // for (i = 0; i < 5; ++i) {
            //   ScoreBALLY[CurrentPlayer][i] = false;                                         // Reset letters to off (not scored)
            //   RPU_SetLampState(LA_B_LANE + i, 1);                                           // Turn on all target letter lights
            // }
            RPU_SetLampState(LA_LEFT_SLINGSHOT_OUTLANE - LeftRight, PhotonBonus[CurrentPlayer] > 2);
            if (dipOutlaneAdjustment) RPU_SetLampState(LA_RIGHT_SLINGSHOT_OUTLANE + LeftRight, PhotonBonus[CurrentPlayer] > 2);
            // PhotonLightShow = false;
            PhotonStep = 0;
          case 7:
          case 5:
          case 3:
          case 1:
            if (PhotonScore != 0) RPU_SetDisplayCredits(PhotonScore / 200, false);
            PhotonStep += 1;
            PhotonLightShowTimer += 400;
            break;
    }
  }
}



// #################### LIGHTS AND SOUND FOR PHASERS ####################
void FireAllPhasers() {
  if (PhaserLightShow)  RPU_SetDisplayFlashCredits(CurrentTime, 200);
  if (PhaserLightShow && CurrentTime > PhaserLightShowTimer) {                    // Handle light show
    if (CurrentTime < FireDelayTimer && PhaserStep != 0) return;                 // Motion of lights is spaced out by 15ms
    FireDelayTimer = CurrentTime + 15;

    switch (PhaserStep) {
      case 0: 
        ShowPhaserBonusOnLadder(0);
        AddToScore(1000 * (unsigned long) PhaserBonus, true, min(255, (int) 1300 / (PhaserBonus == 1 ? 1 : PhaserBonus - 1)));
        LampCounter = LA_BONUS_20K;
        PhaserStep += 1;

      case 1:
        RPU_SetLampState(LampCounter, 1);
        LampCounter--;
        if (LampCounter >= LA_BONUS_1K) return;

        LampCounter = LA_BONUS_20K;
        PhaserStep += 1;
        PhaserLightShowTimer += 1300;
        break;

      case 2:
        RPU_SetLampState(LampCounter, 0);
        LampCounter--;
        if (LampCounter >= LA_BONUS_1K) return;

        LampCounter = LA_BONUS_20K;
        PhaserStep += 1;
        PhaserLightShowTimer += 200;
        if (BonusXState[CurrentPlayer] == 1) 
          PhaserStep = 7;
        else 
          AddToScore(1000 * (unsigned long) PhaserBonus, true, min (255, (int) 1300 / (PhaserBonus == 1 ? 1 : PhaserBonus - 1)));
        break;

      case 3:
        RPU_SetLampState(LampCounter, 1);
        LampCounter--;
        if (LampCounter >= LA_BONUS_1K) return;

        LampCounter = LA_BONUS_20K;
        PhaserStep += 1;
        PhaserLightShowTimer += 1300;
        break;

      case 4:
        RPU_SetLampState(LampCounter, 0);
        LampCounter--;
        if (LampCounter >= LA_BONUS_1K) return;

        LampCounter = LA_BONUS_20K;
        PhaserStep += 1;
        PhaserLightShowTimer += 200;
        if (BonusXState[CurrentPlayer] == 2) 
          PhaserStep = 7;
        else 
          AddToScore(1000 * (unsigned long) PhaserBonus, true, min (255, (int) 1300 / (PhaserBonus == 1 ? 1 : PhaserBonus - 1)));
        break;

      case 5:
        RPU_SetLampState(LampCounter, 1);
        LampCounter--;
        if (LampCounter >= LA_BONUS_1K) return;

        LampCounter = LA_BONUS_20K;
        PhaserStep += 1;
        PhaserLightShowTimer += 1300;
        break;

      case 6:
        RPU_SetLampState(LampCounter, 0);
        LampCounter--;
        if (LampCounter >= LA_BONUS_1K) return;
        
        PhaserStep += 1;
        PhaserLightShowTimer += 200;
        break;

      case 7:
        ShowPhaserBonusOnLadder(PhaserBonus);
        PhaserLightShow = false;
        RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
        break;
    }
  }
}



// #################### NORMAL GAMEPLAY ####################
int NormalGamePlay(boolean curStateChanged) {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  if (curStateChanged) {
    ShowPhaserBonusOnLadder(PhaserBonus);
     if (PhotonLightShow == true) {  // Timing issues on switch from skill shot to normal game play
       SaucerHitTime = CurrentTime;
       PhotonLightShowTimer = PhotonLightShowTimer2 = CurrentTime - 200;
     }
  }
  
  // HANDLE SAUCER
  if (BallInSaucer) {
    if (CurrentTime - SaucerHitTime > (PhotonLightShow ? 5000 : 1700)) { // length of photon torpedo message
      My_PushToSolenoidStack(SO_SAUCER, 5);
      BallInSaucer = false;
    }
  }
  
  FirePhotonTorpedoes(); // lights and scoring for photon torpedo bonus
  FireAllPhasers();      // lights and scoring for phaser bonus

  CountingDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], true, 2);

  // Check to see if ball is in the outhole
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (BallTimeInTrough==0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least 
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime-BallTimeInTrough)>500) {
        BallDrained = true;
        if (NumTiltWarnings>=MAX_TILT_WARNINGS || !PFValidated)
            returnState = MACHINE_STATE_BALL_OVER;
        else
            returnState = MACHINE_STATE_COUNTDOWN_BONUS;
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  return returnState;
}



// #################### INIT GAMEPLAY ####################
int InitGamePlay(boolean curStateChanged) {
  int returnState = MACHINE_STATE_INIT_GAMEPLAY;

  if (curStateChanged) {
    RPU_SetCoinLockout((Credits < dipMaxCredits) ? false:true);
    RPU_SetDisableFlippers(true);
    RPU_DisableSolenoidStack();
    RPU_TurnOffAllLamps();
    if (Credits > 0) RPU_SetLampState(LA_CREDIT_INDICATOR, 1); // Credit light stays on
    RPU_SetDisplayBallInPlay(0);
    RPU_EnableSolenoidStack();
    My_PushToSolenoidStack(SO_SAUCER, 5, true);

    // Set up general game variables
    CurrentNumPlayers = 1;
    CurrentPlayer = 0;
    CurrentBallInPlay = 1;
    PlayerShootsAgain = false;
    HyperSpace25000Paid = false;

    StopAudio();
    AttractWait = 0;

    for (byte i = 0; i < 4; i++) {
         ScoreAwardStates[i] = 0;
           MiniGamePlayed[i] = 0;
        FreeGameCollected[i] = 0;
        FreeBallCollected[i] = 0;
            CurrentScores[i] = 0;
             BallSaveUsed[i] = ballSaves;
            RPU_SetDisplay(i, 0);
       RPU_SetDisplayBlank(i, 0);
    }
  }
  RPU_SetDisplayBlank(0, 0x60); // First player display on

  RPU_EnableSolenoidStack();
  RPU_SetDisableFlippers(false);
  returnState = MACHINE_STATE_INIT_NEW_BALL;

  return returnState;
}


// #################### BALL OVER ####################
int BallOver(boolean curStateChanged) {
  int returnState = MACHINE_STATE_BALL_OVER;
  ResetCountingDisplay();

  if (CurrentScores[CurrentPlayer] == InitialScore && NumTiltWarnings < MAX_TILT_WARNINGS) // Ball has fallen back into outhole
    return MACHINE_STATE_INIT_NEW_BALL;

  // if we have remaining ball saves, and we're under the time limit, then save the ball
  if ((BallSaveUsed[CurrentPlayer] > 0) && (CurrentTime < (BallSaveStartTime + ballSaveNumMS)) && !PlayerShootsAgain) {
    BallSaveUsed[CurrentPlayer]--;
    RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
    RPU_SetLampState(LA_SHOOT_AGAIN, 1);
    PlayerShootsAgain = true;
    CurrentScores[CurrentPlayer] = InitialScore;
    PlaySound(VoiceBallSave, 500);
    ballSavePlayed = true;
  }
    // if we have remaining ball saves, and we're under the score limit, then save the ball
  else if ((BallSaveUsed[CurrentPlayer] > 0) && (CurrentScores[CurrentPlayer] < InitialScore + ballSaveScore) && !PlayerShootsAgain) {
    BallSaveUsed[CurrentPlayer]--;
    RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
    RPU_SetLampState(LA_SHOOT_AGAIN, 1);
    PlayerShootsAgain = true;
    CurrentScores[CurrentPlayer] = InitialScore;
    PlaySound(VoiceBallSave, 500);
    ballSavePlayed = true;
  }

  if (ScoreAwardStates[CurrentPlayer] >= 2 && MiniGamePlayed[CurrentPlayer] == 0) { // In case threshold passed with saved ball and mini-game not awarded
    RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
    RPU_SetLampState(LA_SHOOT_AGAIN, 1);
    PlayerShootsAgain = true;
  }
  if (ScoreAwardStates[CurrentPlayer] >= 3 && MiniGamePlayed[CurrentPlayer] == 1) { // In case threshold passed with saved ball and mini-game not awarded
    RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
    RPU_SetLampState(LA_SHOOT_AGAIN, 1);
    PlayerShootsAgain = true;
  }

  if (PlayerShootsAgain) {
    returnState = MACHINE_STATE_INIT_NEW_BALL;
  } else {
    CurrentPlayer+=1;
    if (CurrentPlayer>=CurrentNumPlayers) {
      CurrentPlayer = 0;
      CurrentBallInPlay+=1;
    }
      
    if (CurrentBallInPlay > dipBallsPerGame) {
      for (byte count=0; count<CurrentNumPlayers; count++) {
        RPU_SetDisplay(count, CurrentScores[count], true, 2);
      }
      DisplayValue = CurrentScores[CurrentPlayer];
      if (NumTiltWarnings >= MAX_TILT_WARNINGS) {
      StopAudio(); // Stop all sounds.
      PlaySound(VoiceTiltEnd); // He's dead Jim. Only used if tilt on last ball.
      }
      returnState = MACHINE_STATE_MATCH_MODE;
    }
    else returnState = MACHINE_STATE_INIT_NEW_BALL;
  }
  return returnState;
}


// #################### MATCH MODE ####################
int MatchMode(boolean curStateChanged) {
  int returnState = MACHINE_STATE_MATCH_MODE;
      
  if (curStateChanged) {
    StopAudio(); // Clear out any queued sounds
    ResetCountingDisplay();
    highestScore = 0; // Take care of high score award
    for (byte count=0; count<CurrentNumPlayers; count++) {
      if (CurrentScores[count]>highestScore) highestScore = CurrentScores[count];
    }
  if (highestScore > HighScore){
    HighScore = highestScore;
    if (dipHighScoreAward > 0) {
      AddCredit(0, CreditsperCoin[0], CoinsperCredit[0], dipHighScoreAward);
      for (byte i = 0; i < dipHighScoreAward; i++) {
        My_PushToTimedSolenoidStack(SO_KNOCKER, 3, CurrentTime + 250 * i, true);
      }
    }
    if (!kidMode){ // High score info not saved in kid mode
      TotalReplays += dipHighScoreAward;
      RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, TotalReplays);
      HighScoreBeat = HighScoreBeat + 1;
      RPU_WriteByteToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, HighScoreBeat);
      RPU_WriteULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, HighScore);
    }
    for (byte count=0; count<CurrentNumPlayers; count++) {
      RPU_SetDisplay(count, CurrentScores[count], true, 2);
      }
      DisplayValue = CurrentScores[CurrentPlayer];
    }
  }

  if (dipMatch) {
    returnState = ShowMatchSequence(curStateChanged); 
  } else {
    StopAudio();
    if (PersonalGoalOn) {
        if (highestScore < PersonalGoal)
          PlaySound(VoiceLose[random(1, 1 + VoiceLose[0])]); 
        else
          PlaySound(VoiceWin[random(1, 1 + VoiceWin[0])]); 
    }
    else
      PlaySound(VoiceLose[random(1, 1 + VoiceLose[0])]);;

    // Wait 10 seconds to avoid final sound clobbering first attract sound
    AttractCalloutTimer = CurrentTime - ATTRACT_SPEECH_TIMER;
    AttractWait = 10000;
    returnState = MACHINE_STATE_ATTRACT;
  }
  return returnState;
}



// #################### SHOW MATCH SEQUENCE ####################
int ShowMatchSequence(boolean curStateChanged) {  
  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 300;
    NumMatchSpins = 0;
    RPU_SetLampState(LA_MATCH, 1, 0);
    RPU_SetDisableFlippers();
    ScoreMatches = 0;
    RPU_SetLampState(LA_BALL_IN_PLAY, 0);

    finalMatch[1] = Credits % 10;
    finalMatch[0] = (Credits - finalMatch[1]) / 10;
    finalMatch[3] = 0;
    finalMatch[2] = random(0, 10);
    MatchDigit = 10 * finalMatch[2];

    workingMatch[0] = workingMatch[1] = workingMatch[2] = workingMatch[3] = 0;

    StopAudio();
    PlaySound(VoiceMatch); // Computer: Working...
  }

  if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) { 
    if (NumMatchSpins < 14) { // Flash as long as "Computer: Working..." sound plays
      for (i = 0; i < 60; ++i)
        RPU_SetLampState(i, 0 == random(0, 3)); // 1960s computer! Lots of flashing lights!
    }
    else if (NumMatchSpins == 14) {
      RPU_TurnOffAllLamps();
      RPU_SetLampState(LA_MATCH, 1);
    }


    RPU_SetLampState(LA_MATCH, 1);

    RPU_SetDisplayCredits(10 * workingMatch[0] + workingMatch[1], true, true);
    RPU_SetDisplayBallInPlay(10 * workingMatch[2] + workingMatch[3], true, true);

    workingMatch[0] = workingMatch[1];
    workingMatch[1] = workingMatch[2];
    workingMatch[2] = workingMatch[3];

    if (NumMatchSpins < 9)
      workingMatch[3] = random(0, 10);
    
    else if (NumMatchSpins < 13) {
      workingMatch[3] = finalMatch[NumMatchSpins - 9];

      if ((CurrentNumPlayers > (NumMatchSpins - 9)) && ((CurrentScores[NumMatchSpins-9] % 100) == MatchDigit)) {
        ScoreMatches |= (1<<(NumMatchSpins-9));
        AddCredit(0, CreditsperCoin[0], CoinsperCredit[0], 1);
        TotalReplays += 1;
        RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, TotalReplays);
        My_PushToTimedSolenoidStack(SO_KNOCKER, 3, CurrentTime + 5 * MatchDelay, true);
        RPU_SetLampState(LA_MATCH, 1);
      }
    }

    // Play end sound
    else if (NumMatchSpins==13) {
      // ScoreMatches = 15; // Line to test successful match. Comment out of final code!
      if (PersonalGoalOn) {
        unsigned long highestScore = 0;
        for (byte count=0; count<CurrentNumPlayers; count++) 
            if (CurrentScores[count]>highestScore) highestScore = CurrentScores[count];
            
        if (highestScore < PersonalGoal)
          PlaySound(VoiceLose[random(1, 1 + VoiceLose[0])], 2 * MatchDelay); 
        else
          PlaySound(VoiceWin[random(1, 1 + VoiceWin[0])], 2 * MatchDelay); 
      }
      MatchDelay = (ScoreMatches == 0 ? MatchDelay : 4000);
    }
    // End of game, return to attract mode
    else if (NumMatchSpins == 14) {
      for (byte count=0; count<4; count++) {
        if ((ScoreMatches>>count)&0x01) // If this score matches, put back the last two digits
          RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) | 0x30); // 0x30 110000 // 0x60 1100000
      }
      // Delay attract sound 10 seconds to avoid having it clobbered by end sound
      AttractCalloutTimer = CurrentTime - ATTRACT_SPEECH_TIMER;
      AttractWait = 10000;
      NumMatchSpins = 99; // takes care of self-test running on!
      return MACHINE_STATE_ATTRACT;
    }    
    NumMatchSpins += 1;
    MatchSequenceStartTime += MatchDelay;
  }

  // Flash last two digits of all scores as they match the selected value
  for (byte count=0; count<4; count++) {
    if ((ScoreMatches>>count)&0x01) {
      // If this score matches, we're going to flash the last two digits
      if ( (CurrentTime/200)%2 ) {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) & 0x0F); // 0x0F 1111 // 0x1F 11111 // 0x7f 1111111
      } else {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) | 0x30); // 0x30 110000 // 0x60 1100000
      }
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}



// #################### RUN GAMEPLAY MODE ####################
int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte PhaserBonusAtTop = PhaserBonus;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  if (curState==MACHINE_STATE_INIT_GAMEPLAY) { // Very first time into gameplay loop
    returnState = InitGamePlay(curStateChanged);
  } else if (curState==MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged, CurrentBallInPlay);
  } else if (curState==MACHINE_STATE_SKILL_SHOT) {
    returnState = SkillShot(curStateChanged);
  } else if (curState==MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = NormalGamePlay(curStateChanged);
  } else if (curState==MACHINE_STATE_MINI_GAME_1) {
    returnState = MiniGame1(curStateChanged);
  } else if (curState==MACHINE_STATE_MINI_GAME_2) {
    returnState = MiniGame2(curStateChanged);
  } else if (curState==MACHINE_STATE_COUNTDOWN_BONUS) {
    returnState = CountdownBonus(curStateChanged);
  } else if (curState==MACHINE_STATE_BALL_OVER) {
    returnState = BallOver(curStateChanged);
  } else if (curState==MACHINE_STATE_MATCH_MODE) {
    returnState = MatchMode(curStateChanged);
  }

  // The following code is common to all gameplay states. Primarily switch routines, tilt reaction, and award thresholds.
  
  PlayQueue(); // Handle delayed sounds
  LoopCount(); // Display count of times per second that this code is run.

  if (NumTiltWarnings<MAX_TILT_WARNINGS) {
    while ((switchHit=RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY) {

      if (DoubleHitFix(switchHit)) break; // ignore if a double hit
      if (ResetHitFix(switchHit)) break; // ignore if switch set off by resetting solenoid
      if (Debugger(switchHit)) break;     // display most recent switches hit, scrolling through displays 2, 3, and 4. Break if game button held.

      switch (switchHit) {
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LAMPS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;

        case SW_SHOOTER_LANE:
          if (curState == MACHINE_STATE_SKILL_SHOT || curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) {
            if (PlayerShootsAgain==true) {
              PlayerShootsAgain = false;
              SPSATurnedOff = true;
              RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 0);
              RPU_SetLampState(LA_SHOOT_AGAIN, 0);
            }
          }
          if (PFValidated && !(curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2))  {
            PFValidated = false;
            SetUpPhasers();
          }
          break;

        case SW_CREDIT_BUTTON:
          if (CurrentBallInPlay<2) { // single player
            // If we haven't finished the first ball, we can add players
            AddPlayer();
          } 
          break;

        case SW_TILT:
        case SW_SLAM:
          TiltHit();
          break;

        case SW_COIN_1:
          if (!FreePlay) {
            AddCredit(0, CreditsperCoin[0], CoinsperCredit[0]);
            RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
            GetValue = RPU_ReadULFromEEProm(RPU_CHUTE_1_COINS_START_BYTE);
            RPU_WriteULToEEProm(RPU_CHUTE_1_COINS_START_BYTE, (GetValue + 1));
          }
          break;
        case SW_COIN_2:
          if (!FreePlay) {
            AddCredit(1, CreditsperCoin[1], CoinsperCredit[1]);
            RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
            GetValue = RPU_ReadULFromEEProm(RPU_CHUTE_2_COINS_START_BYTE);
            RPU_WriteULToEEProm(RPU_CHUTE_2_COINS_START_BYTE, (GetValue + 1));
          }
          break;
        case SW_COIN_3:
          if (!FreePlay) {
            AddCredit(2, CreditsperCoin[2], CoinsperCredit[2]);
            RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
            GetValue = RPU_ReadULFromEEProm(RPU_CHUTE_3_COINS_START_BYTE);
            RPU_WriteULToEEProm(RPU_CHUTE_3_COINS_START_BYTE, (GetValue + 1));
          }
          break;
          
        case SW_DROP_TARGET_D_LOW:
        case SW_DROP_TARGET_C_MID_LOW:
        case SW_DROP_TARGET_B_MID_HIGH:
        case SW_DROP_TARGET_A_HIGH:
          PFValidated = true;

          if (curState == MACHINE_STATE_MINI_GAME_2 && !MiniGame2Score[switchHit] && CurrentTime - CountdownTimer < 61700) {
            MiniGame2Score[switchHit] = true;
            AddToScore(12500, true);
            PlaySound(SFXCommandSequence[1 + switchHit], 0, voiceGain + 2); // Command Sequence
            if (MiniGame2Score[0]) RPU_SetLampState(LA_2X, 0);
            if (MiniGame2Score[1] && MiniGame2Score[2]) RPU_SetLampState(LA_SPEC_3X, 0);
            if (MiniGame2Score[3]) RPU_SetLampState(LA_3X_2X, 0);
            break;
          }
          if (curState == MACHINE_STATE_MINI_GAME_1) 
            break;
          if (curState == MACHINE_STATE_MINI_GAME_2) 
            break;
            
          AddToScore(500);
          AddToPhaserBonus(1);
          if (CheckIfDTargets4Down() && CurrentTime - DropTimer > 500) { // Two drop targets hit at the same time were registering twice here!
            DropTimer = CurrentTime;
            AddToScore(5000);

            if (BonusXState[CurrentPlayer] < 3) {
              RPU_SetLampState(LA_2X + 1 - BonusXState[CurrentPlayer], 0);
              ++BonusXState[CurrentPlayer];
              RPU_SetLampState(LA_2X + 1 - BonusXState[CurrentPlayer], 1);
            }
            else if (dipThresholdAward == 1) {
              AddToScore(50000, true);
            }
            else if (dipThresholdAward == 2) {
              if (!PlayerShootsAgain) {
                PlayerShootsAgain = true;
                RPU_SetLampState(LA_SHOOT_AGAIN, 1);
                RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
              }
              else
                AddToScore(50000, true);
            }
            else if (dipThresholdAward == 3 && !FreeGameCollected[CurrentPlayer]) {
              AddCredit(0, CreditsperCoin[0], CoinsperCredit[0]);
              My_PushToSolenoidStack(SO_KNOCKER, 3);
              FreeGameCollected[CurrentPlayer] = true;
            }

            My_PushToTimedSolenoidStack(SO_DTARGET_RESET, 15, CurrentTime + 500);
            PlaySound(VoicePlanet[random(1, 1 + VoicePlanet[0])], 1000); 
          }
          break;

        case SW_SAUCER:
          PFValidated = true;
          SkillShotMade = false;

          if (SaucerHitTime == 0 || (CurrentTime - SaucerHitTime) > 500) {
            SaucerHitTime = CurrentTime;

            if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) {
              BallInSaucer = true;
              break;
            }

            if (curState == MACHINE_STATE_SKILL_SHOT) {
              if (PlayerShootsAgain) {
                PlayerShootsAgain = false;
                SPSATurnedOff = true;
                RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 0);
                RPU_SetLampState(LA_SHOOT_AGAIN, 0);
              }
              RPU_SetLampState(LA_A_LANE, !ScoreBALLY[CurrentPlayer][1]);             // In case A-lane set to flashing
              RPU_SetLampState(LA_B_LANE, !ScoreBALLY[CurrentPlayer][0]);             // In case B-lane set to flashing
              RPU_SetLampState(LA_SAUCER_B + ShootFor, 0);                            // In case saucer light is flashing
              RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);          // Turn on letter in saucer lights
              
              if (ShootFor == SaucerBALLY[CurrentPlayer]) {
                SkillShotMade = true;
                PlaySound(SFXSkillShot, 0, bgGain); // Opening fanfare
                RPU_SetDisplay(CurrentPlayer, 6000 * SkillShotDoubler, true, 2);
                CurrentScores[CurrentPlayer] += 6000 * SkillShotDoubler; // Note: Points awarded immediately
                // RPU_SetDisplay(CurrentPlayer, DisplayValue = CurrentScores[CurrentPlayer], true, 2);
              } else {
                if (SkillShotDoubler > 1) {
                  SkillShotMade = true;
                  PlaySound(SFXSkillShot, 0, bgGain); // Opening fanfare
                  RPU_SetDisplay(CurrentPlayer, 3000 * SkillShotDoubler, true, 2);
                  CurrentScores[CurrentPlayer] += 3000 * SkillShotDoubler; // Note: Points awarded immediately
                  // RPU_SetDisplay(CurrentPlayer, DisplayValue = CurrentScores[CurrentPlayer], true, 2);
                } else
                  AddToScore(3000);
              }
            } else
                AddToScore(3000);

            AddToPhaserBonus(3);

            ScoreBALLY[CurrentPlayer][SaucerBALLY[CurrentPlayer]] = true;             // Current letter in SaucerBALLY scored, stored in ScoreBALLY
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 0);            // Turn off letter in saucer lights
            RPU_SetLampState(LA_B_LANE + SaucerBALLY[CurrentPlayer], 0);              // Turn off letter in target lights
            RPU_SetLampState(BALLYMiddle[SaucerBALLY[CurrentPlayer]], 1);             // Turn on letter in middle playfield lights
            if (++SaucerBALLY[CurrentPlayer] > 4) SaucerBALLY[CurrentPlayer] = 0;     // Move saucer to next letter
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);            // Turn on letter in saucer lights

            if (SkillShotMade) {   // Allow fanfare to play a while before anything else
              StopBackgroundSong();
              delay(100 * SoundTimings[SFXSkillShot][1]);
              RPU_SetDisplay(CurrentPlayer, DisplayValue = CurrentScores[CurrentPlayer], true, 2);
              if (backgroundSound != 3)
                ResumeBackgroundSong();
              else
                PlaySound(BackgroundMusic[1 + (backgroundSound = 4)]); // Photon torpedoes don't go with tribble music!
            }
            if (!BALLYScored() && !SkillShotMade) 
              PlaySound(VoiceSaucer, 500); // Arm photon torpedos

            BallInSaucer = true;
          }
          break;

        case SW_HYPER_SPACE_LOWER:
        case SW_HYPER_SPACE_UPPER:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;

          if (HyperSpace2468 < 5) {
            AddToScore(2000 * (1 + HyperSpace2468));
            RPU_SetLampState(LA_HYPER_SPACE_2K + HyperSpace2468, 0);
            ++HyperSpace2468;
            RPU_SetLampState(LA_HYPER_SPACE_2K + HyperSpace2468, 1);
          }
          else if (dipThresholdAward == 1) { // Extra ball light is on. Can receive award 1, 2, or 3
            AddToScore(HyperSpace25000Paid ? 10000 : 30000); // Award 1: 25000 + 5000 first time, 10000 thereafter

            // Hey, I checked. And wherever it says 25000 for Hyper Space extra ball, it actually gives 30000. I don't know why, but I'm reproducing it.

            HyperSpace25000Paid = true;
            RPU_SetLampState(LA_HYPER_SPACE_SPEC, 0);
            RPU_SetLampState(LA_HYPER_SPACE_10K, 1);
          }
          else if (dipThresholdAward == 2) { // Award 2: Extra ball + 5000, or 25000 + 5000 if same player shoots again. 10000 second and subsequent times.
            if (PlayerShootsAgain) {
              AddToScore(HyperSpace25000Paid ? 10000 : 30000);
              HyperSpace25000Paid = true;
            }
            else {
              PlayerShootsAgain = true;
              AddToScore(5000); // Extra 5000 here as well.
              HyperSpace25000Paid = true;
              RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
              RPU_SetLampState(LA_SHOOT_AGAIN, 1);
            }
            RPU_SetLampState(LA_HYPER_SPACE_SPEC, 0);
            RPU_SetLampState(LA_HYPER_SPACE_10K, 1);
          }
          else if (dipThresholdAward == 3) { // Extra ball + 5000. 10000 thereafter.
            PlayerShootsAgain = true;
            AddToScore(HyperSpace25000Paid ? 10000 : 5000);
            HyperSpace25000Paid = true;
            RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
            RPU_SetLampState(LA_SHOOT_AGAIN, 1);
            RPU_SetLampState(LA_HYPER_SPACE_SPEC, 0);
            RPU_SetLampState(LA_HYPER_SPACE_10K, 1);
          }
          break;

        case SW_RIGHT_FLIPPER_LANE:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;

          AddToPhaserBonus(1);
          if (dipFlipperFeedAdjustment || !LeftRight) {
            if (HyperSpace2468 < 5) {
              AddToScore(2000 * (1 + HyperSpace2468));
              RPU_SetLampState(LA_HYPER_SPACE_2K + HyperSpace2468, 0);
              ++HyperSpace2468;
              RPU_SetLampState(LA_HYPER_SPACE_2K + HyperSpace2468, 1);
            }
            else {
              if (PlayerShootsAgain)
                AddToScore(10000);
              else {
                PlayerShootsAgain = true;
                AddToScore(5000);
                RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
                RPU_SetLampState(LA_SHOOT_AGAIN, 1);
              }
              RPU_SetLampState(LA_HYPER_SPACE_SPEC, 0);
              RPU_SetLampState(LA_HYPER_SPACE_10K, 1);
            }
          }
          else {
          AddToScore(300);
          }
          break;

        case SW_LEFT_FLIPPER_LANE:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          AddToPhaserBonus(1);
          if (dipFlipperFeedAdjustment || LeftRight) {
            if (HyperSpace2468 < 5) {
              AddToScore(2000 * (1 + HyperSpace2468));
              RPU_SetLampState(LA_HYPER_SPACE_2K + HyperSpace2468, 0);
              ++HyperSpace2468;
              RPU_SetLampState(LA_HYPER_SPACE_2K + HyperSpace2468, 1);
            }
            else {
              if (PlayerShootsAgain)
                AddToScore(10000);
              else {
                PlayerShootsAgain = true;
                AddToScore(5000);
                RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
                RPU_SetLampState(LA_SHOOT_AGAIN, 1);
              }
              RPU_SetLampState(LA_HYPER_SPACE_SPEC, 0);
              RPU_SetLampState(LA_HYPER_SPACE_10K, 1);
            }
          }
          else {
          AddToScore(300);
          }
          break;

        case SW_500_POINT_TARGET:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 && !MiniGame1Score[0]) {
            MiniGame1Score[0] = true;
            AddToScore(10000, true);
            PlaySound(SFXExplosion4, 0, voiceGain + 2); // Explosion
            RPU_SetLampState(LA_HYPER_SPACE_10K, 0);
            break;
          }
          if (curState == MACHINE_STATE_MINI_GAME_1) 
            break;
          if (curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          AddToScore(500);
          break;

        case SW_10_POINT_REBOUND:
          // Playfield not validated by 10 point rebound!
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          AddToScore(10);
          RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 0);            // Turn off letter in saucer lights
          if (++SaucerBALLY[CurrentPlayer] > 4) SaucerBALLY[CurrentPlayer] = 0;     // Move saucer to next letter
          RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);            // Turn on letter in saucer lights

          if (curState == MACHINE_STATE_SKILL_SHOT) {
            SkillShotDoubler = SkillShotDoubler * 2;
            if (SaucerBALLY[CurrentPlayer] == ShootFor)
              RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1, 0, 250);
          }
          break;

        case SW_300_OR_3000_TARGET:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 && !MiniGame1Score[1]) {
            MiniGame1Score[1] = true;
            AddToScore(10000, true);
            PlaySound(SFXExplosion3, 0, voiceGain + 2); // Explosion
            RPU_SetLampState(LA_300_OR_3000_TARGET, 0);
            break;
          }
          if (curState == MACHINE_STATE_MINI_GAME_1) 
            break;
          if (curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          if (LeftRight || dipCenterAdjustment) {
            AddToScore(3000);
            AddToPhaserBonus(3);
          }
          else {
            AddToScore(300);
          }
          break;

        case SW_PLANET_INLANE:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          AddToScore(LeftRight ? 500 : 5000);
          AddToPhaserBonus(1);
          break;

        case SW_PLANET_OUTLANE:
          PFValidated = true;
            if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          AddToScore(LeftRight ? 5000: 500);
          AddToPhaserBonus(1);
          break;

        case SW_KLINGON_Y:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 && !MiniGame1Score[2]) {
            MiniGame1Score[2] = true;
            AddToScore(10000, true);
            PlaySound(SFXExplosion6, 0, voiceGain + 2); // Explosion
            RPU_SetLampState(LA_KLINGON_Y, 0);
            break;
          }
          if (curState == MACHINE_STATE_MINI_GAME_1) 
            break;
          if (curState == MACHINE_STATE_MINI_GAME_2) 
            break;
            AddToScore(300);
            AddToPhaserBonus(1);

            ScoreBALLY[CurrentPlayer][4] = true;                                      // Y scored, stored in ScoreBALLY
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 0);            // Turn off letter in saucer lights
            RPU_SetLampState(LA_KLINGON_Y, 0);                                        // Turn off letter in target lights
            RPU_SetLampState(BALLYMiddle[4], 1);                                      // Turn on letter in middle playfield lights
            if (++SaucerBALLY[CurrentPlayer] > 4) SaucerBALLY[CurrentPlayer] = 0;     // Move saucer to next letter
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);            // Turn on letter in saucer lights
            BALLYScored();                                                            // Check if BALLY scored, set up photon display
          break;

        case SW_KLINGON_L2:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 && !MiniGame1Score[3]) {
            MiniGame1Score[3] = true;
            AddToScore(10000, true);
            PlaySound(SFXExplosion5, 0, voiceGain + 2); // Explosion
            RPU_SetLampState(LA_KLINGON_L2, 0);
            break;
          }
          if (curState == MACHINE_STATE_MINI_GAME_1) 
            break;
          if (curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          AddToScore(300);
          AddToPhaserBonus(1);

            ScoreBALLY[CurrentPlayer][3] = true;                                      // L2 scored, stored in ScoreBALLY
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 0);            // Turn off letter in saucer lights
            RPU_SetLampState(LA_KLINGON_L2, 0);                                        // Turn off letter in target lights
            RPU_SetLampState(BALLYMiddle[3], 1);                                      // Turn on letter in middle playfield lights
            if (++SaucerBALLY[CurrentPlayer] > 4) SaucerBALLY[CurrentPlayer] = 0;     // Move saucer to next letter
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);            // Turn on letter in saucer lights
            BALLYScored();                                                            // Check if BALLY scored, set up photon display
          break;

        case SW_KLINGON_L1:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 && !MiniGame1Score[4]) {
            MiniGame1Score[4] = true;
            AddToScore(10000, true);
            PlaySound(SFXExplosion4, 0, voiceGain + 2); // Explosion
            RPU_SetLampState(LA_KLINGON_L1, 0);
            break;
          }
          if (curState == MACHINE_STATE_MINI_GAME_1) 
            break;
          if (curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          AddToScore(300);
          AddToPhaserBonus(1);

            ScoreBALLY[CurrentPlayer][2] = true;                                      // L1 scored, stored in ScoreBALLY
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 0);            // Turn off letter in saucer lights
            RPU_SetLampState(LA_KLINGON_L1, 0);                                        // Turn off letter in target lights
            RPU_SetLampState(BALLYMiddle[2], 1);                                      // Turn on letter in middle playfield lights
            if (++SaucerBALLY[CurrentPlayer] > 4) SaucerBALLY[CurrentPlayer] = 0;     // Move saucer to next letter
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);            // Turn on letter in saucer lights
            BALLYScored();                                                            // Check if BALLY scored, set up photon display
          break;

        case SW_KLINGON_A:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          SkillShotMade = false;

          if (curState == MACHINE_STATE_SKILL_SHOT) {
            if (PlayerShootsAgain) {
              PlayerShootsAgain = false;
              SPSATurnedOff = true;
              RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 0);
              RPU_SetLampState(LA_SHOOT_AGAIN, 0);
            }
            RPU_SetLampState(LA_SAUCER_B + ShootFor, 0);


            if (ShootFor == 1) {
                SkillShotMade = true;
                PlaySound(SFXSkillShot, 0, bgGain); // Opening fanfare
                // AddToScore(3300 * SkillShotDoubler, true);
                DisplayValue = CurrentScores[CurrentPlayer] += 3300 * SkillShotDoubler; // Note: Points awarded immediately
                RPU_SetDisplay(CurrentPlayer, 3300 * SkillShotDoubler, true, 2);
            } else {
              if (SkillShotDoubler > 1) {
                SkillShotMade = true;
                PlaySound(SFXSkillShot, 0, bgGain); // Opening fanfare
              }
              AddToScore(300 * SkillShotDoubler, SkillShotDoubler != 1);
              RPU_SetLampState(LA_B_LANE, !ScoreBALLY[CurrentPlayer][0]); // In case B-lane set to flashing
            } 
            if (SkillShotMade) {   // Allow fanfare to play a while before anything else
              StopBackgroundSong();
              SetBackgroundSong(0);
              PlaySound(BackgroundMusic[1 + backgroundSound], 100 * SoundTimings[SFXSkillShot][1]); // Resume background music after fanfare
            }
          } else
                AddToScore(300);

          AddToPhaserBonus(1);

            ScoreBALLY[CurrentPlayer][1] = true;                                      // A scored, stored in ScoreBALLY
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 0);            // Turn off letter in saucer lights
            RPU_SetLampState(LA_A_LANE, 0);                                           // Turn off letter in target lights
            RPU_SetLampState(BALLYMiddle[1], 1);                                      // Turn on letter in middle playfield lights
            if (++SaucerBALLY[CurrentPlayer] > 4) SaucerBALLY[CurrentPlayer] = 0;     // Move saucer to next letter
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);            // Turn on letter in saucer lights
            BALLYScored();                                                            // Check if BALLY scored, set up photon display
          break;

        case SW_KLINGON_B:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          SkillShotMade = false;

          if (curState == MACHINE_STATE_SKILL_SHOT) {
            if (PlayerShootsAgain) {
              PlayerShootsAgain = false;
              SPSATurnedOff = true;
              RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 0);
              RPU_SetLampState(LA_SHOOT_AGAIN, 0);
            }
            RPU_SetLampState(LA_SAUCER_B + ShootFor, 0);
            
            if (ShootFor == 0) {
              SkillShotMade = true;
              PlaySound(SFXSkillShot, 0, bgGain); // Opening fanfare
                // AddToScore(3300 * SkillShotDoubler, true);
                DisplayValue = CurrentScores[CurrentPlayer] += 3300 * SkillShotDoubler; // Note: Points awarded immediately
                RPU_SetDisplay(CurrentPlayer, 3300 * SkillShotDoubler, true, 2);
            } else {
              if (SkillShotDoubler > 1) {
                SkillShotMade = true;
                PlaySound(SFXSkillShot, 0, bgGain); // Opening fanfare (SFX, but background gain used)
              }
              AddToScore(300 * SkillShotDoubler, SkillShotDoubler != 1);
              RPU_SetLampState(LA_A_LANE, !ScoreBALLY[CurrentPlayer][1]); // In case A-lane set to flashing
            } 
            if (SkillShotMade) {   // Allow fanfare to play a while before anything else
              StopBackgroundSong();
              SetBackgroundSong(0);
              PlaySound(BackgroundMusic[1 + backgroundSound], 100 * SoundTimings[SFXSkillShot][1]); // Resume background music after fanfare
            }
          } else
                AddToScore(300);

          AddToPhaserBonus(1);

            ScoreBALLY[CurrentPlayer][0] = true;                                      // B scored, stored in ScoreBALLY
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 0);            // Turn off letter in saucer lights
            RPU_SetLampState(LA_B_LANE, 0);                                        // Turn off letter in target lights
            RPU_SetLampState(BALLYMiddle[0], 1);                                      // Turn on letter in middle playfield lights
            if (++SaucerBALLY[CurrentPlayer] > 4) SaucerBALLY[CurrentPlayer] = 0;     // Move saucer to next letter
            RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);            // Turn on letter in saucer lights
            BALLYScored();                                                            // Check if BALLY scored, set up photon display
          break;

        case SW_RIGHT_OUTLANE:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          AddToScore(1000);
          if ((dipOutlaneAdjustment || LeftRight) && PhotonBonus[CurrentPlayer] > 2) {
            if (dipThresholdAward == 1) {
              AddToScore(50000);
            }
            else if (dipThresholdAward == 2) {
              if (PlayerShootsAgain)
                  AddToScore(50000);
                else {
                  PlayerShootsAgain = true;
                  RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
                  RPU_SetLampState(LA_SHOOT_AGAIN, 1);
                }
              }
            else if (dipThresholdAward == 3) {
              if (!FreeGameCollected[CurrentPlayer]) {
                My_PushToTimedSolenoidStack(SO_KNOCKER, 3, CurrentTime, true);
                AddCredit(0, CreditsperCoin[0], CoinsperCredit[0], 1);
                FreeGameCollected[CurrentPlayer] = true;
              }
            }
          }
          break;

        case SW_LEFT_OUTLANE:
          PFValidated = true;
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          AddToScore(1000);
          if ((dipOutlaneAdjustment || !LeftRight) && PhotonBonus[CurrentPlayer] > 2) {
            if (dipThresholdAward == 1) {
              AddToScore(50000);
            }
            else if (dipThresholdAward == 2) {
              if (PlayerShootsAgain)
                  AddToScore(50000);
                else {
                  PlayerShootsAgain = true;
                  RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
                  RPU_SetLampState(LA_SHOOT_AGAIN, 1);
                }
              }
            else if (dipThresholdAward == 3) {
              if (!FreeGameCollected[CurrentPlayer]) {
                My_PushToTimedSolenoidStack(SO_KNOCKER, 3, CurrentTime, true);
                AddCredit(0, CreditsperCoin[0], CoinsperCredit[0], 1);
                FreeGameCollected[CurrentPlayer] = true;
              }
            }
          }
          break;

        case SW_RIGHT_SLINGSHOT:
        case SW_LEFT_SLINGSHOT:
          PFValidated = true;
          PlaySound(SFXSlingshot, 100); // Phaser
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          AddToScore(10, true);

          if (!dipFlipperFeedAdjustment) RPU_SetLampState(LA_RIGHT_SLINGSHOT_INLANE + LeftRight, 0);
          if (!dipOutlaneAdjustment) RPU_SetLampState(LA_LEFT_SLINGSHOT_OUTLANE - LeftRight, 0);
          RPU_SetLampState(LA_PLANET_INLANE + LeftRight, 0);
          LeftRight = !LeftRight;
          if (!dipFlipperFeedAdjustment) RPU_SetLampState(LA_RIGHT_SLINGSHOT_INLANE + LeftRight, 1);
          RPU_SetLampState(LA_LEFT_SLINGSHOT_OUTLANE - LeftRight, PhotonBonus[CurrentPlayer] > 2);
          // if (dipOutlaneAdjustment) RPU_SetLampState(LA_RIGHT_SLINGSHOT_OUTLANE + LeftRight, PhotonBonus[CurrentPlayer] > 2);
          RPU_SetLampState(LA_PLANET_INLANE + LeftRight, 1);
          RPU_SetLampState(LA_300_OR_3000_TARGET, LeftRight || dipCenterAdjustment);
          break;

        case SW_CENTER_THUMPER_BUMPER:
          PFValidated = true;
          PlaySound(SFXExplosion1, 0, voiceGain + 2); // Explosion
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          
          AddToScore(100, true);
          break;

        case SW_LEFT_THUMPER_BUMPER:
          PFValidated = true;
          PlaySound(SFXExplosion2, 0, voiceGain + 2); // Explosion
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          AddToScore(100, true);
          break;

        case SW_RIGHT_THUMPER_BUMPER:
          PFValidated = true;
          PlaySound(SFXExplosion3, 0, voiceGain + 2); // Explosion
          if (curState == MACHINE_STATE_MINI_GAME_1 || curState == MACHINE_STATE_MINI_GAME_2) 
            break;
          AddToScore(100, true);
          break;

      }
    }
  } else {
    // We're tilted, so just wait for outhole. Pull all switches, no points awarded. React to bare minimum of switches!
    while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY && switchHit != SW_OUTHOLE) {
      switch (switchHit) {
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LAMPS;
          SetLastSelfTestChangedTime(CurrentTime);
          break; 
        case SW_COIN_1:
          if (!FreePlay) {
            AddCredit(0, CreditsperCoin[0], CoinsperCredit[0]);
            RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
          }
          break;
        case SW_COIN_2:
          if (!FreePlay) {
            AddCredit(1, CreditsperCoin[1], CoinsperCredit[1]);
            RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
          }
          break;
        case SW_COIN_3:
          if (!FreePlay) {
            AddCredit(2, CreditsperCoin[2], CoinsperCredit[2]);
            RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
          }
          break;
        case SW_SAUCER:
          My_PushToTimedSolenoidStack(SO_SAUCER, 5, CurrentTime + 500, true);
         break;
      }
    }
  }

  // Handle score award thresholds. Star Trek has one standard threshold level, then two mini-game levels.

  if (CurrentScores[CurrentPlayer]>=ScoreAward1 && ScoreAward1 > 0 && ScoreAwardStates[CurrentPlayer]==0) { // Threshold Level 1 Award
    ScoreAwardStates[CurrentPlayer] = 1;
    if (dipThresholdAward == 1 || (dipThresholdAward == 2 && PlayerShootsAgain))                            // Points
      AddToScore(25000, true);
    else if (dipThresholdAward == 2) {                                                                      // Extra Ball
      SPSATurnedOff = true;
      PlayerShootsAgain = true;
      RPU_SetLampState(LA_SHOOT_AGAIN, 1);
      RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
    }
    else if (dipThresholdAward == 3) {                                                                      // Game Credit
    My_PushToTimedSolenoidStack(SO_KNOCKER, 3, CurrentTime, true);
    AddCredit(0, CreditsperCoin[0], CoinsperCredit[0], 1);
    // if (!dipUnlimitedReplays) FreeGameCollected[CurrentPlayer] = true;
    TotalReplays += 1;
    RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, TotalReplays);
    }
  }
  if (CurrentScores[CurrentPlayer]>=ScoreAward2 && ScoreAward2 > 0 && ScoreAwardStates[CurrentPlayer]==1) { // Mini-Game 1
    ScoreAwardStates[CurrentPlayer] = 2;
    SPSATurnedOff = true;
    if (PlayerShootsAgain) // Passed threshold with player shoots again, 25000 points awarded to compensate because we are using it for mini-game
      AddToScore(25000);
    PlayerShootsAgain = true;
    RPU_SetLampState(LA_SHOOT_AGAIN, 1);
    RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
  }
  if (CurrentScores[CurrentPlayer]>=ScoreAward3 && ScoreAward3 > 0 && ScoreAwardStates[CurrentPlayer]==2 && MiniGamePlayed[CurrentPlayer]==1) { // Mini-Game 2
    ScoreAwardStates[CurrentPlayer] = 3;
    SPSATurnedOff = true;
    if (PlayerShootsAgain) // Passed threshold with player shoots again, 25000 points awarded to compensate because we are using it for mini-game
      AddToScore(25000);
    PlayerShootsAgain = true;
    RPU_SetLampState(LA_SHOOT_AGAIN, 1);
    RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
  }

  if (switchHit == SW_OUTHOLE && NumTiltWarnings >= MAX_TILT_WARNINGS) 
    returnState = MACHINE_STATE_BALL_OVER; // Should take care of tilt during skill shot!

  if (PhaserBonusAtTop!=PhaserBonus) { // display Photon bonus if changed
    ShowPhaserBonusOnLadder(PhaserBonus);
  }
  if (scoreAtTop!=CurrentScores[CurrentPlayer]) {
  }
  return returnState;
}



// #################### LOOP ####################
void loop() {
  // This line has to be in the main loop
  RPU_DataRead(0);

  CurrentTime = millis();
  int newMachineState = MachineState;

  // Machine state is self-test/attract/game play
  if (MachineState<0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState!=MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  RPU_ApplyFlashToLamps(CurrentTime);
  RPU_UpdateTimedSolenoidStack(CurrentTime);
}



// #################### TILT HIT ####################
void TiltHit() {
  NumTiltWarnings += 1;
  if (NumTiltWarnings>=MAX_TILT_WARNINGS) {
    StopAudio(); // Stop sound
    ResetCountingDisplay();
    RPU_DisableSolenoidStack();
    RPU_SetDisableFlippers(true);
    RPU_TurnOffAllLamps();
    if (Credits > 0) RPU_SetLampState(LA_CREDIT_INDICATOR, 1); // Credit light stays on
    RPU_SetLampState(LA_TILT, 1);
    PlaySound(VoiceTilt[random(1, 1 + VoiceTilt[0])], 10); 
  }
}



// #################### SKILL SHOT ####################
int SkillShot(boolean curStateChanged) {
  int returnState = MACHINE_STATE_SKILL_SHOT;
  
  if (curStateChanged) {
    if (backgroundOn)
      PlayBackgroundSong(BackgroundMusic[1 + backgroundSound]);           // Background music

    if (!ballSavePlayed) {                                                // "Try Again" already played?
      if (kidMode)
        PlaySound(VoiceKidsMode[random(1, 1 + VoiceKidsMode[0])], 500);   // Always start with kid sounds in kids' mode
      else if (backgroundSound == 0)                                      // Bridge sounds
        PlaySound(VoiceBridge[random(1, 1 + VoiceBridge[0])], 500);       // Bridge voices
      else if (backgroundSound == 3)                                      // Tribbles music
        PlaySound(VoiceFun[random(1, 1 + VoiceFun[0])], 500);             // Fun voices to go with Tribble music
      else                                                                // Adventurous music
        PlaySound(VoiceAdventure[random(1, 1 + VoiceAdventure[0])], 500); // Adventurous voices to go with adventurous music
    }
    ballSavePlayed = false;

    ShootFor = 0;                                                         // Identify skill shot target (next letter in BALLY)
    for (i = 0; i < 5; ++i) {
      if (ScoreBALLY[CurrentPlayer][i] == 0) {
        ShootFor = i;
        i = 99;
      }
    }
    RPU_SetDisplayCredits(Credits, dipCreditsDisplayed);
    
    if (ShootFor == 0) RPU_SetLampState(LA_B_LANE, 1, 0, 250);
    if (ShootFor == 1) RPU_SetLampState(LA_A_LANE, 1, 0, 250);
    if (ShootFor == SaucerBALLY[CurrentPlayer]) RPU_SetLampState(LA_SAUCER_B + ShootFor, 1, 0, 250);
    SkillShotDoubler = 1;
    SPSATurnedOff = false;
  }

  if (!PFValidated) {
    CountingFlashDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], CurrentTime, 250, 2); // Display flashes during skill shot
  } else {
    returnState = MACHINE_STATE_NORMAL_GAMEPLAY;                                            // Skill shot is over

    RPU_SetLampState(LA_B_LANE, !ScoreBALLY[CurrentPlayer][0]);                             // Stop all flashing lamps
    RPU_SetLampState(LA_A_LANE, !ScoreBALLY[CurrentPlayer][1]);
    for (i = 0; i < 5; ++i)
      RPU_SetLampState(LA_SAUCER_B + i, 0);
    RPU_SetLampState(LA_SAUCER_B + SaucerBALLY[CurrentPlayer], 1);

    for (byte count=0; count<CurrentNumPlayers; count++)                                   // Stop flashing display
      CountingDisplay(count, CurrentScores[count], true, 2, true);

    if (!SPSATurnedOff) {                                                                  // Make sure "same player shoots again" has been turned off
      PlayerShootsAgain = false;                                                           //   (it may have been turned off and back on again)
      RPU_SetLampState(LA_SHOOT_AGAIN, 0);
      RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 0);
    }
  }

  // Check to see if ball is in the outhole. Added to fix skillshot straight to outhole, or simply fell back in.
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (BallTimeInTrough == 0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least 
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime - BallTimeInTrough) > 500) {
          BallTimeInTrough = 0;
          SkillShotState = 0;
          SkillShotHits = 0;
          if (CurrentScores[CurrentPlayer] == InitialScore) {
            My_PushToTimedSolenoidStack(SO_OUTHOLE, 4, CurrentTime + 100);
            // PlaySound(VoiceBallSave, 1000); // Try again
            }
          else
            returnState = MACHINE_STATE_BALL_OVER; // points scored in lower playfield with Skill Shot mode. Ball over will handle.
      } 
    }
  }
  else
    BallTimeInTrough = 0;
  
  return returnState;
}




// #################### MINI GAME 1: Klingon Battle ####################
int MiniGame1(boolean curStateChanged) {
  int returnState = MACHINE_STATE_MINI_GAME_1;
  
  if (curStateChanged) {
    if (backgroundOn)
      PlayBackgroundSong(BGMiniGame1);           // Background music
    PlaySound(VoiceMiniGame1, 500); // Sensors picking up a klingon battle cruiser


    RPU_TurnOffAllLamps();
    RPU_SetLampState(LA_KLINGON_L1, 1, 0, 250);
    RPU_SetLampState(LA_KLINGON_L2, 1, 0, 250);
    RPU_SetLampState(LA_KLINGON_Y, 1, 0, 250);
    RPU_SetLampState(LA_300_OR_3000_TARGET, 1, 0, 250);
    RPU_SetLampState(LA_HYPER_SPACE_10K, 1, 0, 250);

    if (PlayerShootsAgain) {
      RPU_SetLampState(LA_SHOOT_AGAIN, 1);
      RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
    }

    MiniGame1Score[0] = MiniGame1Score[1] = MiniGame1Score[2] = MiniGame1Score[3] = MiniGame1Score[4] = false;
    ballSavePlayed = false;
    SPSATurnedOff = false;
    MiniGameStarted = false;
    MiniGameEnded = false;
    SaucerHitTime = CurrentTime;
    MiniGamePauseTimer = 0;
    EELightShow = false;
    WinnerLightShow = false;
  }

  if (!PFValidated) {
    CountingFlashDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], CurrentTime, 250, 2); // Display flashes until ball on playfield
  } else if (!MiniGameStarted) {
      CountingDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], true, 2, true);

      if (!SPSATurnedOff) {                                                                  // Make sure "same player shoots again" has been turned off
        PlayerShootsAgain = false;                                                           //   (it may have been turned off and back on again)
        RPU_SetLampState(LA_SHOOT_AGAIN, 0);
        RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 0);
      }
    MiniGameStarted = true;
  }

  // HANDLE SAUCER
  if (BallInSaucer) {
    if (CurrentTime - SaucerHitTime > 500) { 
      My_PushToSolenoidStack(SO_SAUCER, 5);
      BallInSaucer = false;
    }
  }

  CountingDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], true, 2);
  if (EELightShow) EnterpriseExplodes();
  if (WinnerLightShow) WinnerLights();

  if (MiniGame1Score[0] && MiniGame1Score[1] && MiniGame1Score[2] && MiniGame1Score[3] && MiniGame1Score[4] && !MiniGameEnded) { // Win!
    // MiniGame1Score[0] = MiniGame1Score[1] = MiniGame1Score[2] = MiniGame1Score[3] = MiniGame1Score[4] = false;
    RPU_SetDisableFlippers(true);
    SetUpWinner();
    AddToScore(50000, true, 10);
    MiniGameEnded = true;
  }

  // Check to see if ball is in the outhole.
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (MiniGameStarted && !MiniGameEnded) {
      MiniGameEnded = true;
      SetUpEnterpriseExplodes();
    }

    if (BallTimeInTrough == 0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least 
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime - BallTimeInTrough) > 500 && CurrentTime > MiniGamePauseTimer) { // Also, wait for end of final explosion or fanfare
        BallTimeInTrough = 0;
        if (MiniGameStarted) {
          BallDrained = true;
          RPU_SetDisableFlippers(false);
          returnState = MACHINE_STATE_BALL_OVER; 
        }
        else
          My_PushToTimedSolenoidStack(SO_OUTHOLE, 4, CurrentTime + 100);
      } 
    }
  }
  else
    BallTimeInTrough = 0;
  
  return returnState;
}




// #################### MINI GAME 2: Auto-Destruct Sequence ####################
int MiniGame2(boolean curStateChanged) {
  int returnState = MACHINE_STATE_MINI_GAME_2;
  
  if (curStateChanged) {
    if (backgroundOn)
      PlayBackgroundSong(BGMiniGame2);           // Background music
    PlaySound(VoiceMiniGame2, 500); // One minute to auto-destruct

    CountdownTimer = CurrentTime;

    RPU_TurnOffAllLamps();
    RPU_SetLampState(LA_2X, 1, 0, 250);
    RPU_SetLampState(LA_3X_2X, 1, 0, 250);
    RPU_SetLampState(LA_SPEC_3X, 1, 0, 250);

    RPU_SetDisplayCredits(60, true);
    
    if (PlayerShootsAgain) {
      RPU_SetLampState(LA_SHOOT_AGAIN, 1);
      RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 1);
    }

    MiniGame2Score[0] = MiniGame2Score[1] = MiniGame2Score[2] = MiniGame2Score[3] = false;
    ballSavePlayed = false;
    SPSATurnedOff = false;
    MiniGameStarted = false;
    MiniGameEnded = false;
    SaucerHitTime = CurrentTime;
    MiniGamePauseTimer = 0;
    EELightShow = false;
    WinnerLightShow = false;
    FinalTime = 255;
  }

  if (!PFValidated) {
    CountingFlashDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], CurrentTime, 250, 2);  // Display flashes until ball on playfield
  } else if (!MiniGameStarted) {                                                             // Run once, when playfield reached
      CountingDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], true, 2, true);

      if (!SPSATurnedOff) {                                                                  // Make sure "same player shoots again" has been turned off
        PlayerShootsAgain = false;                                                           //   (it may have been turned off and back on again)
        RPU_SetLampState(LA_SHOOT_AGAIN, 0);
        RPU_SetLampState(LA_SAME_PLAYER_SHOOTS_AGAIN, 0);
      }
    MiniGameStarted = true;
  }

  // HANDLE SAUCER
  if (BallInSaucer) {
    if (CurrentTime - SaucerHitTime > 500) { 
      My_PushToSolenoidStack(SO_SAUCER, 5);
      BallInSaucer = false;
    }
  }
  CountingDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], true, 2);
  if (FinalTime != 255) RPU_SetDisplayFlashCredits(CurrentTime, 250);
  if (EELightShow) EnterpriseExplodes();
  if (WinnerLightShow) WinnerLights();

  if (CurrentTime - CountdownTimer > 1700 && !MiniGameEnded) {                                                       // Start Timer when auto-destruct voice ends, 1700ms
      DestructTimer = (1000 + 1700 + 60000 - (CurrentTime - CountdownTimer)) / 1000;
      RPU_SetDisplayCredits(DestructTimer, true, true);                                                              // +1000 to round up
  }

  if (CurrentTime - CountdownTimer > 61700 && !MiniGameEnded) {                                                     // Time is up!
    // MiniGame2Score[0] = MiniGame2Score[1] = MiniGame2Score[2] = MiniGame2Score[3] = false;
    RPU_SetDisableFlippers(true);
    FinalTime = DestructTimer;
    // RPU_SetDisplayCredits(0, true);
    SetUpEnterpriseExplodes();
    MiniGameEnded = true;
  }

  if (MiniGame2Score[0] && MiniGame2Score[1] && MiniGame2Score[2] && MiniGame2Score[3] && !MiniGameEnded) {         // Win!
    // MiniGame2Score[0] = MiniGame2Score[1] = MiniGame2Score[2] = MiniGame2Score[3] = false;
    RPU_SetDisableFlippers(true);
    FinalTime = DestructTimer;
    // RPU_SetDisplayCredits(0, true);
    SetUpWinner();
    AddToScore(50000, true, 10);
    MiniGameEnded = true;
  }

  // Check to see if ball is in the outhole.
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (MiniGameStarted && !MiniGameEnded) {
      MiniGameEnded = true;
      SetUpEnterpriseExplodes();
      FinalTime = DestructTimer;
    }

    if (BallTimeInTrough == 0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least 
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime - BallTimeInTrough) > 500 && CurrentTime > MiniGamePauseTimer) { // Also, wait for end of final explosion or fanfare
        BallTimeInTrough = 0;
        if (MiniGameStarted) {
          BallDrained = true;
          RPU_SetDisableFlippers(false);
          // RPU_SetDisplayCredits(0, false);
          returnState = MACHINE_STATE_BALL_OVER; 
        }
        else
          My_PushToTimedSolenoidStack(SO_OUTHOLE, 4, CurrentTime + 100);
      } 
    }
  }
  else
    BallTimeInTrough = 0;
  
  return returnState;
}



// #################### DROP TARGETS ####################
// ########## CHECK IF DROP TARGETS FOUR DOWN ##########
boolean CheckIfDTargets4Down() {
  return (  RPU_ReadSingleSwitchState(SW_DROP_TARGET_D_LOW) &&
            RPU_ReadSingleSwitchState(SW_DROP_TARGET_C_MID_LOW) &&
            RPU_ReadSingleSwitchState(SW_DROP_TARGET_B_MID_HIGH) &&
            RPU_ReadSingleSwitchState(SW_DROP_TARGET_A_HIGH) );
}



// ########## ADD TO Photon BONUS ##########
void AddToPhaserBonus(byte bonusAddition) {
  PhaserBonus += bonusAddition;
  if (PhaserBonus>MAX_PHASER_BONUS) PhaserBonus = MAX_PHASER_BONUS;
}



// ########## SELF TEST CAN RUN ANY OF THE LIGHTSHOWS ##########
void SelfTestLightShows(int LightShow) {
  EELightShow = WinnerLightShow = PhotonLightShow = PhaserLightShow = MatchLightShow = AttractLightShow = false;
  PhaserBonus = 0;
  for (i = (LightShow == 0); i < 4; ++i) { // Blank all displays, except display 1 during test zero
    RPU_SetDisplay(i, 0);
    RPU_SetDisplayBlank(i, 0x00);        
    }

  switch (LightShow) {
    case 1:                  // Setup Photon Torpedoes
      StopBackgroundSong();
      BALLYScored(true);
      break; 
    case 2:                  // Setup Phasers
      StopBackgroundSong();
      SetUpPhasers();
      break;
    case 3:                  // Setup Enterprise Explosion
      SetUpEnterpriseExplodes();
      break;
    case 4:                  // Setup Winner Light Show
      SetUpWinner();
      break;
    case 5:                  // Setup Match Light Show
      SetUpMatch();
      break;
    case 6:                  // Setup Attract Light Show
      SetUpAttract();
      break;
  }
}



// ########## DETERMINE IF BALLY SCORED. INITIATE SOUND AND LIGHT SHOW. ##########
boolean BALLYScored(boolean selfTest = 0) {
  boolean scored = true;

  if (selfTest)
    scored = true;
  else
    for (i = 0; i < 5; ++i) 
      scored = scored && ScoreBALLY[CurrentPlayer][i];

  if (scored) {
    if (!selfTest) {
      SetBackgroundSong(BackgroundMusic[1 + backgroundSound]);
      StopAllButBG();
    }
    PlaySound(VoicePhotonTorpedoesFire, 200); // Photon torpedoes, fire!
    PhotonLightShow = true;
    PhotonStep = PhotonStep2 = 0;
    PhotonLightShowTimer = PhotonLightShowTimer2 = CurrentTime;
    for (i = 0; i < 5; ++i) {
      ScoreBALLY[CurrentPlayer][i] = 0;
      RPU_SetLampState(LA_B_LANE + i, 1);
    }
    if      (PhotonBonus[CurrentPlayer] == 0) PhotonScore = 2000;
    else if (PhotonBonus[CurrentPlayer] == 1) PhotonScore = 5000;
    else if (PhotonBonus[CurrentPlayer] == 2) PhotonScore = 10000;
    else if (PhotonBonus[CurrentPlayer] == 3 && ((dipThresholdAward == 1) || (dipThresholdAward == 2 && PlayerShootsAgain))) PhotonScore = 10000;
    else if (PhotonBonus[CurrentPlayer] == 4 && dipThresholdAward == 1) PhotonScore = 10000;
    else if (PhotonBonus[CurrentPlayer] == 4 && (!dipBallySpecialAdjustment) && (dipThresholdAward == 2) && 
                                                                    (FreeBallCollected[CurrentPlayer] || PlayerShootsAgain)) PhotonScore = 10000;
    else if (PhotonBonus[CurrentPlayer] == 4 && dipBallySpecialAdjustment && (dipThresholdAward == 2) && PlayerShootsAgain) PhotonScore = 10000;
    else PhotonScore = 0;
  }
  return scored;
}



// ########## SET UP PHASER SOUND AND LIGHT SHOW ##########
void SetUpPhasers() {
  // StopAllButBG();
  PlaySound(VoiceFireAllPhasers[BonusXState[CurrentPlayer]], 0, bgGain); // Fire all phasers + 1, 2, or 3 phaser sounds. Voice + SFX, compromise at BG.
  PhaserLightShow = true;
  PhaserStep = 0;
  PhaserLightShowTimer = 1000 + CurrentTime;
  RPU_SetDisplayCredits(PhaserBonus);
}


// ########## SET UP ENTERPRISE EXPLODES SOUND AND LIGHT SHOW ##########
void SetUpEnterpriseExplodes() {
  RPU_TurnOffAllLamps();
  StopBackgroundSong();
  LampPlacement[13].distance = AlternativeLamps[0].distance;
  LampPlacement[14].distance = AlternativeLamps[1].distance;
  PlaySound(SFXEnterpriseExplodes, 0, voiceGain + 2);
  MiniGamePauseTimer = CurrentTime + 100 * SoundTimings[SFXEnterpriseExplodes][1];
  EELightShow = true;
}


// ########## SET UP MINIGAME WINNER SOUND AND LIGHT SHOW ##########
void SetUpWinner() {
  RPU_TurnOffAllLamps();
  StopBackgroundSong();
  LampPlacement[13].vertical = AlternativeLamps[2].vertical;
  LampPlacement[13].horizontal = AlternativeLamps[2].horizontal;
  LampPlacement[14].vertical = AlternativeLamps[3].vertical;
  LampPlacement[14].horizontal = AlternativeLamps[3].horizontal;
  PlaySound(SFXSkillShot, 0, bgGain); // Opening fanfare
  MiniGamePauseTimer = CurrentTime + 100 * SoundTimings[SFXSkillShot][1];
  WinnerLightShow = true;
}


// ########## SET UP MINIGAME WINNER SOUND AND LIGHT SHOW ##########
void SetUpMatch() {
  CurrentScores[0] = 0;
  RPU_TurnOffAllLamps();
  StopBackgroundSong();
  MatchMode(1);
  MatchLightShow = true;
}


// ########## SET UP MINIGAME WINNER SOUND AND LIGHT SHOW ##########
void SetUpAttract() {
  RPU_TurnOffAllLamps();
  StopBackgroundSong();
  RunAttractMode(MACHINE_STATE_TEST_LAMPS, 1);
  AttractLightShow = true;
}



// ########## SHOW PHASER BONUS ON LADDER ##########
void ShowPhaserBonusOnLadder(byte bonus) {
  if (bonus > MAX_PHASER_BONUS) bonus = MAX_PHASER_BONUS;

  RPU_SetLampState(LA_BONUS_20K, bonus >= 20, 0, 0);
  RPU_SetLampState(LA_BONUS_10K, bonus >= 10 && bonus < 20, 0, 0);
  bonus = (bonus % 10) - 1;
  for (i = 0; i < 9; ++i)
    RPU_SetLampState(LA_BONUS_1K + i, bonus == i);
}



// ########## COUNTDOWN BONUS ##########
int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {
    RPU_SetLampState(LA_BALL_IN_PLAY, 1, 0, 250);
    PhotonsDone = false;
  }
  if (WTWaitingTime > CurrentTime && !PhotonsDone) {
    FirePhotonTorpedoes(); // (In case this is still running from normal game play)
    FireAllPhasers();
    CountingDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], true, 2);
    return MACHINE_STATE_COUNTDOWN_BONUS;
  }
  else if (!PhotonsDone) {
    SetUpPhasers();
    PhotonsDone = true;
  }
  FireAllPhasers();
  CountingDisplay(CurrentPlayer, CurrentScores[CurrentPlayer], true, 2);
  if (!PhaserLightShow) return MACHINE_STATE_BALL_OVER;
  return MACHINE_STATE_COUNTDOWN_BONUS;
}



// #################### GAME-SPECIFIC SELF TESTS ####################
int RunExtendedSelfTest(int curState, boolean curStateChanged, unsigned long CurrentTime, byte resetSwitch, byte otherSwitch, byte endSwitch) {
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  int returnState = curState;
  boolean resetDoubleClick = false;
  unsigned short auditNumStartByte = 0;
  unsigned short auditNumStartUL = 0;
  long highNum = 1;
  boolean resetBeingHeld = false;
  
  if (curSwitch==resetSwitch) {
      ExtResetHold = CurrentTime;
      if ((CurrentTime-ExtLastResetPress)<400) {
        resetDoubleClick = true;
        curSwitch = SWITCH_STACK_EMPTY;
      }
      ExtLastResetPress = CurrentTime;
    }

  if (ExtResetHold!=0 && !RPU_ReadSingleSwitchState(resetSwitch)) ExtResetHold = 0;
  if (ExtResetHold!=0 && (CurrentTime-ExtResetHold)>250) resetBeingHeld = true;

  if (curSwitch == endSwitch) return MACHINE_STATE_ATTRACT;

  if (curSwitch==SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime())>500) {         
    returnState -= 1;
    SetLastSelfTestChangedTime(CurrentTime);
    }

  if (curStateChanged) {
    for (int count=0; count<4; count++) {
      RPU_SetDisplay(count, 0);
      RPU_SetDisplayBlank(count, 0x00);        
      }
    RPU_SetDisplayCredits(MACHINE_STATE_TEST_DIP_SWITCHES - curState);
    RPU_SetDisplayBallInPlay(0, false);
    }
    
  if (curState == MACHINE_STATE_TEST_BACKGROUND_ON || curState == MACHINE_STATE_TEST_FREEPLAY) 
    highNum = 1;
  else
    highNum = 999999;
  
  if (curState==MACHINE_STATE_TEST_BACKGROUND_ON)
    auditNumStartByte = RPU_BACKGROUND_ON;
  else if (curState==MACHINE_STATE_TEST_FREEPLAY)
    auditNumStartByte = RPU_FREEPLAY;
  else if (curState == MACHINE_STATE_TEST_BALL_SAVE)
    EnterBallSaveData(curSwitch, resetDoubleClick, resetBeingHeld, curStateChanged, CurrentTime, resetSwitch, otherSwitch, endSwitch);
  else if (curState == MACHINE_STATE_TEST_GAIN)
    EnterGainData(curSwitch, resetDoubleClick, resetBeingHeld, curStateChanged, CurrentTime, resetSwitch, otherSwitch, endSwitch);
  else return MACHINE_STATE_ATTRACT;

    if (auditNumStartByte != 0) {
      if (curStateChanged) {
        ExtSavedValue = RPU_ReadByteFromEEProm(auditNumStartByte);
        if (ExtSavedValue > highNum) ExtSavedValue = highNum;
        RPU_SetDisplay(0, ExtSavedValue, true, 2);
      }
      if (curSwitch == resetSwitch) {
        ExtSavedValue += 1;
        if (ExtSavedValue > highNum) 
            ExtSavedValue = 0;
        RPU_SetDisplay(0, ExtSavedValue, true, 2);  
        RPU_WriteByteToEEProm(auditNumStartByte, ExtSavedValue);
      }
      if (resetBeingHeld) {
        ExtSavedValue += 1;
        if (ExtSavedValue > highNum) 
            ExtSavedValue = 0;
        ExtResetHold = CurrentTime;
        RPU_SetDisplay(0, ExtSavedValue, true, 2);  
        RPU_WriteByteToEEProm(auditNumStartByte, ExtSavedValue);
      }
      if (resetDoubleClick) {
        ExtSavedValue = 0;
        RPU_SetDisplay(0, ExtSavedValue, true, 2); 
        RPU_WriteByteToEEProm(auditNumStartByte, ExtSavedValue);
      }
    }

    if (auditNumStartUL != 0) {
      if (curStateChanged) {
        if ((ExtSavedValue = RPU_ReadULFromEEProm(auditNumStartUL)) > highNum) ExtSavedValue = highNum;
        RPU_SetDisplay(0, ExtSavedValue, true, 2);
      }
      if (curSwitch == resetSwitch) {
        ExtSavedValue +=1;
        if (ExtSavedValue > highNum) ExtSavedValue = 0;
        RPU_SetDisplay(0, ExtSavedValue, true, 2);  
        RPU_WriteULToEEProm(auditNumStartUL, ExtSavedValue);
      }
      if (resetBeingHeld) {
        ExtSavedValue +=1;
        if (ExtSavedValue > highNum) ExtSavedValue = 0;
        ExtResetHold = CurrentTime;
        RPU_SetDisplay(0, ExtSavedValue, true, 2);  
        RPU_WriteULToEEProm(auditNumStartUL, ExtSavedValue);
      }
      if (resetDoubleClick) {
        ExtSavedValue = 0;
        RPU_SetDisplay(0, ExtSavedValue, true, 2); 
        RPU_WriteULToEEProm(auditNumStartUL, ExtSavedValue);
        }
    }
  return returnState;
}

// #################### ENTER BALL SAVE DATA USING DISPLAYS ####################
void EnterBallSaveData(byte curSwitch,boolean resetDoubleClick, boolean resetBeingHeld, boolean curStateChanged, 
                                          unsigned long CurrentTime, byte resetSwitch, byte otherSwitch, byte endSwitch) {
  unsigned long highNum[3] = {5, 25000, 25}; // Maximum values

  if (curStateChanged) {
    dispNum = 0;
    ExtSavedValue = RPU_ReadByteFromEEProm(RPU_BALL_SAVES);
    RPU_SetDisplayFlash(0, ExtSavedValue, CurrentTime, 250, 2);
    ExtSavedValue = 100 * RPU_ReadByteFromEEProm(RPU_BALL_SAVE_SCORE);
    RPU_SetDisplay(1, ExtSavedValue, true, 2);
    ExtSavedValue = RPU_ReadByteFromEEProm(RPU_BALL_SAVE_NUM_SECS);
    RPU_SetDisplay(2, ExtSavedValue, true, 2);
    }
  ExtSavedValue = RPU_ReadByteFromEEProm(RPU_BALL_SAVES + dispNum);
  if (dispNum == 1) ExtSavedValue = 100 * ExtSavedValue;
  RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2);
  if (curSwitch == otherSwitch) {
    RPU_SetDisplay(dispNum, ExtSavedValue, true, 2);
    dispNum = (dispNum + 1) % 3;
    ExtSavedValue = RPU_ReadByteFromEEProm(RPU_BALL_SAVES + dispNum);
    if (dispNum == 1) ExtSavedValue = 100 * ExtSavedValue;
    RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2);
    } 
    if (resetDoubleClick) {
    ExtSavedValue = 0;
    RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2); 
    RPU_WriteByteToEEProm(RPU_BALL_SAVES + dispNum, ExtSavedValue);
    }
    if (curSwitch == resetSwitch) {
    ExtSavedValue += (dispNum == 1) ? 100 : 1;
    if (ExtSavedValue > highNum[dispNum]) ExtSavedValue = highNum[dispNum];
    RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2); 
    RPU_WriteByteToEEProm(RPU_BALL_SAVES + dispNum, ExtSavedValue / ((dispNum == 1) ? 100 : 1));
    }
  if (resetBeingHeld) {
    // ExtSavedValue = RPU_ReadByteFromEEProm(RPU_BALL_SAVES + dispNum);
    // if (dispNum == 1) ExtSavedValue = 100 * ExtSavedValue;
    ExtSavedValue += (dispNum == 1) ? 100 : 1;
    if (ExtSavedValue > highNum[dispNum]) ExtSavedValue = highNum[dispNum];
    ExtResetHold = CurrentTime;
    RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2);  
    RPU_WriteByteToEEProm(RPU_BALL_SAVES + dispNum, ExtSavedValue / ((dispNum == 1) ? 100 : 1));
    }
}

// #################### ENTER GAIN DATA USING DISPLAYS ####################
void EnterGainData(byte curSwitch, boolean resetDoubleClick, boolean resetBeingHeld, boolean curStateChanged, 
                            unsigned long CurrentTime, byte resetSwitch, byte otherSwitch, byte endSwitch) {
  unsigned long highNum[3] = {100, 100, 100}; // Maximum values

  if (curStateChanged) {
    UpDown = 1;
    dispNum = 0;
    ExtSavedValue = RPU_ReadByteFromEEProm(RPU_BG_GAIN);
    RPU_SetDisplayFlash(0, ExtSavedValue, CurrentTime, 250, 2);
    ExtSavedValue = RPU_ReadByteFromEEProm(RPU_SFX_GAIN);
    RPU_SetDisplay(1, ExtSavedValue, true, 2);
    ExtSavedValue = RPU_ReadByteFromEEProm(RPU_VOICE_GAIN);
    RPU_SetDisplay(2, ExtSavedValue, true, 2);

    StopAudio();
    PlayBackgroundSong(BGTest, bgGain);
    }
  ExtSavedValue = RPU_ReadByteFromEEProm(RPU_BG_GAIN + dispNum);
  RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2);
  if (curSwitch == otherSwitch) {
    UpDown = 1;
    RPU_SetDisplay(dispNum, ExtSavedValue, true, 2);
    dispNum = (dispNum + 1) % 3;
    ExtSavedValue = RPU_ReadByteFromEEProm(RPU_BG_GAIN + dispNum);
    RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2);

    StopSoundEffect(SFXTest);
    StopSoundEffect(VoiceTest);
    if (dispNum == 1) {
      PlaySoundEffect(SFXTest, sfxGain);
      SetTrackLoop(SFXTest, true);
    }
    if (dispNum == 2) {
      PlaySoundEffect(VoiceTest, voiceGain);
      SetTrackLoop(VoiceTest, true);
    }
    }
    if (resetDoubleClick) {
    UpDown = -UpDown;
    ExtSavedValue += UpDown;
    if (ExtSavedValue < 0) ExtSavedValue = highNum[dispNum];
    if (ExtSavedValue > highNum[dispNum]) ExtSavedValue = 0;
    RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2); 
    RPU_WriteByteToEEProm(RPU_BG_GAIN + dispNum, ExtSavedValue);
    }
    if (curSwitch == resetSwitch) {
    ExtSavedValue += UpDown;
    if (ExtSavedValue < 0) ExtSavedValue = highNum[dispNum];
    if (ExtSavedValue > highNum[dispNum]) ExtSavedValue = 0;
    RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2); 
    RPU_WriteByteToEEProm(RPU_BG_GAIN + dispNum, ExtSavedValue);
    }
  if (resetBeingHeld) {
    ExtSavedValue += UpDown;
    if (ExtSavedValue < 0) ExtSavedValue = highNum[dispNum];
    if (ExtSavedValue > highNum[dispNum]) ExtSavedValue = 0;
    ExtResetHold = CurrentTime;
    RPU_SetDisplayFlash(dispNum, ExtSavedValue, CurrentTime, 250, 2);  
    RPU_WriteByteToEEProm(RPU_BG_GAIN + dispNum, ExtSavedValue);
    }
  if (dispNum == 0) {
    AdjBgGain = ExtSavedValue;
    CalculateGain();
    ResetGain(BGTest, bgGain);
    }
  else if (dispNum == 1) {
    AdjSfxGain = ExtSavedValue;
    CalculateGain();
    ResetGain(SFXTest, sfxGain);
    }
  else if (dispNum == 2) {
    AdjVoiceGain = ExtSavedValue;
    CalculateGain();
    ResetGain(VoiceTest, voiceGain);
    }
}

// #################### CALCULATE GAIN FOR BACKGROUND, SFX, AND VOICE ####################
void CalculateGain() {
// Initial gain values set as reasonable values for gain, and do not change
// Adustments are values from 0 - 100 entered by user, with distance from 50 indicating lower or higher gain
// bgGain, sfxGain, and voiceGain are calculated from Initial and Adjustment values. Calculated gains are in the range of -70 to +10.

  if (AdjBgGain < 0) AdjBgGain = 0;
  if (AdjBgGain > 100) AdjBgGain = 100;
  if (AdjSfxGain < 0) AdjSfxGain = 0;
  if (AdjSfxGain > 100) AdjSfxGain = 100;
  if (AdjVoiceGain < 0) AdjVoiceGain = 0;
  if (AdjVoiceGain > 100) AdjVoiceGain = 100;

  bgGain = InitBgGain;
  if (AdjBgGain > 50) bgGain = InitBgGain + (10 - InitBgGain) * (AdjBgGain - 50) / 50.0;
  if (AdjBgGain < 50) bgGain = InitBgGain - (70 + InitBgGain) * (50 - AdjBgGain) / 50.0;

  sfxGain = InitSfxGain;
  if (AdjSfxGain > 50) sfxGain = InitSfxGain + (10 - InitSfxGain) * (AdjSfxGain - 50) / 50.0;
  if (AdjSfxGain < 50) sfxGain = InitSfxGain - (70 + InitSfxGain) * (50 - AdjSfxGain) / 50.0;

  voiceGain = InitVoiceGain;
  if (AdjVoiceGain > 50) voiceGain = InitVoiceGain + (10 - InitVoiceGain) * (AdjVoiceGain - 50) / 50.0;
  if (AdjVoiceGain < 50) voiceGain = InitVoiceGain - (70 + InitVoiceGain) * (50 - AdjVoiceGain) / 50.0;
}

// #################### RESET ALL VARIABLES FOR A BEGINNER GAME ####################
void KidSettings() { // Set all DIP switches and game settings for beginner mode

  // DIP switches set to easy
  dipHyperSpaceAdjustment = true;
  dipBallySpecialAdjustment = true;
  dipCenterAdjustment = true;
  dipBally10000Adjustment = true;
  dipOutlaneAdjustment = true;
  dipFlipperFeedAdjustment = true;
  dipBallsPerGame = 5;

  // Game settings in memory set to easy
  HighScore            = 50000;
  ScoreAward1          = 25000;
  ScoreAward2          = 75000;
  ScoreAward3          = 150000; 
  PersonalGoal         = 75000;
  dipThresholdAward    = 2;
  ballSaves            = 5;
  ballSaveScore        = 7000;
  ballSaveNumMS        = 20000;

  // Operator game adjustments set to easy:
  // (currently none!)

}



////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////

#define SOUND_STACK_SIZE   10
#define SOUND_STACK_EMPTY  0xFFFF
#define SOUND_EFFECT_NONE               0

SendOnlyWavTrigger wTrig;             // Our WAV Trigger object

byte CurrentBackgroundSong = SOUND_EFFECT_NONE;
int iWT, jWT, kWT;

struct SoundStruct { // define sound structure, to be used in storing data needed to delay and play sounds
  byte sound;
  byte type;
  int  gain;
  unsigned long start;
  unsigned long length;
};

struct SoundStruct SoundQueue[SOUND_STACK_SIZE] = {
{0, 0xFF, 0, 0, 0},
{0, 0xFF, 0, 0, 0},
{0, 0xFF, 0, 0, 0},
{0, 0xFF, 0, 0, 0},
{0, 0xFF, 0, 0, 0},
{0, 0xFF, 0, 0, 0},
{0, 0xFF, 0, 0, 0},
{0, 0xFF, 0, 0, 0},
{0, 0xFF, 0, 0, 0},
{0, 0xFF, 0, 0, 0}
};


void SetupAudio(byte sound = 0) { // Must be run at beginning of program. Will play specified test sound at startup.
  wTrig.start();
  wTrig.stopAllTracks();
  delayMicroseconds(10000);

  StopAudio();
  PlaySound(sound);
}

void StopAudio() {
  wTrig.stopAllTracks();
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
  for (iWT = 0; iWT < SOUND_STACK_SIZE; ++iWT) SoundQueue[iWT].type = 0xFF;
  WTWaitingTime = 0;
}

void StopAllButBG() {
  byte curSong = CurrentBackgroundSong;
  StopAudio();
  PlayBackgroundSong(curSong);
}

void PlayBackgroundSong(byte songNum, int gain = 100) {

  if (gain == 100) gain = bgGain;

  if (CurrentBackgroundSong != songNum) {
    if (CurrentBackgroundSong != SOUND_EFFECT_NONE) wTrig.trackStop(CurrentBackgroundSong);
    if (songNum != SOUND_EFFECT_NONE) {
      wTrig.trackPlayPoly(songNum, true);
      wTrig.trackLoop(songNum, true);
      wTrig.trackGain(songNum, gain);
      CurrentBackgroundSong = songNum;
    }
  }
}

void SetTrackLoop(byte soundNum, boolean enable) {
      wTrig.trackLoop(soundNum, enable);
}

void SetBackgroundSong(byte soundNum) {
  CurrentBackgroundSong = soundNum;
}

void StopBackgroundSong() {
  PlayBackgroundSong(SOUND_EFFECT_NONE);
}

void ResumeBackgroundSong() {
  byte curSong = CurrentBackgroundSong;
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
  PlayBackgroundSong(curSong);
}

inline void StopSoundEffect(byte soundEffectNum) {
  wTrig.trackStop(soundEffectNum);
}

void PlaySoundEffect(byte soundEffectNum, int gain = 100) {
  if (gain == 100) {
    if (SoundTimings[soundEffectNum][0] == 1) gain = bgGain;
    else if (SoundTimings[soundEffectNum][0] == 2) gain = voiceGain;
    else if (SoundTimings[soundEffectNum][0] == 3) gain = sfxGain;
  }
  wTrig.trackPlayPoly(soundEffectNum);
  wTrig.trackGain(soundEffectNum, gain);
}

void ResetGain(byte soundEffectNum, int gain) {
  wTrig.trackGain(soundEffectNum, gain);
}


// #################### PLAY ANY TYPE OF SOUND, OR ADD IT TO THE QUEUE IF NECESSARY ####################
void PlaySound(byte soundByte, unsigned long delaySound = 0, int gain = 100) {

// Specify number of sound to play (see SoundTimings list of sounds), 
//    delay until sound played (in ms, default zero), 
//    and gain (like volume but not quite. Separate defaults for background, voice, and special effects).
//
// Note: Gain is specified from -70 to +10, and is used to change RELATIVE volumes of different sounds.
//       Negative values are better, as adding multiple loud sounds together with high gains will lose the high and low ends of the sounds.

  if (gain == 100) {
    if (SoundTimings[soundByte][0] == 1) gain = bgGain;
    else if (SoundTimings[soundByte][0] == 2) gain = voiceGain;
    else if (SoundTimings[soundByte][0] == 3) gain = sfxGain;
  }
  
  if (SoundTimings[soundByte][0] == 0) return;                // Invalid sound
  if (SoundTimings[soundByte][0] == 1 && delaySound == 0) {   // Play background music immediately, unless delayed
    PlayBackgroundSong(soundByte, gain);
    return;
  }
  if (SoundTimings[soundByte][0] == 2 && delaySound == 0 && WTWaitingTime < CurrentTime) {   // Play voice immediately, unless delayed or another voice playing
    PlaySoundEffect(soundByte, gain);
    WTWaitingTime = CurrentTime + 100 * (unsigned long) SoundTimings[soundByte][1];
    return;
  }
  if (SoundTimings[soundByte][0] == 3 && delaySound == 0) {   // Play sound effect immediately, unless delayed
    PlaySoundEffect(soundByte, gain);
    return;
  }
  kWT = 99;
  for (iWT = 0; iWT < SOUND_STACK_SIZE; ++iWT) {                    // Find space in queue
    if (SoundQueue[iWT].type == 0xFF) {
      kWT = iWT;
      iWT = 99;
    }
  }
  if (kWT >= SOUND_STACK_SIZE) kWT = 0;                           // Add new sound to queue. Write over oldest value if queue full :(

  SoundQueue[kWT].sound  = soundByte;
  SoundQueue[kWT].type   = SoundTimings[soundByte][0];
  SoundQueue[kWT].gain   = gain;
  SoundQueue[kWT].start  = CurrentTime + delaySound;
  SoundQueue[kWT].length = 100 * (unsigned long) SoundTimings[soundByte][1];
}

// #################### CHECK IF A SOUND IS QUEUED AND READY TO PLAY ####################
void PlayQueue() {

// Run this every time through loop(). It looks for sounds set up with delays through PlaySound(), and plays them when appropriate.

  kWT = 99;
  for (iWT = 0; iWT < SOUND_STACK_SIZE; ++iWT){                             // Find next sound to be played
    if (SoundQueue[iWT].type != 0xFF) {                                     // Look for valid sound type. 0xFF indicates empty row of queue.
      if (CurrentTime >= SoundQueue[iWT].start) {                           // This sound is ready to be played
        if (SoundQueue[iWT].type == 1) {                                    // Background sounds always play at requested start time
          PlayBackgroundSong(SoundQueue[iWT].sound);
          wTrig.trackGain(SoundQueue[iWT].sound, SoundQueue[iWT].gain);
          SoundQueue[iWT].type = 0xFF;
          kWT = iWT;
          iWT = 99;
        }
        else if (SoundQueue[iWT].type == 2 && CurrentTime >= WTWaitingTime) { // Voices need to wait for other voices to finish
          PlaySoundEffect(SoundQueue[iWT].sound);
          wTrig.trackGain(SoundQueue[iWT].sound, SoundQueue[iWT].gain);
          WTWaitingTime = CurrentTime + SoundQueue[iWT].length;
          SoundQueue[iWT].type = 0xFF;
          kWT = iWT;
          iWT = 99;
        }
        else if (SoundQueue[iWT].type == 3) {                              // Sound effects can play at requested start time
          PlaySoundEffect(SoundQueue[iWT].sound);
          wTrig.trackGain(SoundQueue[iWT].sound, SoundQueue[iWT].gain);
          SoundQueue[iWT].type = 0xFF;
          kWT = iWT;
          iWT = 99;
        }
      }
    }
  }
  if (kWT < SOUND_STACK_SIZE - 1) {                                        // Remove played sound from stack. Compress stack.
    for (iWT = kWT + 1; iWT < SOUND_STACK_SIZE; ++iWT) {
      SoundQueue[iWT - 1].sound    = SoundQueue[iWT].sound;
      SoundQueue[iWT - 1].type     = SoundQueue[iWT].type;
      SoundQueue[iWT - 1].gain     = SoundQueue[iWT].gain;
      SoundQueue[iWT - 1].start    = SoundQueue[iWT].start;
      SoundQueue[iWT - 1].length   = SoundQueue[iWT].length;
    }
    SoundQueue[SOUND_STACK_SIZE - 1].type = 0xFF;
  }
}