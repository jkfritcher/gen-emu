/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          Sound.c                        **/
/**                                                         **/
/** This file file implements core part of the sound API    **/
/** and functions needed to log soundtrack into a MIDI      **/
/** file. See Sound.h for declarations.                     **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2002                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Sound.h"

#include <stdio.h>

typedef unsigned char byte;
typedef unsigned short word;

struct SndDriverStruct SndDriver =
{
  (void (*)(int,int))0,
  (void (*)(int,int))0,
  (void (*)(int,int))0,
  (void (*)(int,int,int))0,
  (void (*)(int,signed char *,int,int))0
};

static struct { byte Note;word Wheel; } Freqs[4096] =
{
#include "MIDIFreq.h"
};

static struct
{
  int Type;
  int TChange;
  int Note;
  int Pitch;
  int Level;
} CH[MIDI_CHANNELS];

static char *LogName  = 0;
static int  Logging   = MIDI_OFF;
static int  TickCount = 0;
static int  LastMsg   = -1;

static void MIDISound(int Channel,int Freq,int Volume);
static void MIDISetSound(int Channel,int Type);
static void MIDIDrum(int Type,int Force);
static void MIDIMessage(byte D0,byte D1,byte D2);
static void NoteOn(byte Channel,byte Note,byte Level);
static void NoteOff(byte Channel);
static void WriteDelta(void);
static void WriteTempo(int Freq);

/** SHIFT() **************************************************/
/** Make MIDI channel#10 last, as it is normally used for   **/
/** percussion instruments only and doesn't sound nice.     **/
/*************************************************************/
#define SHIFT(Ch) (Ch==15? 9:Ch>8? Ch+1:Ch)

/** Sound() **************************************************/
/** Generate sound of given frequency (Hz) and volume       **/
/** (0..255) via given channel. Setting Freq=0 or Volume=0  **/
/** turns sound off.                                        **/
/*************************************************************/
void Sound(int Channel,int Freq,int Volume)
{
  if(Channel<0) return;
  Freq   = Freq<0? 0:Freq;
  Volume = Volume<0? 0:Volume>255? 255:Volume;

  /* Call sound driver if present */
  if(SndDriver.Sound) (*SndDriver.Sound)(Channel,Freq,Volume);
}

/** Drum() ***************************************************/
/** Hit a drum of given type with given force (0..255).     **/
/** MIDI drums can be used by ORing their numbers with      **/
/** SND_MIDI.                                               **/
/*************************************************************/
void Drum(int Type,int Force)
{
  Force = Force<0? 0:Force>255? 255:Force;

  if(SndDriver.Drum) (*SndDriver.Drum)(Type,Force);
}

/** SetSound() ***********************************************/
/** Set sound type at a given channel. MIDI instruments can **/
/** be set directly by ORing their numbers with SND_MIDI.   **/
/*************************************************************/
void SetSound(int Channel,int Type)
{
  if(Channel<0) return;

  if(SndDriver.SetSound) (*SndDriver.SetSound)(Channel,Type);
}

/** SetChannels() ********************************************/
/** Set master volume (0..255) and switch channels on/off.  **/
/** Each channel N has corresponding bit 2^N in Switch. Set **/
/** or reset this bit to turn the channel on or off.        **/ 
/*************************************************************/
void SetChannels(int Volume,int Switch)
{
  Volume = Volume<0? 0:Volume>255? 255:Volume;

  if(SndDriver.SetChannels) (*SndDriver.SetChannels)(Volume,Switch);
}

/** SetWave() ************************************************/
/** Set waveform for a given channel. The channel will be   **/
/** marked with sound type SND_WAVE. Set Rate=0 if you want **/
/** waveform to be an instrument or set it to the waveform  **/
/** own playback rate.                                      **/
/*************************************************************/
void SetWave(int Channel,signed char *Data,int Length,int Rate)
{
  if((Channel<0)||(Length<=0)) return;

  if(SndDriver.SetWave) (*SndDriver.SetWave)(Channel,Data,Length,Rate);
}

/** InitMIDI() ***********************************************/
/** Initialize soundtrack logging into MIDI file FileName.  **/
/** Repeated calls to InitMIDI() will close current MIDI    **/
/** file and continue logging into a new one.               **/ 
/*************************************************************/
void InitMIDI(char *FileName)
{
  return;
}

/** TrashMIDI() **********************************************/
/** Finish logging soundtrack and close the MIDI file.      **/
/*************************************************************/
void TrashMIDI(void)
{
  return;
}

/** MIDILogging() ********************************************/
/** Turn soundtrack logging on/off and return its current   **/
/** status. Possible values of Switch are MIDI_OFF (turn    **/
/** logging off), MIDI_ON (turn logging on), MIDI_TOGGLE    **/
/** (toggle logging), and MIDI_QUERY (just return current   **/
/** state of logging).                                      **/
/*************************************************************/
int MIDILogging(int Switch)
{
  return MIDI_OFF;
}

/** MIDITicks() **********************************************/
/** Log N 1ms MIDI ticks.                                   **/
/*************************************************************/
void MIDITicks(int N)
{
}

/** MIDISound() **********************************************/
/** Set sound frequency (Hz) and volume (0..255) for a      **/
/** given channel.                                          **/
/*************************************************************/
void MIDISound(int Channel,int Freq,int Volume)
{
}

/** MIDISetSound() *******************************************/
/** Set sound type for a given channel.                     **/
/*************************************************************/
void MIDISetSound(int Channel,int Type)
{
}

/** MIDIDrum() ***********************************************/
/** Hit a drum of a given type with given force.            **/
/*************************************************************/
void MIDIDrum(int Type,int Force)
{
}

/** MIDIMessage() ********************************************/
/** Write out a MIDI message.                               **/
/*************************************************************/
void MIDIMessage(byte D0,byte D1,byte D2)
{
}

/** NoteOn() *************************************************/
/** Turn on a note on a given channel.                      **/
/*************************************************************/
void NoteOn(byte Channel,byte Note,byte Level)
{
}

/** NoteOff() ************************************************/
/** Turn off a note on a given channel.                     **/
/*************************************************************/
void NoteOff(byte Channel)
{
}

/** WriteDelta() *********************************************/
/** Write number of ticks since the last MIDI command and   **/
/** reset the counter.                                      **/
/*************************************************************/
void WriteDelta(void)
{
}

/** WriteTempo() *********************************************/
/** Write out soundtrack tempo (Hz).                        **/
/*************************************************************/
void WriteTempo(int Freq)
{
}

