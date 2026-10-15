// A minimal game in the older RPU self-test adjustment style, for the scan tests.
#define MACHINE_STATE_ADJUST_FREEPLAY        -21
#define MACHINE_STATE_ADJUST_BALL_SAVE       -22
#define MACHINE_STATE_ADJUST_MUSIC_VOLUME    -23
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING -24
#define MACHINE_STATE_ADJUST_BALLS_OVERRIDE  -25
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD -26
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE  -28
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD   -29
#define MACHINE_STATE_ADJUST_SCROLLING_SCORES -30
#define MACHINE_STATE_ADJUST_DONE            -27
#define SW_LETTERS_EXPIRATION_BASE        60000
#define SW_OUTHOLE                        8

#define EEPROM_BALL_SAVE_BYTE           100
#define EEPROM_FREE_PLAY_BYTE           101
#define EEPROM_MUSIC_VOLUME_BYTE        109
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_BALLS_OVERRIDE_BYTE      106
#define EEPROM_EXTRA_BALL_SCORE_UL      140
#define EEPROM_SPECIAL_SCORE_UL         144
#define EEPROM_AWARD_OVERRIDE_BYTE      105
#define EEPROM_SCROLLING_SCORES_BYTE    110

#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5

boolean FreePlayMode = false;
byte BallSaveNumSeconds = 0;
byte MusicVolume = 10;
boolean TournamentScoring = false;
byte BallsPerGame = 3;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
byte ScoreAwardReplay = 0;
boolean ScrollingScores = true;

byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
byte CurrentAdjustmentStorageByte = 0;

byte ReadSetting(byte setting, byte defaultValue) {
  byte value = EEPROM.read(setting);
  if (value == 0xFF) { EEPROM.write(setting, defaultValue); return defaultValue; }
  return value;
}

void ReadStoredParameters() {
  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;
  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
  MusicVolume = ReadSetting(EEPROM_MUSIC_VOLUME_BYTE, 8);
  TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) ? true : false;
  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  ExtraBallValue = RPU_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_UL, 20000);
  SpecialValue = RPU_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_UL);
  if (SpecialValue % 1000 || SpecialValue > 100000) SpecialValue = 40000;
  byte awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
  ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) ? true : false;
}

int RunSelfTest(int curState, boolean curStateChanged) {
  if (curStateChanged) {
    CurrentAdjustmentByte = NULL;
    CurrentAdjustmentUL = NULL;
    CurrentAdjustmentStorageByte = 0;

    AdjustmentType = ADJ_TYPE_MIN_MAX;
    AdjustmentValues[0] = 0;
    AdjustmentValues[1] = 1;

    switch (curState) {
      case MACHINE_STATE_ADJUST_FREEPLAY:
        CurrentAdjustmentByte = (byte *)&FreePlayMode;
        CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
        break;
      case MACHINE_STATE_ADJUST_BALL_SAVE:
        AdjustmentType = ADJ_TYPE_LIST;
        NumAdjustmentValues = 5;
        AdjustmentValues[1] = 5;
        AdjustmentValues[2] = 10;
        AdjustmentValues[3] = 15;
        AdjustmentValues[4] = 20;
        CurrentAdjustmentByte = &BallSaveNumSeconds;
        CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
        break;
      case MACHINE_STATE_ADJUST_MUSIC_VOLUME:
        AdjustmentValues[1] = 10;
        CurrentAdjustmentByte = &MusicVolume;
        CurrentAdjustmentStorageByte = EEPROM_MUSIC_VOLUME_BYTE;
        break;
      case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
        CurrentAdjustmentByte = (byte *)&TournamentScoring;
        CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
        break;
      case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
        AdjustmentType = ADJ_TYPE_LIST;
        NumAdjustmentValues = 3;
        AdjustmentValues[0] = 3;
        AdjustmentValues[1] = 5;
        AdjustmentValues[2] = 99;
        CurrentAdjustmentByte = &BallsPerGame;
        CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
        break;
      case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
        AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
        CurrentAdjustmentUL = &ExtraBallValue;
        CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_UL;
        break;
      case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
        AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
        AdjustmentValues[1] = 7;
        CurrentAdjustmentByte = &ScoreAwardReplay;
        CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
        break;
      case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
        AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
        CurrentAdjustmentUL = &SpecialValue;
        CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_UL;
        break;
      case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
        CurrentAdjustmentByte = (byte *)&ScrollingScores;
        CurrentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
        break;
      case MACHINE_STATE_ADJUST_DONE:
        break;
    }
  }
  return curState;
}

void setup() {}
void loop() {}
