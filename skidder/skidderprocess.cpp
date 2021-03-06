/*-------------- by Sophia Poirier  ][  December 2000 -------------*/

#include "skidder.h"


//-----------------------------------------------------------------------------------------
void Skidder::processSlopeIn()
{
	float baseSlopeAmp = ((float)(slopeDur-slopeSamples)) * slopeStep;
	baseSlopeAmp *= baseSlopeAmp;	// square-scale the gain scalar

	// dividing the growing slopeDur-slopeSamples by slopeDur makes ascending values
	if (MIDIin)
	{
		if (midiMode == kMidiMode_trigger)
			// start from a 0.0 floor if we are coming in from silence
			sampleAmp = baseSlopeAmp;
		else if (midiMode == kMidiMode_apply)
			// no fade-in for the first entry of MIDI apply
			sampleAmp = 1.0f;
	}
	else if (useRandomFloor)
		sampleAmp = (baseSlopeAmp * randomGainRange) + randomFloor;
	else
		sampleAmp = (baseSlopeAmp * gainRange) + floor;

	slopeSamples--;

	if (slopeSamples <= 0)
	{
		state = kSkidState_Plateau;
		MIDIin = false;	// make sure it doesn't happen again
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processPlateau()
{
	MIDIin = false;	// in case there was no slope-in

	// sampleAmp in the plateau is 1.0, i.e. this sample is unaffected
	sampleAmp = 1.0f;

	plateauSamples--;

	if (plateauSamples <= 0)
	{
#ifdef USE_BACKWARDS_RMS
		// average and then sqare the sample squareroots for the RMS value
		rms = pow( (rms/(double)rmscount), 2.0 );
#else
		// average and then get the sqare root of the squared samples for the RMS value
		rms = sqrt( rms / (double)rmscount );
#endif
		// because RMS tends to be < 0.5, thus unfairly limiting rupture's range
		rms *= 2.0;
		// avoids clipping or illegit values (like from wraparound)
		if ( (rms > 1.0) || (rms < 0.0) )
			rms = 1.0;
		rmscount = 0;	// reset the RMS counter
		//
		// set up the random floor values
		randomFloor = (float) expandparametervalue(kFloor, DFX_InterpolateRandom(floorRandMin_gen, floor_gen));
		randomGainRange = 1.0f - randomFloor;	// the range of the skidding on/off gain
		//
		if (slopeDur > 0)
		{
			state = kSkidState_SlopeOut;
			slopeSamples = slopeDur;	// refill slopeSamples
			slopeStep = 1.0f / (float)slopeDur;	// calculate the fade increment scalar
		}
		else
			state = kSkidState_Valley;
	}
}


//-----------------------------------------------------------------------------------------
void Skidder::processSlopeOut()
{
	float baseSlopeAmp = (float)slopeSamples * slopeStep;
	baseSlopeAmp *= baseSlopeAmp;	// square-scale the gain scalar

	// dividing the decrementing slopeSamples by slopeDur makes descending values
	if ( MIDIout && (midiMode == kMidiMode_trigger) )
		// start from a 0.0 floor if we are coming in from silence
		sampleAmp = baseSlopeAmp;
	else if (useRandomFloor)
		sampleAmp = (baseSlopeAmp * randomGainRange) + randomFloor;
	else
		sampleAmp = (baseSlopeAmp * gainRange) + floor;

	slopeSamples--;

	if (slopeSamples <= 0)
	{
		state = kSkidState_Valley;
		MIDIout = false;	// make sure it doesn't happen again
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processValley()
{
	float cycleRate;	// the base current skid rate value
	bool barSync = false;	// true if we need to sync up with the next bar start


	if (MIDIin)
	{
		if (midiMode == kMidiMode_trigger)
			// there's one sample of valley when trigger mode begins, so silence that one
			sampleAmp = 0.0f;
		else if (midiMode == kMidiMode_apply)
			// there's one sample of valley when apply mode begins, so keep it at full gain
			sampleAmp = 1.0f;
	}
	// otherwise sampleAmp in the valley is whatever the floor gain is, the lowest gain value
	else if (useRandomFloor)
		sampleAmp = randomFloor;
	else
		sampleAmp = floor;

	valleySamples--;

	if (valleySamples <= 0)
	{
		rms = 0.0;	// reset rms now because valley is over
		//
		// This is where we figure out how many samples long each 
		// envelope section is for the next skid cycle.
		//
		if (tempoSync)	// the user wants to do tempo sync / beat division rate
		{
			// randomize the tempo rate if the random min scalar is lower than the upper bound
			if (useRandomRate)
			{
				long randomizedTempoRateIndex = (long) DFX_InterpolateRandom((float)rateRandMinIndex, (float)rateIndex+0.99f);
				cycleRate = tempoRateTable->getScalar(randomizedTempoRateIndex);
				// we can't do the bar sync if the skids durations are random
				needResync = false;
			}
			else
				cycleRate = rateSync;
			// convert the tempo rate into rate in terms of Hz
			cycleRate *= currentTempoBPS;
			// set this true so that we make sure to do the measure syncronisation later on
			if ( needResync && (midiMode == kMidiMode_none) )
				barSync = true;
		}
		else
		{
			if (useRandomRate)
				cycleRate = (float) expandparametervalue(kRate_abs, DFX_InterpolateRandom(rateRandMinHz_gen, rateHz_gen));
			else
				cycleRate = rateHz;
		}
		needResync = false;	// reset this so that we don't have any trouble
		cycleSamples = (long) (getsamplerate_f() / cycleRate);
		//
		if (useRandomPulsewidth)
			pulseSamples = (long) ( (float)cycleSamples * DFX_InterpolateRandom(pulsewidthRandMin, pulsewidth) );
		else
			pulseSamples = (long) ( (float)cycleSamples * pulsewidth );
		valleySamples = cycleSamples - pulseSamples;
		slopeSamples = (long) (getsamplerate() * slopeSeconds);
		slopeDur = slopeSamples;
		slopeStep = 1.0f / (float)slopeDur;	// calculate the fade increment scalar
		plateauSamples = pulseSamples - (slopeSamples * 2);
		if (plateauSamples < 1)	// this shrinks the slope to 1/3 of the pulse if the user sets slope too high
		{
			slopeSamples = (long) (((float)pulseSamples) / 3.0f);
			slopeDur = slopeSamples;
			slopeStep = 1.0f / (float)slopeDur;	// calculate the fade increment scalar
			plateauSamples = pulseSamples - (slopeSamples * 2);
		}

		// go to slopeIn next if slope is not 0.0, otherwise go to plateau
		if (slopeDur > 0)
			state = kSkidState_SlopeIn;
		else
			state = kSkidState_Plateau;

		if (barSync)	// we need to adjust this cycle so that a skid syncs with the next bar
		{
			// calculate how long this skid cycle needs to be
			long countdown = timeinfo.samplesToNextBar % cycleSamples;
			// skip straight to the valley and adjust its length
			if ( countdown <= (valleySamples+(slopeSamples*2)) )
			{
				valleySamples = countdown;
				state = kSkidState_Valley;
			}
			// otherwise adjust the plateau if the shortened skid is still long enough for one
			else
				plateauSamples -= cycleSamples - countdown;
		}

		// if MIDI apply mode is just beginning, make things smooth with no panning
		if ( (MIDIin) && (midiMode == kMidiMode_apply) )
			panGainL = panGainR = 1.0f;
		else
		{
			// this calculates a random float value from -1.0 to 1.0
			float panRander = (DFX_Rand_f() * 2.0f) - 1.0f;
			// ((panRander*panWidth)+1.0) ranges from 0.0 to 2.0
			panGainL = (panRander*panWidth) + 1.0f;
			panGainR = 2.0f - ((panRander*panWidth) + 1.0f);
		}

	}	// end of the "valley is over" if-statement
}

//-----------------------------------------------------------------------------------------
float Skidder::processOutput(float in1, float in2, float panGain)
{
	// output noise
	if ( (state == kSkidState_Valley) && (noise != 0.0f) )
	{
		// out gets random noise with samples from -1.0 to 1.0 times the random pan times rupture times the RMS scalar
		return ((DFX_Rand_f()*2.0f)-1.0f) * panGain * noise * (float)rms;
	}
	// do regular skidding output
	else
	{
		// only output a bit of the first input
		if (panGain <= 1.0f)
			return in1 * panGain * sampleAmp;
		// output all of the first input and a bit of the second input
		else
			return ( in1 + (in2*(panGain-1.0f)) ) * sampleAmp;
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processaudio(const float ** inStreams, float ** outStreams, unsigned long inNumFrames, bool replacing)
{
	unsigned long numInputs = getnuminputs(), numOutputs = getnumoutputs();
	unsigned long samplecount, ch;
	const float channelScalar = 1.0f / (float)numOutputs;

	// handle the special case of mismatched input/output channel counts that we allow 
	// by repeating the mono-input to multiple (faked) input channels
	const float * doubledInputStreams[2];
	if ( (numInputs == 1) && (numOutputs == 2) )
	{
		for (ch=0; ch < numOutputs; ch++)
			doubledInputStreams[ch] = inStreams[0];
		inStreams = doubledInputStreams;
	}


// ---------- begin MIDI stuff --------------
	processMidiNotes();
	bool noteIsOn = false;

	for (int notecount=0; notecount < NUM_NOTES; notecount++)
	{
		if (noteTable[notecount])
		{
			noteIsOn = true;
			break;	// we only need to find one active note
		}
	}

	switch (midiMode)
	{
		case kMidiMode_trigger:
			// check waitSamples also because, if it's zero, we can just move ahead normally
			if ( noteIsOn && (waitSamples != 0) )
			{
				// need to make sure that the skipped part is silent if we're processing in-place
				if (replacing)
				{
					for (ch=0; ch < numOutputs; ch++)
					{
						for (samplecount = 0; samplecount < (unsigned)waitSamples; samplecount++)
							outStreams[ch][samplecount] = 0.0f;

						// jump ahead accordingly in the i/o streams
						outStreams[ch] += waitSamples;
						inStreams[ch] += waitSamples;
					}
				}

				// cut back the number of samples outputted
				inNumFrames -= (unsigned)waitSamples;

				// reset
				waitSamples = 0;
			}

			else if (!noteIsOn)
			{
				// if Skidder currently is in the plateau and has a slow cycle, this could happen
				if ((unsigned)waitSamples > inNumFrames)
					waitSamples -= (signed)inNumFrames;
				else
				{
					if (replacing)
					{
						for (ch=0; ch < numOutputs; ch++)
						{
							for (samplecount = (unsigned)waitSamples; samplecount < inNumFrames; samplecount++)
								outStreams[ch][samplecount] = 0.0f;
						}
					}
					inNumFrames = (unsigned)waitSamples;
					waitSamples = 0;
				}
			}

			// adjust the floor according to note velocity if velocity mode is on
			if (useVelocity)
			{
				floor = (float) expandparametervalue(kFloor, (float)(127-mostRecentVelocity)/127.0f);
				gainRange = 1.0f - floor;	// the range of the skidding on/off gain
				useRandomFloor = false;
			}

			break;

		case kMidiMode_apply:
			// check waitSamples also because, if it's zero, we can just move ahead normally
			if ( noteIsOn && (waitSamples != 0) )
			{
				// need to make sure that the skipped part is unprocessed audio
				for (ch=0; ch < numOutputs; ch++)
				{
					if (replacing)
						memcpy(outStreams[ch], inStreams[ch], waitSamples * sizeof(outStreams[0][0]));
					else
					{
						for (samplecount = 0; samplecount < (unsigned)waitSamples; samplecount++)
							outStreams[ch][samplecount] += inStreams[ch][samplecount];
					}

					// jump ahead accordingly in the i/o streams
					inStreams[ch] += waitSamples;
					outStreams[ch] += waitSamples;
				}

				// cut back the number of samples outputted
				inNumFrames -= (unsigned)waitSamples;

				// reset
				waitSamples = 0;
			}

			else if (!noteIsOn)
			{
				// if Skidder currently is in the plateau and has a slow cycle, this could happen
				if (waitSamples != 0)
				{
					if ((unsigned)waitSamples > inNumFrames)
						waitSamples -= (signed)inNumFrames;
					else
					{
						for (ch=0; ch < numOutputs; ch++)
						{
							if (replacing)
								memcpy( &(outStreams[ch][waitSamples]), &(inStreams[ch][waitSamples]), (inNumFrames - (unsigned)waitSamples) * sizeof(outStreams[0][0]) );
							else
							{
								for (samplecount = (unsigned)waitSamples; samplecount < inNumFrames; samplecount++)
									outStreams[ch][samplecount] += inStreams[ch][samplecount];
							}
						}
						inNumFrames = (unsigned)waitSamples;
						waitSamples = 0;
					}
				}
				else
				{
					for (ch=0; ch < numOutputs; ch++)
					{
						if (replacing)
							memcpy(outStreams[ch], inStreams[ch], inNumFrames * sizeof(outStreams[0][0]));
						else
						{
							for (samplecount = 0; samplecount < inNumFrames; samplecount++)
								outStreams[ch][samplecount] += inStreams[ch][samplecount];
						}
					}
					// that's all we need to do if there are no notes, 
					// just copy the input to the output
					return;
				}
			}

			// adjust the floor according to note velocity if velocity mode is on
			if (useVelocity)
			{
				floor = (float) expandparametervalue(kFloor, (float)(127-mostRecentVelocity)/127.0f);
				gainRange = 1.0f - floor;	// the range of the skidding on/off gain
				useRandomFloor = false;
			}

			break;

		default:
			break;
	}
// ---------- end MIDI stuff --------------


	// figure out the current tempo if we're doing tempo sync
	if (tempoSync)
	{
		// calculate the tempo at the current processing buffer
		if ( useHostTempo && hostCanDoTempo && timeinfo.tempoIsValid )	// get the tempo from the host
		{
			currentTempoBPS = timeinfo.tempo_bps;
			// check if audio playback has just restarted and reset buffer stuff if it has (for measure sync)
			if (timeinfo.playbackChanged)
			{
				needResync = true;
				state = kSkidState_Valley;
				valleySamples = 0;
			}
		}
		else	// get the tempo from the user parameter
		{
			currentTempoBPS = userTempo / 60.0;
			needResync = false;	// we don't want it true if we're not syncing to host tempo
		}
		oldTempoBPS = currentTempoBPS;
	}
	else
		needResync = false;	// we don't want it true if we're not syncing to host tempo


	// stereo processing
	if (numOutputs == 2)
	{
		// this is the per-sample audio processing loop
		for (samplecount=0; samplecount < inNumFrames; samplecount++)
		{
			const float inputValueL = inStreams[0][samplecount], inputValueR = inStreams[1][samplecount];

			// get the average sqare root of the current input samples
			if ( (state == kSkidState_SlopeIn) || (state == kSkidState_Plateau) )
			{
#ifdef USE_BACKWARDS_RMS
				rms += sqrt( (fabsf(inputValueL)+fabsf(inputValueR)) * channelScalar );
#else
				rms += ((inputValueL*inputValueL) + (inputValueR*inputValueR)) * channelScalar;
#endif
				rmscount++;	// this counter is later used for getting the mean
			}

			switch (state)
			{
				case kSkidState_SlopeIn:
					processSlopeIn();
					break;
				case kSkidState_Plateau:
					processPlateau();
					break;
				case kSkidState_SlopeOut:
					processSlopeOut();
					break;
				case kSkidState_Valley:
				default:
					processValley();
					break;
			}
	
		#ifdef TARGET_API_VST
			if (replacing)
			{
		#endif
				outStreams[0][samplecount] = processOutput(inputValueL, inputValueR, panGainL);
				outStreams[1][samplecount] = processOutput(inputValueR, inputValueL, panGainR);
		#ifdef TARGET_API_VST
			}
			else
			{
				outStreams[0][samplecount] += processOutput(inputValueL, inputValueR, panGainL);
				outStreams[1][samplecount] += processOutput(inputValueR, inputValueL, panGainR);
			}
		#endif
		}
	}

	// independent-channel processing
	else
	{
		// this is the per-sample audio processing loop
		for (samplecount=0; samplecount < inNumFrames; samplecount++)
		{
			// get the average sqare root of the current input samples
			if ( (state == kSkidState_SlopeIn) || (state == kSkidState_Plateau) )
			{
#ifdef USE_BACKWARDS_RMS
				float tempSum = 0.0f;
				for (ch=0; ch < numOutputs; ch++)
					tempSum += fabsf(inStreams[ch][samplecount]);
				rms += sqrt(tempSum * channelScalar);
#else
				for (ch=0; ch < numOutputs; ch++)
					rms += inStreams[ch][samplecount] * inStreams[ch][samplecount] * channelScalar;
#endif
				rmscount++;	// this counter is later used for getting the mean
			}

			switch (state)
			{
				case kSkidState_SlopeIn:
					processSlopeIn();
					break;
				case kSkidState_Plateau:
					processPlateau();
					break;
				case kSkidState_SlopeOut:
					processSlopeOut();
					break;
				case kSkidState_Valley:
				default:
					processValley();
					break;
			}
	
			for (ch=0; ch < numOutputs; ch++)
			{
		#ifdef TARGET_API_VST
				if (replacing)
		#endif
					outStreams[ch][samplecount] = processOutput(inStreams[ch][samplecount], inStreams[ch][samplecount], 1.0f);
		#ifdef TARGET_API_VST
				else
					outStreams[ch][samplecount] += processOutput(inStreams[ch][samplecount], inStreams[ch][samplecount], 1.0f);
		#endif
			}
		}
	}
}
