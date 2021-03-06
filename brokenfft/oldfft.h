

/* broken fft: $Id: oldfft.h,v 1.1 2002-03-25 03:14:12 tom7 Exp $ */

#ifndef __AGAIN_H
#define __AGAIN_H

#include <audioeffectx.h>

#define MAXOVERLAP 1024
#define MAXSAMPLES 8192
#define MAXECHO 8192

struct amplentry {
  float a;
  int i;
};

class Fft : public AudioEffectX
{
public:
  Fft(audioMasterCallback audioMaster);
  ~Fft();

  virtual void processX(float **inputs, float **outputs, 
			long sampleFrames, int overwrite);
  virtual void process(float **inputs, float **outputs, long sampleFrames);
  virtual void processReplacing(float **inputs, float **outputs, 
				long sampleFrames);
  virtual void setProgramName(char *name);
  virtual void getProgramName(char *name);
  virtual void setParameter(long index, float value);
  virtual float getParameter(long index);
  virtual void getParameterLabel(long index, char *label);
  virtual void getParameterDisplay(long index, char *text);
  virtual void getParameterName(long index, char *text);

  virtual void suspend();

  void fftops(long samples);

protected:
  float bitres;
  float destruct;
  char programName[32];

  float * left_buffer, 
        * right_buffer;

  int lastblocksize;

  /* BAD FFT */
  int OVERLAP;
  float * overbuff;

  float perturb, quant, rotate, spike;

  float binquant;

  float sampler, samplei;

  float compress;
  float makeupgain;

  float samplehold; /* OVERLAP */
  float bit1, bit2;
  float spikehold;
  int samplesleft;

  float echomix, echotime;

  int amplhold, amplsamples, stopat;
  amplentry * ampl;
  float * echor;
  float * echoc;

  float echofb;

  /* fft work area */
  float * tmp, * oot, * fftr, * ffti;
  int buffersamples; /* size of work area */


  float echomodw, echomodf;
  float echolow, echohi;
  int echoctr;
};

#endif
