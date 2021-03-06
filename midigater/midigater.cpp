/*------------------------------------------------------------------------
Copyright (C) 2001-2009  Sophia Poirier

This file is part of MIDI Gater.

MIDI Gater is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

MIDI Gater is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with MIDI Gater.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "midigater.h"


//----------------------------------------------------------------------------- 
// constants
const long kUnaffectedFadeDur = 18;
const float kUnaffectedFadeStep = 1.0f / (float)kUnaffectedFadeDur;

const bool kUseNiceAudioFades = true;
const bool kUseLegato = false;

// these are the 3 states of the unaffected audio input between notes
enum
{
	kUnaffectedState_FadeIn,
	kUnaffectedState_Flat,
	kUnaffectedState_FadeOut
};


// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(MidiGater)


//-----------------------------------------------------------------------------------------
// initializations
MidiGater::MidiGater(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, kNumParameters, 1)	// 4 parameters, 1 preset
{
	initparameter_f(kAttackSlope, "attack", 3.0, 3.0, 0.0, 3000.0, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_f(kReleaseSlope, "release", 3.0, 3.0, 0.0, 3000.0, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_f(kVelInfluence, "velocity influence", 0.0, 1.0, 0.0, 1.0, kDfxParamUnit_scalar);
	initparameter_f(kFloor, "floor", 0.0, 0.0, 0.0, 1.0, kDfxParamUnit_lineargain, kDfxParamCurve_cubed);

	setpresetname(0, "push the button");	// default preset name

	setAudioProcessingMustAccumulate(true);	// only support accumulating output
	midistuff->setLazyAttack();	// this enables the lazy note attack mode
}

//-----------------------------------------------------------------------------------------
void MidiGater::reset()
{
	// reset the unaffected between-audio stuff
	unaffectedState = kUnaffectedState_FadeIn;
	unaffectedFadeSamples = 0;
}

//-----------------------------------------------------------------------------------------
void MidiGater::processparameters()
{
	attackSlope_seconds = getparameter_f(kAttackSlope) * 0.001;
	releaseSlope_seconds = getparameter_f(kReleaseSlope) * 0.001;
	velInfluence = getparameter_f(kVelInfluence);
	floor = getparameter_f(kFloor);
}


//-----------------------------------------------------------------------------------------
void MidiGater::processaudio(const float ** inAudio, float ** outAudio, unsigned long inNumFrames, bool inReplacing)
{
	unsigned long numChannels = getnumoutputs();
	long numFramesToProcess = (signed)inNumFrames, totalSampleFrames = (signed)inNumFrames;	// for dividing up the block accoring to events


	// counter for the number of MIDI events this block
	// start at -1 because the beginning stuff has to happen
	long eventcount = -1;
	long currentBlockPosition = 0;	// we are at sample 0

	// now we're ready to start looking at MIDI messages and processing sound and such
	do
	{
		// check for an upcoming event and decrease this block chunk size accordingly 
		// if there will be another event
		if ( (eventcount+1) >= midistuff->numBlockEvents )
			numFramesToProcess = totalSampleFrames - currentBlockPosition;
		// else this chunk goes to the end of the block
		else
			numFramesToProcess = midistuff->blockEvents[eventcount+1].delta - currentBlockPosition;

		// this means that 2 (or more) events occur simultaneously, 
		// so there's no need to do calculations during this round
		if (numFramesToProcess == 0)
		{
			eventcount++;
			// take in the effects of the next event
			midistuff->heedEvents(eventcount, getsamplerate_f(), 0.0f, attackSlope_seconds, 
									releaseSlope_seconds, kUseLegato, 1.0f, velInfluence);
			continue;
		}

		// test for whether or not all notes are off and unprocessed audio can be outputted
		bool noNotes = true;	// none yet for this chunk

		for (int notecount=0; notecount < NUM_NOTES; notecount++)
		{
			// only go into the output processing cycle if a note is happening
			if (midistuff->noteTable[notecount].velocity)
			{
				noNotes = false;	// we have a note
				for (long samplecount = currentBlockPosition; 
						samplecount < numFramesToProcess+currentBlockPosition; samplecount++)
				{
					// see whether attack or release are active and fetch the output scalar
					float envAmp = midistuff->processEnvelope(kUseNiceAudioFades, notecount);	// the attack/release scalar
					envAmp *=  midistuff->noteTable[notecount].noteAmp;	// scale by key velocity
					for (unsigned long ch=0; ch < numChannels; ch++)
						outAudio[ch][samplecount] += inAudio[ch][samplecount] * envAmp;
				}
			}
		}	// end of notes loop


		// we had notes this chunk, but the unaffected processing hasn't faded out, so change its state to fade-out
		if ( !noNotes && (unaffectedState == kUnaffectedState_Flat) )
			unaffectedState = kUnaffectedState_FadeOut;

		// we can output unprocessed audio if no notes happened during this block chunk
		// or if the unaffected fade-out still needs to be finished
		if ( noNotes || (unaffectedState == kUnaffectedState_FadeOut) )
			processUnaffected(inAudio, outAudio, numFramesToProcess, currentBlockPosition, numChannels);

		eventcount++;
		// don't do the event processing below if there are no more events
		if (eventcount >= midistuff->numBlockEvents)
			continue;

		// jump our position value forward
		currentBlockPosition = midistuff->blockEvents[eventcount].delta;

		// take in the effects of the next event
		midistuff->heedEvents(eventcount, getsamplerate_f(), 0.0f, attackSlope_seconds, 
								releaseSlope_seconds, kUseLegato, 1.0f, velInfluence);

	} while (eventcount < midistuff->numBlockEvents);
}

//-----------------------------------------------------------------------------------------
// this function outputs the unprocessed audio input between notes, if desired
void MidiGater::processUnaffected(const float ** inAudio, float ** outAudio, long inNumFramesToProcess, long inOffset, unsigned long inNumChannels)
{
	long endPos = inNumFramesToProcess + inOffset;
	for (long samplecount=inOffset; samplecount < endPos; samplecount++)
	{
		float sampleAmp = floor;

		// this is the state when all notes just ended and the clean input first kicks in
		if (unaffectedState == kUnaffectedState_FadeIn)
		{
			// linear fade-in
			sampleAmp = (float)unaffectedFadeSamples * kUnaffectedFadeStep * floor;
			unaffectedFadeSamples++;
			// go to the no-gain state if the fade-in is done
			if (unaffectedFadeSamples >= kUnaffectedFadeDur)
				unaffectedState = kUnaffectedState_Flat;
		}
		// a note has just begun, so we need to hasily fade out the clean input audio
		else if (unaffectedState == kUnaffectedState_FadeOut)
		{
			unaffectedFadeSamples--;
			// linear fade-out
			sampleAmp = (float)unaffectedFadeSamples * kUnaffectedFadeStep * floor;
			// get ready for the next time and exit this function if the fade-out is done
			if (unaffectedFadeSamples <= 0)
			{
				// ready for the next time
				unaffectedState = kUnaffectedState_FadeIn;
				return;	// important!  leave this function or a new fade-in will begin
			}
		}

		for (unsigned long ch=0; ch < inNumChannels; ch++)
			outAudio[ch][samplecount] += inAudio[ch][samplecount] * sampleAmp;
	}
}
