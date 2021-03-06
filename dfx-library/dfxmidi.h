/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2015  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Sophia's Destroy FX MIDI stuff
---------------------------------------------------------------*/

#ifndef __DFX_MIDI_H
#define __DFX_MIDI_H


#include "dfxenvelope.h"


//----------------------------------------------------------------------------- 
// enums

// these are the MIDI event status types
enum
{
	kMidiNoteOff = 0x80,
	kMidiNoteOn = 0x90,
	kMidiPolyphonicAftertouch = 0xA0,
	kMidiCC = 0xB0,
	kMidiProgramChange = 0xC0,
	kMidiChannelAftertouch = 0xD0,
	kMidiPitchbend = 0xE0,

	kMidiSysEx = 0xF0,
	kMidiTimeCode = 0xF1,
	kMidiSongPositionPointer = 0xF2,
	kMidiSongSelect = 0xF3,
	kMidiTuneRequest = 0xF6,
	kMidiEndOfSysex = 0xF7,
	kMidiTimingClock = 0xF8,
	kMidiStart = 0xFA,
	kMidiContinue = 0xFB,
	kMidiStop = 0xFC,
	kMidiActiveSensing = 0xFE,
	kMidiSystemReset = 0xFF,

	kInvalidMidi = -3	// for whatever
};

// these are the MIDI continuous controller messages (CCs)
enum
{
	kMidiCC_BankSelect = 0x00,
	kMidiCC_ModWheel = 0x01,
	kMidiCC_BreathControl = 0x02,
	kMidiCC_FootControl = 0x04,
	kMidiCC_PortamentoTime = 0x05,
	kMidiCC_DataEntry = 0x06,
	kMidiCC_ChannelVolume = 0x07,
	kMidiCC_Balance = 0x08,
	kMidiCC_Pan = 0x0A,
	kMidiCC_ExpressionController = 0x0B,
	kMidiCC_EffectControl1 = 0x0C,
	kMidiCC_EffectControl2 = 0x0D,
	kMidiCC_GeneralPurposeController1 = 0x10,
	kMidiCC_GeneralPurposeController2 = 0x11,
	kMidiCC_GeneralPurposeController3 = 0x12,
	kMidiCC_GeneralPurposeController4 = 0x13,

	// on/off CCs   ( <= 63 is off, >= 64 is on )
	kMidiCC_SustainPedalOnOff = 0x40,
	kMidiCC_PortamentoOnOff = 0x41,
	kMidiCC_SustenutoOnOff = 0x42,
	kMidiCC_SoftPedalOnOff = 0x43,
	kMidiCC_LegatoFootswitch = 0x44,
	kMidiCC_Hold2 = 0x45,

	kMidiCC_SoundController1_soundVariation = 0x46,
	kMidiCC_SoundController2_timbre = 0x47,
	kMidiCC_SoundController3_releaseTime = 0x48,
	kMidiCC_SoundController4_attackTime = 0x49,
	kMidiCC_SoundController5_brightness = 0x4A,
	kMidiCC_SoundController6 = 0x4B,
	kMidiCC_SoundController7 = 0x4C,
	kMidiCC_SoundController8 = 0x4D,
	kMidiCC_SoundController9 = 0x4E,
	kMidiCC_SoundController10 = 0x4F,
	kMidiCC_GeneralPurposeController5 = 0x50,
	kMidiCC_GeneralPurposeController6 = 0x51,
	kMidiCC_GeneralPurposeController7 = 0x52,
	kMidiCC_GeneralPurposeController8 = 0x53,
	kMidiCC_PortamentoControl = 0x54,
	kMidiCC_Effects1Depth = 0x5B,
	kMidiCC_Effects2Depth = 0x5C,
	kMidiCC_Effects3Depth = 0x5D,
	kMidiCC_Effects4Depth = 0x5E,
	kMidiCC_Effects5Depth = 0x5F,
	kMidiCC_DataEntryPlus1 = 0x60,
	kMidiCC_DataEntryMinus1 = 0x61,

	// sepcial commands
	kMidiCC_AllSoundOff = 0x78,	// 0 only
	kMidiCC_ResetAllControllers = 0x79,	// 0 only
	kMidiCC_LocalControlOnOff = 0x7A,	// 0 = off, 127 = on
	kMidiCC_AllNotesOff = 0x7B,	// 0 only
	kMidiCC_OmniModeOff = 0x7C,	// 0 only
	kMidiCC_OmniModeOn = 0x7D,	// 0 only
	kMidiCC_PolyModeOnOff = 0x7E,
	kMidiCC_PolyModeOn = 0x7F	// 0 only
};

//----------------------------------------------------------------------------- 
// constants and macros

const long kDfxMidi_PitchbendMiddleValue = 0x2000;
const double PITCHBEND_MAX = 36.0;

// 128 MIDI notes
const long NUM_NOTES = 128;
// 12th root of 2
const double NOTE_UP_SCALAR = 1.059463094359295264561825294946;	// XXX use exact math?
const double NOTE_DOWN_SCALAR = 0.94387431268169349664191315666792;	// XXX use exact math?
const float MIDI_SCALAR = 1.0f / 127.0f;

const long STOLEN_NOTE_FADE_DUR = 48;
const float STOLEN_NOTE_FADE_STEP = 1.0f / (float)STOLEN_NOTE_FADE_DUR;
const long LEGATO_FADE_DUR = 39;
const float LEGATO_FADE_STEP = 1.0f / LEGATO_FADE_DUR;

const long EVENTS_QUEUE_SIZE = 12000;

inline bool isNote(int inMidiStatus)
{
	return (inMidiStatus == kMidiNoteOn) || (inMidiStatus == kMidiNoteOff);
}


//----------------------------------------------------------------------------- 
// types

// this holds MIDI event information
typedef struct {
	int status;	// the event status MIDI byte
	int byte1;	// the first MIDI data byte
	int byte2;	// the second MIDI data byte
	long delta;	// the delta offset (the sample position in the current block where the event occurs)
	int channel;	// the MIDI channel
} DfxMidiEvent;


//-----------------------------------------------------------------------------
// this holds information for each MIDI note
class DfxMusicNote
{
public:
	DfxMusicNote();
	~DfxMusicNote();

	void clearTail();	// zero out a note's tail buffers

	int mVelocity;	// note velocity - 7-bit MIDI value
	float mNoteAmp;	// the gain for the note, scaled with velocity, curve, and influence
	DfxEnvelope mEnvelope;
	float mLastOutValue;	// capture the most recent output value for smoothing, if necessary   XXX mono assumption
	long mSmoothSamples;	// counter for quickly fading cut-off notes, for smoothity
	float * mTail1;	// a little buffer of output samples for smoothing a cut-off note (left channel)
	float * mTail2;	// (right channel)	XXX wow this stereo assumption is such a bad idea
};


//-----------------------------------------------------------------------------
class DfxMidi
{
public:
	DfxMidi();
	~DfxMidi();

	void reset();	// resets the variables
	void setSampleRate(double inSampleRate);
	void setEnvParameters(double inAttackDur, double inDecayDur, double inSustainLevel, double inReleaseDur);
	void setEnvCurveType(DfxEnvCurveType inCurveType);
	void setResumedAttackMode(bool inNewMode);

	bool incNumEvents();	// increment the numBlockEvents counter, safely

	// handlers for the types of MIDI events that we support
	void handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset);
	void handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset);
	void handleAllNotesOff(int inMidiChannel, long inBufferOffset);
	void handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, long inBufferOffset);
	void handleCC(int inMidiChannel, int inControllerNumber, int inValue, long inBufferOffset);
	void handleProgramChange(int inMidiChannel, int inProgramNumber, long inBufferOffset);

	void preprocessEvents();
	void postprocessEvents();

	// this is where new MIDI events are reckoned with during audio processing
	void heedEvents(long inEventNum, double inPitchbendRange, bool inLegato, 
					float inVelocityCurve, float inVelocityInfluence);

	// these are for manage the ordered queue of active MIDI notes
	void insertNote(int inCurrentNote);
	void removeNote(int inCurrentNote);
	void removeAllNotes();

	// public variables
	DfxMusicNote * noteTable;	// a table with important data about each note
	DfxMidiEvent * blockEvents;	// the new MIDI events for a given processing block
	long numBlockEvents;	// the number of new MIDI events in a given processing block
	int * noteQueue;		// a chronologically ordered queue of all active notes
	double * freqTable;	// a table of the frequency corresponding to each MIDI note
	double pitchbend;		// a frequency scalar value for the current pitchbend setting

	// this function calculates fade scalars if attack, decay, or release are happening
	float processEnvelope(int inCurrentNote);

	// this function writes the audio output for smoothing the tips of cut-off notes
	// by sloping down from the last sample outputted by the note
	void processSmoothingOutputSample(float * outAudio, long inNumSamples, int inCurrentNote);

	// this function writes the audio output for smoothing the tips of cut-off notes
	// by fading out the samples stored in the tail buffers
	void processSmoothingOutputBuffer(float * outAudio, long inNumSamples, int inCurrentNote, int inMidiChannel);


private:
	// initializations
	void fillFrequencyTable();

	void turnOffNote(int inCurrentNote, bool inLegato);

	// a queue of note-offs for when the sustain pedal is active
	bool * sustainQueue;

	// pick up where the release left off, if it's still releasing
//	bool lazyAttackMode;
	// sustain pedal is active
	bool sustain;
};



//-------------------------------------------------------------------------
// this function calculates fade scalars if attack, decay, or release are happening
inline float DfxMidi::processEnvelope(int inCurrentNote)
{
	const float outputAmp = noteTable[inCurrentNote].mEnvelope.process();
	if (noteTable[inCurrentNote].mEnvelope.getState() == kDfxEnvState_Dormant)
		noteTable[inCurrentNote].mVelocity = 0;

	return outputAmp;
}


//-------------------------------------------------------------------------
// this function writes the audio output for smoothing the tips of cut-off notes
// by sloping down from the last sample outputted by the note
inline void DfxMidi::processSmoothingOutputSample(float * outAudio, long inNumSamples, int inCurrentNote)
{
	for (long samplecount=0; samplecount < inNumSamples; samplecount++)
	{
		// add the latest sample to the output collection, scaled by the note envelope and user gain
		float outputFadeScalar = (float)noteTable[inCurrentNote].mSmoothSamples * STOLEN_NOTE_FADE_STEP;
		outputFadeScalar = outputFadeScalar * outputFadeScalar * outputFadeScalar;
		outAudio[samplecount] += noteTable[inCurrentNote].mLastOutValue * outputFadeScalar;
		// decrement the smoothing counter
		(noteTable[inCurrentNote].mSmoothSamples)--;
		// exit this function if we've done all of the smoothing necessary
		if (noteTable[inCurrentNote].mSmoothSamples <= 0)
			return;
	}
}


//-------------------------------------------------------------------------
// this function writes the audio output for smoothing the tips of cut-off notes
// by fading out the samples stored in the tail buffers
inline void DfxMidi::processSmoothingOutputBuffer(float * outAudio, long inNumSamples, int inCurrentNote, int inMidiChannel)
{
	long * smoothsamples = &(noteTable[inCurrentNote].mSmoothSamples);
	float * tail = (inMidiChannel == 1) ? noteTable[inCurrentNote].mTail1 : noteTable[inCurrentNote].mTail2;

	for (long samplecount=0; samplecount < inNumSamples; samplecount++)
	{
		outAudio[samplecount] += tail[STOLEN_NOTE_FADE_DUR-(*smoothsamples)] * 
							(float)(*smoothsamples) * STOLEN_NOTE_FADE_STEP;
		(*smoothsamples)--;
		if (*smoothsamples <= 0)
			return;
	}
}



#endif
