/*-------------- by Sophia Poirier  ][  February 2002 -------------*/

#include "scrubby.h"


//-----------------------------------------------------------------------------------------
inline double calculateTargetSpeed(double a, double n, double k)
{
	double b;	// the target speed

	a = fabs(a);
	n = fabs(n);
	k = fabs(k);


	double lambertInput = (n * a) / k;
	b = k * DFX_LambertW(lambertInput) / n;
	// cuz I don't totally trust my Lambert W function...
#if TARGET_OS_MAC
	if ( isnan(b) || (! isfinite(b)) )
#else
	if ( _isnan(b) || (! _finite(b)) )
#endif
		b = 1.0;
//	else
//		b *= 2.0;
	if (b < 1.0)
		b = pow(b, 0.63);


/*
	float inc;
	float testk = k * 2.0f;
	b = a;
	// b is less than a, guess downwards
	if (k < (n*a))
		inc = 0.5f;
	// b is larger than a, guess upwards
	else
		inc = 2.0f;
	while ( fabsf((testk/k) - 1.0f) > 0.01f )
//	while (testk != k)
	for (int i=0; i < 90; i++)
	{
		b *= inc;
		testk = fabsf( ((b-a) * n) / logf(b/a) );
		if (testk < k)
		{
			b /= inc;
			inc = sqrtf(inc);
		}
		else if (testk == k)
			break;
	}
*/
//	b = k / n;

	return b;
}


//-----------------------------------------------------------------------------------------
void Scrubby::checkTempoSyncStuff()
{
	unsigned long ch, numChannels = getnumoutputs();

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
				for (ch=0; ch < numChannels; ch++)
					needResync[ch] = true;
			}
		}
		else	// get the tempo from the user parameter
		{
			currentTempoBPS = userTempo / 60.0;
			for (ch=0; ch < numChannels; ch++)
				needResync[ch] = false;	// we don't want it true if we're not syncing to host tempo
		}
	}
	else
	{
		for (ch=0; ch < numChannels; ch++)
			needResync[ch] = false;	// we don't want it true if we're not syncing to host tempo
	}

	// reset cycle state stuff if playback has changed (for measure sync)
	for (ch=0; ch < numChannels; ch++)
	{
		if (needResync[ch])
			seekcount[ch] = 0;
	}
}

//-----------------------------------------------------------------------------------------
void Scrubby::generateNewTarget(unsigned long channel)
{
	float currentSeekRate, currentSeekDur;
//	double readStep, portamentoStep;
//	long seekcount, movecount;


// CALCULATE THE SEEK CYCLE LENGTH
	// match the seek rate to tempo
	if (tempoSync)
	{
		// randomize the tempo rate if the random min scalar is lower than the upper bound
		if (useSeekRateRandMin)
		{
			long randomizedSeekRateIndex = (long) DFX_InterpolateRandom((float)seekRateRandMinIndex, (float)seekRateIndex+0.99f);
			currentSeekRate = tempoRateTable->getScalar(randomizedSeekRateIndex);
			// don't do musical bar sync if we're using randomized tempo rate
//			for (unsigned long ch=0; ch < numChannels; ch++)
			needResync[channel] = false;
		}
		else
			currentSeekRate = seekRateSync;
		// convert the tempo rate into rate in terms of Hz
		currentSeekRate *= currentTempoBPS;
	}
	//
	// use the seek rate in terms of Hz
	else
	{
		if (useSeekRateRandMin)
			currentSeekRate = expandparametervalue(kSeekRate_abs, DFX_InterpolateRandom(seekRateRandMinHz_gen, seekRateHz_gen));
		else
			currentSeekRate = seekRateHz;
	}
	// calculate the length of this seek cycle in seconds
	float cycleDur = 1.0f / currentSeekRate;
	// then calculate the length of this seek cycle in samples
	seekcount[channel] = (long) (cycleDur * getsamplerate_f());
	//
	// do bar sync if we're in tempo sync and things resyncing is in order
	if (needResync[channel])
	{
		long samplesUntilBar = timeinfo.samplesToNextBar;
		// because a 0 for seekcount will be soon turned into 1, which is not so good for DJ mode
		if (samplesUntilBar > 0)
		{
			if (speedMode == kSpeedMode_dj)
				// doubling the length of the first seek after syncing to the measure 
				// seems to prevent Cubase from freaking out in tempo sync'd DJ mode
				seekcount[channel] = (samplesUntilBar+seekcount[channel]) % (seekcount[channel]*2);
			else
				seekcount[channel] = samplesUntilBar % seekcount[channel];
			cycleDur = (float)(seekcount[channel]) / getsamplerate_f();
		}
	}
	// avoid bad values
	if (seekcount[channel] < 1)
		seekcount[channel] = 1;

// CALCULATE THE MOVEMENT CYCLE LENGTH
	if (useSeekDurRandMin)
		currentSeekDur = DFX_InterpolateRandom(seekDurRandMin, seekDur);
	else
		currentSeekDur = seekDur;
	// calculate the length of the seeking movement in samples
	movecount[channel] = (long) (cycleDur * currentSeekDur * getsamplerate_f());
	// avoid bad values
	if (movecount[channel] < 1)
		movecount[channel] = 1;

// FIND THE NEW TARGET POSITION IN THE BUFFER
	// randomly locate a new target position within the buffer seek range
	float bufferSizeFloat = seekRangeSeconds * getsamplerate_f();
	// search back from the current writer input point
	long newTargetPos = writePos - (long)(bufferSizeFloat * DFX_Rand_f());
	//
	// calculate the distance between
	long readPosInt = (long)(readPos[channel]);
	if (readPosInt >= writePos)
		readPosInt -= MAX_BUFFER;
//	if ( abs(readPosInt-writePos) > (MAX_BUFFER/2) )
//	{
//		if (readPosInt >= writePos)
//			readPosInt -= MAX_BUFFER;
//		else
//			readPosInt += MAX_BUFFER;
//	}
	long targetDistance = newTargetPos - readPosInt;
	if (targetDistance == 0)
		targetDistance = 1;	
	//
	// calculate the step size of playback movement through the buffer
	double newReadStep = (double)targetDistance / (double)movecount[channel];
	// constrain the speed to a semitone step, if that's what we want to do
	if ( pitchConstraint && (speedMode == kSpeedMode_robot) )
		newReadStep = processPitchConstraint(newReadStep);
	// still constrain to octave ranges, even when pitch constraint is disabled
	else
	{
		double fdirection = (newReadStep < 0.0) ? -1.0 : 1.0;	// direction scalar
		double newReadStep_abs = fabs(newReadStep);
		// constrain to octaves range, if we're doing that
		if ( (octaveMin > kOctave_MinValue) && (newReadStep_abs < 1.0) )
		{
			double minstep = pow(2.0, (double)octaveMin);
			if (newReadStep_abs < minstep)
				newReadStep = minstep * fdirection;
		}
		else if ( (octaveMax < kOctave_MaxValue) && (newReadStep_abs > 1.0) )
		{
			double maxstep = pow(2.0, (double)octaveMax);
			if (newReadStep_abs > maxstep)
				newReadStep = maxstep * fdirection;
		}
	}
	//
	// calculate the step size of portamento playback incrementation
//	if ( (speedMode == kSpeedMode_dj) && !(needResync[channel]) )
	if (speedMode == kSpeedMode_dj)
	{
		bool moveBackwards = (newReadStep < 0.0);	// will we move backwards in the delay buffer?
		double oldReadStep = readStep[channel];
		oldReadStep = fabs(oldReadStep);
		//
#if USE_LINEAR_ACCELERATION
		double absReadStep = fabs(newReadStep);
		bool slowdown = (absReadStep < oldReadStep);	// are we slowing down or speeding up?
		double stepDifference = absReadStep - oldReadStep;
		bool cross0 = ( slowdown && (stepDifference > absReadStep) );	// will we go down to 0 Hz and then back up again?
		portamentoStep = (stepDifference * 2.0) / (double)(movecount[channel]);
		if (moveBackwards)
			portamentoStep[channel] *= -1.0;
//		portamentoStep[channel] = fabs(portamentoStep[channel]);
//		if (slowdown)
//			portamentoStep[channel] *= -1.0;
#else
		// exponential acceleration
		if (oldReadStep < 0.001)	// protect against a situation impossible to deal with
			oldReadStep = 0.001;
		double targetReadStep = calculateTargetSpeed(oldReadStep, (double)(movecount[channel]), (double)targetDistance);
		portamentoStep[channel] = pow( fabs(targetReadStep)/oldReadStep, 1.0/(double)(movecount[channel]) );
		portamentoStep[channel] = fabs(portamentoStep[channel]);
//showme = targetReadStep;
//long ktest = (long) fabsf( (targetReadStep * (float)movecount) / logf(fabsf(targetReadStep/oldReadStep)) );
//printf("oldReadStep = %.6f\nreadStep = %.6f\ntargetReadStep = %.6f\nportamentoStep = %.6f\nmovecount = %ld\ntargetDistance = %ld\nktest = %ld\n\n", oldReadStep,newReadStep,targetReadStep,portamentoStep[channel], movecount[channel], targetDistance,ktest);
//printf("a = %.3f,\tb = %.3f,\tn = %ld,\tk = %ld\tportamentoStep = %.6f\n", oldReadStep, targetReadStep, movecount[channel], targetDistance, portamentoStep[channel]);
//printf("\noldReadStep = %.6f\treadStep = %.6f\ttargetReadStep = %.6f\tportamentoStep = %.6f\n", oldReadStep,newReadStep,targetReadStep,portamentoStep[channel]);

#endif
		newReadStep = oldReadStep;
		if (moveBackwards)
			newReadStep = -newReadStep;
#if USE_LINEAR_ACCELERATION
		if (cross0)
			newReadStep = -newReadStep;
#endif
	}
	else
#if USE_LINEAR_ACCELERATION
		portamentoStep[channel] = 0.0;	// for no linear acceleration
#else
		portamentoStep[channel] = 1.0;	// for no exponential acceleration
#endif

	// assign the new readStep value into the correct channel's variable
	readStep[channel] = newReadStep;
	needResync[channel] = false;	// untrue this so that we don't do the measure sync calculations again unnecessarily
}

// this is a necessary value for calculating the pitches related to the playback speeds
static const double LN2TO1_12TH = log( pow(2.0, 1.0/12.0) );
//const double LN2TO1_12TH = 0.05776226504666215;

//-----------------------------------------------------------------------------------------
double Scrubby::processPitchConstraint(double readStep)
{
	bool backwards = (readStep < 0.0);	// traveling backwards through the buffer?
	double fdirection = backwards ? -1.0 : 1.0;	// direction scalar

	// check to see if none of the semitones are enabled, 
	// in which case we act like they are all enabled.
	// no we don't, we become silent, I changed my mind
	bool noNotesActive = true;
	for (long i=0; i < kNumPitchSteps; i++)
	{
		if (pitchSteps[i])
			noNotesActive = false;
	}

	// determine the lower bound of the number of semitone shifts that this playback speed results in
	// the equation below comes from 2^(semitone/12) = readStep, solving for semitone
	double fsemitone = log(fabs(readStep)) / LN2TO1_12TH;
	fsemitone = floor(fsemitone);
	long semitone = (long) (fsemitone + 0.1);	// add a little bit to prevent any float truncation errors

	// . . . search for a possible active note to constrain to . . .
	long octave = semitone / 12;	// the octave transposition
	long remainder = semitone % 12;	// the semitone transposition
	// remainder will be used as the index to the pitchSteps array, 
	// so it must be positive, and we compensate for adding 12 to it 
	// by subtracting 1 octave, so it all evens out
	if (remainder < 0)
	{
		remainder += 12;
		octave -= 1;
	}
	// just in case it fails both searches, which shouldn't happen
	semitone = remainder;
	// start searching for an active pitchStep with the current pitchStep (remainder) 
	// and then all those below it
	long i = remainder;
	// if no notes are active, then don't play back anything (this basically means silence)
	if (noNotesActive)
		return 0.0f;
	// if we are choosing from a selection of semitone steps, then do the selection process
	else
	{
		do
		{
			// we found the pitchstep that we will use, store it and exit this loop
			if (pitchSteps[i])
			{
				semitone = i;
				break;
			}
			// otherwise keep searching down
			i--;
			if (i < 0)
			{
				// didn't find an active one yet, so wrap around to the top of the pitchStep array
				i = kNumPitchSteps - 1;
				// and compensate for that by subtracting an octave
				octave--;
			}
		} while (i != remainder);
	}
	// constrain to octaves range, if we're doing that
	if ( (octaveMin > kOctave_MinValue) && (octave < octaveMin) )
		octave = octaveMin;
	else if ( (octaveMax < kOctave_MaxValue) && (octave > octaveMax) )
		octave = octaveMax;
	// add the octave transposition back in to get the correct semitone transposition
	semitone += (octave * 12);

	// return the transposition converted to a playback speed scalar, with direction restored
	return pow(2.0, (double)semitone/12.0) * fdirection;
}


//-----------------------------------------------------------------------------------------
void Scrubby::processaudio(const float ** in, float ** out, unsigned long inNumFrames, bool replacing)
{
	unsigned long ch, numChannels = getnumoutputs();

//-------------------------SAFETY CHECK----------------------
	// there must have not been available memory or something (like WaveLab goofing up), 
	// so try to allocate buffers now
	if (numBuffers < numChannels)
		createbuffers();
	for (ch=0; ch < numChannels; ch++)
	{
		if (buffers[ch] == NULL)
		{
			// exit the loop if creation succeeded
			if ( createbuffers() )
				break;
			// or abort audio processing if the creation failed
			else return;
		}
	}

	processMidiNotes();
	checkTempoSyncStuff();

	// if we're using pitch constraint and the previous block had no notes active, 
	// then check to see if new notes have begun and, if so, begin new seeks 
	// so that we start producing sound again immediately
	if ( (pitchConstraint && (speedMode == kSpeedMode_robot)) && !notesWereAlreadyActive )
	{
		for (int i=0; i < kNumPitchSteps; i++)
		{
			if (pitchSteps[i])
			{
				for (ch=0; ch < numChannels; ch++)
					seekcount[ch] = 0;
				break;
			}
		}
	}
	// now remember the current situation for informing the next processing block
	notesWereAlreadyActive = false;
	for (int i=0; i < kNumPitchSteps; i++)
	{
		if (pitchSteps[i])
			notesWereAlreadyActive = true;
	}

	for (unsigned long samplecount=0; samplecount < inNumFrames; samplecount++)
	{
		// update the buffers with the latest samples
		if (!freeze)
		{
			for (ch=0; ch < numChannels; ch++)
				buffers[ch][writePos] = in[ch][samplecount];
// melody test
//			for (ch=0; ch < numChannels; ch++)
//				buffers[ch][writePos] = 0.69f*sinf(24.0f*kDFX_PI_f*((float)samplecount/(float)inNumFrames));
//				buffers[ch][writePos] = 0.69f*sinf(2.0f*kDFX_PI_f*((float)sinecount/169.0f));
//			if (++sinecount > 168)   sinecount = 0;	// produce a sine wave of C4 when using 44.1 kHz
		}

		// write the output to the output streams, interpolated for smoothness
	#ifdef TARGET_API_VST
		if (replacing)
		{
	#endif
			for (ch=0; ch < numChannels; ch++)
				out[ch][samplecount] = DFX_InterpolateHermite(buffers[ch], readPos[ch], MAX_BUFFER);
	#ifdef TARGET_API_VST
		}
		else
		{
			for (ch=0; ch < numChannels; ch++)
				out[ch][samplecount] += DFX_InterpolateHermite(buffers[ch], readPos[ch], MAX_BUFFER);
		}
	#endif

		// increment/decrement the position trackers and counters
		if (!freeze)
			writePos = (writePos+1) % MAX_BUFFER;
		for (ch=0; ch < numChannels; ch++)
		{
			(seekcount[ch])--;
			(movecount[ch])--;
		}
		//
		// it's time to find a new target to seek
		if (seekcount[0] < 0)
		{
			generateNewTarget(0);

			// copy the left channel's new values if we're in unified channels mode
			if (!splitChannels)
			{
				for (ch=1; ch < numChannels; ch++)
				{
					readPos[ch] = readPos[0];
					readStep[ch] = readStep[0];
					portamentoStep[ch] = portamentoStep[0];
					seekcount[ch] = seekcount[0];
					movecount[ch] = movecount[0];
					needResync[ch] = needResync[0];
				}
			}
		}
		// find a new target to seek for the right channel if we're in split channels mode
		if (splitChannels)
		{
			for (ch=1; ch < numChannels; ch++)
			{
				if (seekcount[ch] < 0)
					generateNewTarget(ch);
			}
		}
		//
		// only increment the read position trackers if we're still moving towards the target
		for (ch=0; ch < numChannels; ch++)
		{
			if (movecount[ch] >= 0)
			{
				if (speedMode == kSpeedMode_dj)
#if USE_LINEAR_ACCELERATION
					readStep[ch] += portamentoStep[ch];
#else
					readStep[ch] *= portamentoStep[ch];
#endif
				readPos[ch] += readStep[ch];
				long readPosInt = (long)readPos[ch];
				// wraparound the read position tracker if necessary
				if (readPosInt >= MAX_BUFFER)
					readPos[ch] = fmod(readPos[ch], MAX_BUFFER_FLOAT);
				else if (readPosInt < 0)
				{
					while (readPos[ch] < 0.0)
						readPos[ch] += MAX_BUFFER_FLOAT;
				}
	//printf("readStep[ch] = %.6f\treadPos[ch] = %.9f\tmovecount[ch] = %ld\n", readStep[ch], readPos[ch], movecount[ch]);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------
void Scrubby::processMidiNotes()
{
	bool oldnotes[kNumPitchSteps];
	for (int i=0; i < kNumPitchSteps; i++)
		oldnotes[i] = getparameter_b(i+kPitchStep0);

	for (long i=0; i < midistuff->numBlockEvents; i++)
	{
		// wrap the note value around to our 1-octave range
		int currentNote = (midistuff->blockEvents[i].byte1) % kNumPitchSteps;

		switch (midistuff->blockEvents[i].status)
		{
			case kMidiNoteOn:
				// correct any bogus values that would mess up the system
				if (activeNotesTable[currentNote] < 0)
					activeNotesTable[currentNote] = 0;
				// if there are currently no keys playing this note, 
				// then turn its associated pitch constraint parameter on
				if (activeNotesTable[currentNote] == 0)
				{
					setparameter_b(currentNote+kPitchStep0, true);
				}
				// increment the active notes table for this note
				activeNotesTable[currentNote] += 1;
				break;

			case kMidiNoteOff:
				// if this is the last key that was playing this note, 
				// then turn its associated pitch constraint parameter off
				if (activeNotesTable[currentNote] == 1)
				{
					setparameter_b(currentNote+kPitchStep0, false);
				}
				// decrement the active notes table for this note, but don't go below 0
				if (activeNotesTable[currentNote] > 0)
					activeNotesTable[currentNote] -= 1;
				else
					activeNotesTable[currentNote] = 0;
				break;

			case kMidiCC:
				if (midistuff->blockEvents[i].byte1 == kMidiCC_AllNotesOff)
				{
					for (int notecount=0; notecount < kNumPitchSteps; notecount++)
					{
						// if this note is currently active, then turn its 
						// associated pitch constraint parameter off
						if (activeNotesTable[notecount] > 0)
						{
							setparameter_b(notecount+kPitchStep0, false);
						}
						// reset this note in the table
						activeNotesTable[notecount] = 0;
					}
				}
				break;

			default:
				break;
		}
	}

	// go through and inform any listeners of any parameter changes that might have just happened
	for (int i=0; i < kNumPitchSteps; i++)
	{
		if ( oldnotes[i] != getparameter_b(i+kPitchStep0) )
			postupdate_parameter(i+kPitchStep0);
	}
}
