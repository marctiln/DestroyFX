
/* Geometer,
   Featuring the Super Destroy FX Windowing System! */

#include "geometer.h"

int intcompare(const void * a, const void * b);
int intcompare(const void * a, const void * b) {
  return (*(int*)a - *(int*)b);
}

/* this macro does boring entry point stuff for us */
#if 1
DFX_EFFECT_ENTRY(Geometer)
#else
extern "C" OSStatus GeometerEntry(ComponentParameters * params, Geometer * obj);
extern "C" OSStatus GeometerEntry(ComponentParameters * params, Geometer * obj)
{
	if (params->what == kAudioUnitResetSelect)
		printf("Geometer kAudioUnitResetSelect\n");
	return ComponentEntryPoint<Geometer>::Dispatch(params, obj);
}
#endif
#if TARGET_PLUGIN_USES_DSPCORE
  DFX_CORE_ENTRY(GeometerDSP)
#endif

PLUGIN::PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, NUM_PARAMS, NUM_PRESETS) {

#if !TARGET_PLUGIN_USES_DSPCORE
  cs = NULL;
#endif

  initparameter_list(P_BUFSIZE, "wsize", 9, 9, BUFFERSIZESSIZE, kDfxParamUnit_samples);
  initparameter_list(P_SHAPE, "wshape", WINDOW_TRIANGLE, WINDOW_TRIANGLE, MAX_WINDOWSHAPES);

  initparameter_list(P_POINTSTYLE, "points where", POINT_EXTNCROSS, POINT_EXTNCROSS, MAX_POINTSTYLES);

  initparameter_f(P_POINTPARAMS + POINT_EXTNCROSS, "point:ext'n'cross", 0.0, 0.0, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "magn");
  initparameter_f(P_POINTPARAMS + POINT_FREQ, "point:freq", 0.08, 0.08, 0.0, 1.0, kDfxParamUnit_scalar);
  initparameter_f(P_POINTPARAMS + POINT_RANDOM, "point:rand", 0.20, 0.20, 0.0, 1.0, kDfxParamUnit_scalar);
  initparameter_f(P_POINTPARAMS + POINT_SPAN, "point:span", 0.20, 0.20, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "width");
  initparameter_f(P_POINTPARAMS + POINT_DYDX, "point:dydx", 0.50, 0.50, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "gap");
  initparameter_f(P_POINTPARAMS + POINT_LEVEL, "point:level", 0.50, 0.50, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "level");

  for(int pp = NUM_POINTSTYLES; pp < MAX_POINTSTYLES; pp++) {
    initparameter_f(P_POINTPARAMS + pp, "pointparam:unused", 0.04, 0.04, 0.0, 1.0, kDfxParamUnit_generic);
    setparameterattributes(P_POINTPARAMS + pp, kDfxParamAttribute_unused);	/* don't display as an available parameter */
  }

  initparameter_list(P_INTERPSTYLE, "interpolate how", INTERP_POLYGON, INTERP_POLYGON, MAX_INTERPSTYLES);

  initparameter_f(P_INTERPARAMS + INTERP_POLYGON, "interp:polygon", 0.0, 0.0, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "angle");
  initparameter_f(P_INTERPARAMS + INTERP_WRONGYGON, "interp:wrongy", 0.0, 0.0, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "angle");
  initparameter_f(P_INTERPARAMS + INTERP_SMOOTHIE, "interp:smoothie", 0.5, 0.5, 0.0, 1.0, kDfxParamUnit_exponent);
  initparameter_f(P_INTERPARAMS + INTERP_REVERSI, "interp:reversie", 0.0, 0.0, 0.0, 1.0, kDfxParamUnit_generic);
  initparameter_f(P_INTERPARAMS + INTERP_PULSE, "interp:pulse", 0.05, 0.05, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "pulse");
  initparameter_f(P_INTERPARAMS + INTERP_FRIENDS, "interp:friends", 1.0, 1.0, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "width");
  initparameter_f(P_INTERPARAMS + INTERP_SING, "interp:sing", 0.8, 0.8, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "mod");
  initparameter_f(P_INTERPARAMS + INTERP_SHUFFLE, "interp:shuffle", 0.3, 0.3, 0.0, 1.0, kDfxParamUnit_generic);

  for(int ip = NUM_INTERPSTYLES; ip < MAX_INTERPSTYLES; ip++) {
    initparameter_f(P_INTERPARAMS + ip, "inter:unused", 0.0, 0.0, 0.0, 1.0, kDfxParamUnit_generic);
    setparameterattributes(P_INTERPARAMS + ip, kDfxParamAttribute_unused);	/* don't display as an available parameter */
  }

  initparameter_list(P_POINTOP1, "pointop1", OP_NONE, OP_NONE, MAX_OPS);
  initparameter_list(P_POINTOP2, "pointop2", OP_NONE, OP_NONE, MAX_OPS);
  initparameter_list(P_POINTOP3, "pointop3", OP_NONE, OP_NONE, MAX_OPS);

#define ALLOP(n, str, def, unit, unitstr) \
  do { \
    initparameter_f(P_OPPAR1S + n, "op1:" str, def, def, 0.0, 1.0, unit, kDfxParamCurve_linear, unitstr); \
    initparameter_f(P_OPPAR2S + n, "op2:" str, def, def, 0.0, 1.0, unit, kDfxParamCurve_linear, unitstr); \
    initparameter_f(P_OPPAR3S + n, "op3:" str, def, def, 0.0, 1.0, unit, kDfxParamCurve_linear, unitstr); \
  } while (0)

  ALLOP(OP_DOUBLE, "double", 0.5, kDfxParamUnit_lineargain, NULL);
  ALLOP(OP_HALF, "half", 0.0, kDfxParamUnit_generic, NULL);
  ALLOP(OP_QUARTER, "quarter", 0.0, kDfxParamUnit_generic, NULL);
  ALLOP(OP_LONGPASS, "longpass", 0.15, kDfxParamUnit_custom, "length");
  ALLOP(OP_SHORTPASS, "shortpass", 0.5, kDfxParamUnit_custom, "length");
  ALLOP(OP_SLOW, "slow", 0.25, kDfxParamUnit_scalar, NULL);	// "factor"
  ALLOP(OP_FAST, "fast", 0.5, kDfxParamUnit_scalar, NULL);	// "factor"
  ALLOP(OP_NONE, "none", 0.0, kDfxParamUnit_generic, NULL);
  
  for(int op = NUM_OPS; op < MAX_OPS; op++) {
    ALLOP(op, "unused", 0.5, kDfxParamUnit_generic, NULL);
    setparameterattributes(P_OPPAR1S + op, kDfxParamAttribute_unused);	/* don't display as an available parameter */
    setparameterattributes(P_OPPAR2S + op, kDfxParamAttribute_unused);	/* don't display as an available parameter */
    setparameterattributes(P_OPPAR3S + op, kDfxParamAttribute_unused);	/* don't display as an available parameter */
  }
#undef ALLOP

  long i;
  /* windowing */
  char * bufstr = (char*) malloc(256);
  for (i=0; i < BUFFERSIZESSIZE; i++)
  {
    if (buffersizes[i] > 1000)
      sprintf(bufstr, "%ld,%03ld", buffersizes[i]/1000, buffersizes[i]%1000);
    else
      sprintf(bufstr, "%ld", buffersizes[i]);
    setparametervaluestring(P_BUFSIZE, i, bufstr);
  }
  free(bufstr);
  setparametervaluestring(P_SHAPE, WINDOW_TRIANGLE, "linear");
  setparametervaluestring(P_SHAPE, WINDOW_ARROW, "arrow");
  setparametervaluestring(P_SHAPE, WINDOW_WEDGE, "wedge");
  setparametervaluestring(P_SHAPE, WINDOW_COS, "best");
  for (i=NUM_WINDOWSHAPES; i < MAX_WINDOWSHAPES; i++)
    setparametervaluestring(P_SHAPE, i, "???");
  /* geometer */
  setparametervaluestring(P_POINTSTYLE, POINT_EXTNCROSS, "ext 'n cross");
  setparametervaluestring(P_POINTSTYLE, POINT_LEVEL, "at level");
  setparametervaluestring(P_POINTSTYLE, POINT_FREQ, "at freq");
  setparametervaluestring(P_POINTSTYLE, POINT_RANDOM, "randomly");
  setparametervaluestring(P_POINTSTYLE, POINT_SPAN, "span");
  setparametervaluestring(P_POINTSTYLE, POINT_DYDX, "dy/dx");
  for (i=NUM_POINTSTYLES; i < MAX_POINTSTYLES; i++)
    setparametervaluestring(P_POINTSTYLE, i, "unsup");
  setparametervaluestring(P_INTERPSTYLE, INTERP_POLYGON, "polygon");
  setparametervaluestring(P_INTERPSTYLE, INTERP_WRONGYGON, "wrongygon");
  setparametervaluestring(P_INTERPSTYLE, INTERP_SMOOTHIE, "smoothie");
  setparametervaluestring(P_INTERPSTYLE, INTERP_REVERSI, "reversi");
  setparametervaluestring(P_INTERPSTYLE, INTERP_PULSE, "pulse");
  setparametervaluestring(P_INTERPSTYLE, INTERP_FRIENDS, "friends");
  setparametervaluestring(P_INTERPSTYLE, INTERP_SING, "sing");
  setparametervaluestring(P_INTERPSTYLE, INTERP_SHUFFLE, "shuffle");
  for (i=NUM_INTERPSTYLES; i < MAX_INTERPSTYLES; i++)
    setparametervaluestring(P_INTERPSTYLE, i, "unsup");
#define ALLOPSTR(n, str) \
  do { \
    setparametervaluestring(P_POINTOP1, n, str); \
    setparametervaluestring(P_POINTOP2, n, str); \
    setparametervaluestring(P_POINTOP3, n, str); \
  } while (0)
  ALLOPSTR(OP_DOUBLE, "x2");
  ALLOPSTR(OP_HALF, "1/2");
  ALLOPSTR(OP_QUARTER, "1/4");
  ALLOPSTR(OP_LONGPASS, "longpass");
  ALLOPSTR(OP_SHORTPASS, "shortpass");
  ALLOPSTR(OP_SLOW, "slow");
  ALLOPSTR(OP_FAST, "fast");
  ALLOPSTR(OP_NONE, "none");
  for (i=NUM_OPS; i < MAX_OPS; i++)
    ALLOPSTR(i, "unsup");
#undef ALLOPSTR

  long delay_samples = buffersizes[getparameter_i(P_BUFSIZE)];
  setlatency_samples(delay_samples);
  settailsize_samples(delay_samples);

  setpresetname(0, "Geometer LoFi");	/* default preset name */
  makepresets();

  /* since we don't use notes for any specialized control of Geometer, 
     allow them to be assigned to control parameters via MIDI learn */
  dfxsettings->setAllowPitchbendEvents(true);
  dfxsettings->setAllowNoteEvents(true);

#if !TARGET_PLUGIN_USES_DSPCORE
  addchannelconfig(1, 1);	/* mono */
  cs = new dfxmutex();
#endif

##if TARGET_PLUGIN_USES_DSPCORE
  DFX_INIT_CORE(GeometerDSP);
#endif
}

PLUGIN::~PLUGIN() {
#if !TARGET_PLUGIN_USES_DSPCORE
  if (cs != NULL)
    delete cs;
  cs = NULL;
#endif
}

void PLUGIN::randomizeparameter(long inParameterIndex)
{
  // we need to constrain the range of values of the parameters that have extra (currently unused) room for future expansion
  int64_t maxValue = 0;
  switch (inParameterIndex)
  {
    case P_SHAPE:
      maxValue = NUM_WINDOWSHAPES;
	  break;
    case P_POINTSTYLE:
      maxValue = NUM_POINTSTYLES;
	  break;
    case P_INTERPSTYLE:
      maxValue = NUM_INTERPSTYLES;
	  break;
    case P_POINTOP1:
    case P_POINTOP2:
    case P_POINTOP3:
      maxValue = NUM_OPS;
	  break;
	default:
	  DfxPlugin::randomizeparameter(inParameterIndex);
	  return;
  }

  int64_t newValue = rand() % maxValue;
  setparameter_i(inParameterIndex, newValue);

  update_parameter(inParameterIndex);	// make the host aware of the parameter change
  postupdate_parameter(inParameterIndex);	// inform any parameter listeners of the changes
}

#if TARGET_PLUGIN_USES_DSPCORE
PLUGINCORE::PLUGINCORE(DfxPlugin * inDfxPlugin)
  : DfxPluginCore(inDfxPlugin), cs(NULL)
{
  cs = new DfxMutex();
#else
long PLUGIN::initialize()
{
#endif
  /* determine the size of the largest window size */
  long maxframe = 0;
  for (int i=0; i< BUFFERSIZESSIZE; i++)
    maxframe = (buffersizes[i] > maxframe) ? buffersizes[i] : maxframe;

  /* add some leeway? */
  in0 = (float*)calloc(maxframe, sizeof (float));
  out0 = (float*)calloc(maxframe * 2, sizeof (float));

  /* prevmix is only a single third long */
  prevmix = (float*)calloc((maxframe / 2), sizeof (float));

  /* geometer buffers */
  pointx = (int*)calloc((maxframe * 2 + 3), sizeof (int));
  storex = (int*)calloc((maxframe * 2 + 3), sizeof (int));

  pointy = (float*)calloc((maxframe * 2 + 3), sizeof (float));
  storey = (float*)calloc((maxframe * 2 + 3), sizeof (float));

  windowbuf = (float*)calloc(maxframe, sizeof(float));
#if !TARGET_PLUGIN_USES_DSPCORE
  return kDfxErr_NoError;
#endif
}

#if TARGET_PLUGIN_USES_DSPCORE
PLUGINCORE::~PLUGINCORE()
{
  if (cs != NULL)
    delete cs;
  cs = NULL;
#else
void PLUGIN::cleanup()
{
#endif
free(windowbuf);
  /* windowing buffers */
  free (in0);
  free (out0);

  free (prevmix);

  /* geometer buffers */
  free(pointx);
  free(pointy);
  free(storex);
  free(storey);
}

void PLUGINCORE::reset() {

  framesize = buffersizes[getparameter_i(P_BUFSIZE)];
  third = framesize / 2;
  bufsize = third * 3;

  /* set up buffers. Prevmix and first frame of output are always 
     filled with zeros. */

  if (prevmix != NULL)
  {
    for (int i = 0; i < third; i ++) {
      prevmix[i] = 0.0f;
    }
  }

  if (out0 != NULL)
  {
    for (int j = 0; j < framesize; j ++) {
      out0[j] = 0.0f;
    }
  }
  
  /* start input at beginning. Output has a frame of silence. */
  insize = 0;
  outstart = 0;
  outsize = framesize;

#if TARGET_PLUGIN_USES_DSPCORE
  dfxplugin->setlatency_samples(framesize);
  /* tail is the same as delay, of course */
  dfxplugin->settailsize_samples(framesize);
#else
  setlatency_samples(framesize);
  /* tail is the same as delay, of course */
  settailsize_samples(framesize);
#endif

  shape = getparameter_i(P_SHAPE);
  long z;
  const float oneDivThird = 1.0f / (float)third;
  if (windowbuf != NULL)
  {
    switch(shape) {
      case WINDOW_TRIANGLE:
        for(z = 0; z < third; z++) {
          windowbuf[z] = ((float)z * oneDivThird);
          windowbuf[z+third] = (1.0f - ((float)z * oneDivThird));
        }
        break;
      case WINDOW_ARROW:
        for(z = 0; z < third; z++) {
          float p = (float)z * oneDivThird;
          p *= p;
          windowbuf[z] = p;
          windowbuf[z+third] = (1.0f - p);
        }
        break;
      case WINDOW_WEDGE:
        for(z = 0; z < third; z++) {
          float p = sqrtf((float)z * oneDivThird);
          windowbuf[z] = p;
          windowbuf[z+third] = (1.0f - p);
        }
        break;
      case WINDOW_COS:
        for(z = 0; z < third; z ++) {
          // XXX what about Eulor's law for cosine?
          float p = 0.5f * (-cosf(kDFX_PI_f * ((float)z * oneDivThird)) + 1.0f);
          windowbuf[z] = p;
          windowbuf[z+third] = (1.0f - p);
        }
        break;
    }
  }
}


void PLUGINCORE::processparameters() {

  shape = getparameter_i(P_SHAPE);
  pointstyle = getparameter_i(P_POINTSTYLE);
  pointparam = getparameter_f(P_POINTPARAMS + pointstyle);
  interpstyle = getparameter_i(P_INTERPSTYLE);
  interparam = getparameter_f(P_INTERPARAMS + interpstyle);
  pointop1 = getparameter_i(P_POINTOP1);
  oppar1 = getparameter_f(P_OPPAR1S + pointop1);
  pointop2 = getparameter_i(P_POINTOP2);
  oppar2 = getparameter_f(P_OPPAR2S + pointop2);
  pointop3 = getparameter_i(P_POINTOP3);
  oppar3 = getparameter_f(P_OPPAR3S + pointop3);

  #ifdef TARGET_API_VST
  if (getparameterchanged(P_BUFSIZE))
    /* this tells the host to call a suspend()-resume() pair, 
      which updates initialDelay value */
    #if TARGET_PLUGIN_USES_DSPCORE
    dfxplugin->setlatencychanged(true);
    #else
    setlatencychanged(true);
    #endif
  #endif
}


/* operations on points. this is a separate function
   because it is called once for each operation slot.
   It's static to enforce thread-safety.
*/
int PLUGINCORE::pointops(long pop, int npts, float op_param, int samples,
                     int * px, float * py, int maxpts,
                     int * tempx, float * tempy) {
  /* pointops. */

  int times = 2;
  switch(pop) {
  case OP_DOUBLE: {
    /* x2 points */
    int i = 0;
    int t;
    for(t = 0; i < (npts - 1) && t < (maxpts-4); i++) {
      /* always include the actual point */
      tempx[t] = px[i];
      tempy[t] = py[i];
      t++;
      /* now, only if there's room... */
      if ((i < npts) && ((px[i+1] - px[i]) > 1)) {
        /* add an extra point, right between the old points.
           (the idea is to double the frequency).
           Pick its y coordinate according to the parameter.
        */

        tempy[t] = (op_param * 2.0f - 1.0f) * py[i];
        tempx[t] = (px[i] + px[i+1]) >> 1;

        t++;
      }
    }
    /* include last if not different from previous */
    if (t > 0 && npts > 0 && tempx[t-1] != px[npts-1]) {
      tempx[t] = px[npts-1];
      tempy[t] = py[npts-1];
      t++;
    }

    for(int c = 0; c < t; c++) {
      px[c] = tempx[c];
      py[c] = tempy[c];
    }
    npts = t;
    break;
  }
  case OP_HALF:
  case OP_QUARTER: {
    times = 1;
    if (pop == OP_QUARTER) times = 2;
    for(int t = 0; t < times; t++) {
      int i;
      /* cut points in half. never touch first or last. */
      int q = 1;
      for(i=1; q < (npts - 1); i++) {
        px[i] = px[q];
        py[i] = py[q];
        q += 2;
      }
      px[i] = px[npts - 1];
      py[i] = py[npts - 1];
      npts = i+1;
    }
    break;
  }
  case OP_LONGPASS: {
    /* longpass. drop any point that's not at least param*samples
       past the previous. */ 
    /* XXX this can cut out the last point? */
    tempx[0] = px[0];
    tempy[0] = py[0];

    int stretch = (op_param * op_param) * samples;
    int np = 1;

    for(int i=1; i < (npts-1); i ++) {
      if (px[i] - tempx[np-1] > stretch) {
        tempx[np] = px[i];
        tempy[np] = py[i];
        np++;
        if (np == maxpts) break;
      }
    }

    for(int c = 1; c < np; c++) {
      px[c] = tempx[c];
      py[c] = tempy[c];
    }
    
    px[np] = px[npts-1];
    py[np] = py[npts-1];
    np++;

    npts = np;

    break;
  }
  case OP_SHORTPASS: {
    /* shortpass. If an interval is longer than the
       specified amount, zero the 2nd endpoint.
    */

    int stretch = (op_param * op_param) * samples;

    for (int i=1; i < npts; i ++) {
      if (px[i] - px[i-1] > stretch) py[i] = 0.0f;
    }

    break;
  }
  case OP_SLOW: {
    /* slow points down. stretches the points out so that
       the tail is lost, but preserves their y values. */
    
    float factor = 1.0f + op_param;

    /* We don't need to worry about maxpoints, since
       we will just be moving existing samples (and
       truncating)... */
    int i;
    for(i = 0; i < (npts-1); i ++) {
      px[i] *= factor;
      if (px[i] > samples) {
        /* this sample can't stay. */
        i --;
        break;
      }
    }
    /* but save last point */
    px[i] = px[npts-1];
    py[i] = py[npts-1];
    
    npts = i + 1;

    break;
  }
  case OP_FAST: {

    float factor = 1.0f + (op_param * 3.0f);
    float onedivfactor = 1.0f / factor;

    /* number of times we need to loop through samples */
    int times = (int)(factor + 1.0);

    int outi = 0;
    for(int rep = 0; rep < times; rep ++) {
      /* where this copy of the points begins */
      int offset = rep * (onedivfactor * samples);
      for (int s = 0; s < npts; s ++) {
        /* XXX is destx in range? */
        int destx = offset + (px[s] * onedivfactor);

        if (destx >= samples) goto op_fast_out_of_points;

        /* check if we already have one here.
           if not, add it and advance, otherwise ignore. 
           XXX: one possibility would be to mix...
        */
        if (!(outi > 0 && tempx[outi-1] == destx)) {
          tempx[outi] = destx;
          tempy[outi] = py[s];
          outi ++;
        } 

        if (outi > (maxpts - 2)) goto op_fast_out_of_points;
      }
    }

  op_fast_out_of_points:
    
    /* always save last sample, as usual */
    tempx[outi] = px[npts - 1];
    tempy[outi] = py[npts - 1];

    /* copy.. */

    for(int c = 1; c < outi; c++) {
      px[c] = tempx[c];
      py[c] = tempy[c];
    }

    npts = outi;

    break;
  }
  case OP_NONE:
  default:
    /* nothing ... */
    break;

  } /* end of main switch(op) statement */

  return npts;
}

/* this processes an individual window.
   1. generate points
   2. do operations on points (in slots op1, op2, op3)
   3. generate waveform
*/
int PLUGINCORE::processw(float * in, float * out, long samples,
                     int * px, float * py, int maxpts,
                     int * tempx, float * tempy) {

  /* collect points. */

  px[0] = 0;
  py[0] = in[0];
  int numpts = 1;

  /* MS Visual C++ is super retarded, so we have to define 
     and initialize all of these before the switch statement */
  int i = 0, extx = 0, nth = 0, ctr = 0, n = 0, sd = 0;
  float ext = 0.0f;

  switch(pointstyle) {

  case POINT_EXTNCROSS:
    /* extremities and crossings 
       XXX: Can this generate points out of order? Don't think so...
    */

    ext = 0.0f;
    extx = 0;

    enum {SZ, SZC, SA, SB};
    int state;

    state = SZ;

    for(i = 0 ; i < samples ; i ++) {
      switch(state) {
      case SZ: {
        /* just output a zero. */
        if (in[i] <= pointparam && in[i] >= -pointparam) state = SZC;
        else if (in[i] < -pointparam) { state = SB; ext = in[i]; extx = i; }
        else { state = SA; ext = in[i]; extx = i; }
        break;
      }
      case SZC: {
        /* continuing zeros */
        if (in[i] <= pointparam && in[i] >= -pointparam) break;
  
        /* push zero for last spot (we know it was a zero and not pushed). */
        if (numpts < (maxpts-1)) {
          px[numpts] = (i>0)?(i - 1):0;
          py[numpts] = 0.0f;
          numpts++;
        } 

        if (in[i] < 0.0f) { 
          state = SB;
        } else {
          state = SA;
        }

        ext = in[i];
        extx = i;
        break;
      }
      case SA: {
        /* above zero */

        if (in[i] <= pointparam) {
          /* no longer above 0. push the highest point I reached. */
          if (numpts < (maxpts-1)) {
            px[numpts] = extx;
            py[numpts] = ext;
            numpts++;
          } 
          /* and decide state */
          if (in[i] >= -pointparam) {
            if (numpts < (maxpts-1)) {
              px[numpts] = i;
              py[numpts] = 0.0f;
              numpts++;
            } 
            state = SZ;
          } else {
            state = SB;
            ext = in[i];
            extx = i;
          }
        } else {
          if (in[i] > ext) {
            ext = in[i];
            extx = i;
          }
        }
        break;
      }
      case SB: {
        /* below zero */

        if (in[i] >= -pointparam) {
          /* no longer below 0. push the lowest point I reached. */
          if (numpts < (maxpts-1)) {
            px[numpts] = extx;
            py[numpts] = ext;
            numpts++;
          } 
          /* and decide state */
          if (in[i] <= pointparam) {
            if (numpts < (maxpts-1)) {
              px[numpts] = i;
              py[numpts] = 0.0f;
              numpts++;
            } 
            state = SZ;
          } else {
            state = SA;
            ext = in[i];
            extx = i;
          }
        } else {
          if (in[i] < ext) {
            ext = in[i];
            extx = i;
          }
        }
        break;
      }
      default:;

      }
    }

    break;

  case POINT_LEVEL: {

    enum { ABOVE, BETWEEN, BELOW };

    int state = BETWEEN;
    float level = (pointparam * .9999f) + .00005f;
    numpts = 1;

    px[0] = 0;
    py[0] = in[0];

    for(i = 0; i < samples; i ++) {

      if (in[i] > pointparam) {
        if (state != ABOVE) {
          px[numpts] = i;
          py[numpts] = in[i];
          numpts ++;
          state = ABOVE;
        }
      } else if (in[i] < -pointparam) {
        if (state != BELOW) {
          px[numpts] = i;
          py[numpts] = in[i];
          numpts ++;
          state = BELOW;
        }
      } else {
        if (state != BETWEEN) {
          px[numpts] = i;
          py[numpts] = in[i];
          numpts++;
          state = BETWEEN;
        }
      }

      if (numpts > samples - 2) break;

    }

    px[numpts] = samples - 1;
    py[numpts] = in[samples - 1];

    numpts ++;

    break;
  }
  case POINT_FREQ:
    /* at frequency */

    /* XXX let the user choose hz, do conversion */
    nth = (pointparam * pointparam) * samples;
    ctr = nth;
  
    for(i = 0; i < samples; i ++) {
      ctr--;
      if (ctr <= 0) {
        if (numpts < (maxpts-1)) {
          px[numpts] = i;
          py[numpts] = in[i];
          numpts++;
        } else break; /* no point in continuing... */
        ctr = nth;
      }
    }

    break;


  case POINT_RANDOM: {
    /* randomly */

    n = (1.0f - pointparam) * samples;

    for(;n --;) {
      if (numpts < (maxpts-1)) {
        px[numpts++] = rand() % samples;
      } else break;
    }

    /* sort them */

    qsort(px, numpts, sizeof (int), intcompare);

    for (sd = 0; sd < numpts; sd++) {
      py[sd] = in[px[sd]];
    }

    break;
  }
  
  case POINT_SPAN: {
    /* next x determined by sample magnitude
       
    suggested by bram.
    */

    int span = (pointparam * pointparam) * samples;

    i = abs((int)(py[0] * span)) + 1;

    while (i < samples) {
      px[numpts] = i;
      py[numpts] = in[i];
      numpts++;
      i = i + abs((int)(in[i] * span)) + 1;
    }

    break;

  }
  
  case POINT_DYDX: {
    /* dy/dx */
    int lastsign = 0;
    float lasts = in[0];
    int sign;

    px[0] = 0;
    py[0] = in[0];
    numpts = 1;

    float pp;
    int above;
    if (pointparam > 0.5f) {
      pp = pointparam - 0.5f;
      above = 1;
    } else {
      pp = 0.5f - pointparam;
      above = 0;
    }

    pp = powf(pp, 2.7f);

    for(i = 1; i < samples; i++) {
      
      if (above)
        sign = (in[i] - lasts) > pp;
      else
        sign = (in[i] - lasts) < pp;

      lasts = in[i];

      if (sign != lastsign) {
        px[numpts] = i;
        py[numpts] = in[i];
        numpts++;
        if (numpts > (maxpts-1)) break;
      }

      lastsign = sign;
    }

    break;

  }
  default:
    /* nothing, unsupported... */
    numpts = 1;
    break;

  } /* end of pointstyle cases */


  /* always push final point for continuity (we saved room) */
  px[numpts] = samples-1;
  py[numpts] = in[samples-1];
  numpts++;

  /* modify the points according to the three slots and
     their parameters */

  numpts = pointops(pointop1, numpts, oppar1, samples, 
                    px, py, maxpts, tempx, tempy);
  numpts = pointops(pointop2, numpts, oppar2, samples, 
                    px, py, maxpts, tempx, tempy);
  numpts = pointops(pointop3, numpts, oppar3, samples, 
                    px, py, maxpts, tempx, tempy);

  int u=1, z=0;
  switch(interpstyle) {

  case INTERP_SHUFFLE: {
    /* mix around the intervals. The parameter determines
       how mobile an interval is. 

       I build an array of interval indices (integers).
       Then I swap elements with nearby elements (where
       nearness is determined by the parameter). Then
       I reconstruct the wave by reading the intervals
       in their new order.
    */
    
    /* fix last point at last sample -- necessary to
       preserve invariants */
    px[numpts-1] = samples - 1;

    int intervals = numpts - 1;

    /* generate table */
    for(int a = 0; a < intervals; a++) {
      tempx[a] = a;
    }

    for(int z = 0; z < intervals; z++) {
      if (DFX_Rand_f() < interparam) {
        int t;
        int dest = z + ((interparam * 
                         interparam * (float)intervals)
                        * DFX_Rand_f()) - (interparam *
                                          interparam *
                                          0.5f * (float)intervals);
        if (dest < 0) dest = 0;
        if (dest >= intervals) dest = intervals - 1;

        t = tempx[z];
        tempx[z] = tempx[dest];
        tempx[dest] = t;
      }
    }

    /* generate output */
    int c = 0;
    for(int u = 0; u < intervals; u++) {
      int size = px[tempx[u]+1] - px[tempx[u]];
      memcpy(out + c,
             in + px[tempx[u]],
             size * sizeof (float));
      c += size;
    }

    break;
  }

  case INTERP_FRIENDS: {
    /* bleed each segment into next segment (its "friend"). 
       interparam controls the amount of bleeding, between 
       0 samples and next-segment-size samples. 
       suggestion by jcreed (sorta).
    */

    /* copy last block verbatim. */
    if (numpts > 2)
      for(int s=px[numpts-2]; s < px[numpts-1]; s++)
        out[s] = in[s];

    /* steady state */
    int x = numpts - 2;
    for(; x > 0; x--) {
      /* x points at the beginning of the segment we'll be bleeding
         into. */
      int sizeright = px[x+1] - px[x];
      int sizeleft = px[x] - px[x-1];

      int tgtlen = sizeleft + (sizeright * interparam);

      if (tgtlen > 0) {
        /* to avoid using temporary storage, copy from end of target
           towards beginning, overwriting already used source parts on
           the way. 

           j is an offset from p[x-1], ranging from 0 to tgtlen-1.
           Once we reach p[x], we have to start mixing with the
           data that's already there.
        */
        for (int j = tgtlen - 1;
             j >= 0;
             j--) {

          /* XXX. use interpolated sampling for this */
          float wet = in[(int)(px[x-1] + sizeleft * 
                               (j/(float)tgtlen))];

          if ((j + px[x-1]) > px[x]) {
            /* after p[x] -- mix */

            /* linear fade-out */
            float pct = (j - sizeleft) / (float)(tgtlen - sizeleft);

            out[j + px[x-1]] =
              wet * (1.0f - pct) +
              out[j + px[x-1]] * pct;
          } else {
            /* before p[x] -- no mix */
            out[j + px[x-1]] = wet;
          }

        }
      }
    }

    break;
  }
  case INTERP_POLYGON:
    /* linear interpolation - "polygon" 
       interparam causes dimming effect -- at 1.0 it just does
       straight lines at the median.
    */

    for(u=1; u < numpts; u ++) {
      float denom = (px[u] - px[u-1]);
      float minterparam = interparam * (py[u-1] + py[u]) * 
        0.5f;
      for(z=px[u-1]; z < px[u]; z++) {
        float pct = (float)(z-px[u-1]) / denom;
        float s = py[u-1] * (1.0f - pct) +
          py[u]   * pct;
        out[z] = minterparam + (1.0f - interparam) * s;
      }
    }

    out[samples-1] = in[samples-1];

    break;


  case INTERP_WRONGYGON:
    /* linear interpolation, wrong direction - "wrongygon" 
       same dimming effect from polygon.
    */

    for(u=1; u < numpts; u ++) {
      float denom = (px[u] - px[u-1]);
      float minterparam = interparam * (py[u-1] + py[u]) 
        * 0.5f;
      for(z=px[u-1]; z < px[u]; z++) {
        float pct = (float)(z-px[u-1]) / denom;
        float s = py[u-1] * pct +
          py[u]   * (1.0f - pct);
        out[z] = minterparam + (1.0f - interparam) * s;
      }
    }

    out[samples-1] = in[samples-1];

    break;


  case INTERP_SMOOTHIE:
    /* cosine up or down - "smoothie" */

    for(u=1; u < numpts; u ++) {
      float denom = (px[u] - px[u-1]);
      for(z=px[u-1]; z < px[u]; z++) {
        float pct = (float)(z-px[u-1]) / denom;
        
        float p = 0.5f * (-cosf(kDFX_PI_f * pct) + 1.0f);
        
        if (interparam > 0.5f) {
          p = powf(p, (interparam - 0.16666667f) * 3.0f);
        } else {
          p = powf(p, interparam * 2.0f);
        }

        float s = py[u-1] * (1.0f - p) + py[u]   * p;

        out[z] = s;
      }
    }

    out[samples-1] = in[samples-1];

    break;


  case INTERP_REVERSI:
    /* x-reverse input samples for each waveform - "reversi" */

    for(u=1; u < numpts; u ++) {
      if (px[u-1] < px[u])
        for(z = px[u-1]; z < px[u]; z++) {
          int s = (px[u] - (z + 1)) + px[u - 1];
          out[z] = in[(s>0)?s:0];
        }
    }

    break;


  case INTERP_PULSE: {

    int wid = (int)(100.0 * interparam);
    
    for(i = 0; i < samples; i++) out[i] = 0.0f;

    for(z = 0; z < numpts; z ++) { 
      out[px[z]] = DFX_MagnitudeMax(out[px[z]], py[z]);

      if (wid > 0) {
        /* put w samples on the left, stopping if we hit a sample
           greater than what we're placing */
        int w = wid;
        float onedivwid = 1.0f / (float)(wid + 1);
        for(int i=px[z]-1; i >= 0 && w > 0; i--, w--) {
          float sam = py[z] * (w * onedivwid);
          if ((out[i] + sam) * (out[i] + sam) > (sam * sam)) out[i] = sam;
          else out[i] += sam;
        }

        w = wid;
        for(int ii=px[z]+1; ii < samples && w > 0; ii++, w--) {
          float sam = py[z] * (w * onedivwid);
          out[ii] = sam;
        }

      }
    }

    break;

  }
  case INTERP_SING:

    for(u=1; u < numpts; u ++) {
      float oodenom = 1.0f / (px[u] - px[u-1]);

      for(z=px[u-1]; z < px[u]; z++) {
        float pct = (float)(z-px[u-1]) * oodenom;
        
        float wand = sinf(2.0f * kDFX_PI_f * pct);
        out[z] = wand * 
          interparam + 
          ((1.0f-interparam) * 
           in[z] *
           wand);
      }
    }

    out[samples-1] = in[samples-1];


    break;
  default:

    /* unsupported ... ! */
    for(int i = 0; i < samples; i++) out[i] = 0.0f;

    break;

  } /* end of interpstyle cases */

  return numpts;

}


/* this fake processX function reads samples one at a time
   from the true input. It simultaneously copies samples from
   the beginning of the output buffer to the true output.
   We maintain that out0 always has at least 'third' samples
   in it; this is enough to pick up for the delay of input
   processing and to make sure we always have enough samples
   to fill the true output buffer.

   If the input frame is full:
    - calls processw on this full input frame
    - applies the windowing envelope to the tail of out0 (output frame)
    - mixes in prevmix with the first half of the output frame
    - increases outsize so that the first half of the output frame is
      now available output
    - copies the second half of the output to be prevmix for next frame.
    - copies the second half of the input buffer to the first,
      resets the size (thus we process each third-size chunk twice)

  If we have read more than 'third' samples out of the out0 buffer:
   - Slide contents to beginning of buffer
   - Reset outstart

*/

/* to improve: 
   - can we use tail of out0 as prevmix, instead of copying?
   - can we use circular buffers instead of memmoving a lot?
     (probably not)
XXX Sophia's ideas:
   - only calculate a given window the first time that it's needed (how often to we change them anyway?)
   - cache it and use it next time
   - moreover, we only need one cache for the plugin, although that might be tricky with the whole DSP cores abstraction thing
   - possibly we only need to save half since the second half is always the first half reversed
   - this can allow for an easy way to vector-optimize the windowing
   - it would also be nice to make this windowing stuff into a reusable class so that we don't find ourselves maintaining the same code accross so many different plugins
*/

#include <vecLib/vDSP.h>
#if TARGET_PLUGIN_USES_DSPCORE
void PLUGINCORE::process(const float * tin, float * tout, unsigned long samples, bool replacing) {
#else
void PLUGIN::processaudio(const float ** trueinputs, float ** trueoutputs, unsigned long samples, 
                      bool replacing) {
  const float * tin  = *trueinputs;
  float * tout = *trueoutputs;
#endif

  for (unsigned long ii = 0; ii < samples; ii++) {

    /* copy sample in */
    in0[insize] = tin[ii];
    insize ++;
 
    if (insize == framesize) {
      /* frame is full! */

      /* in0 -> process -> out0(first free space) */
      if (cs != NULL)
        cs->grab();
      processw(in0, out0+outstart+outsize, framesize,
               pointx, pointy, framesize * 2,
               storex, storey);
      if (cs != NULL)
        cs->release();

#if 0
      const float oneDivThird = 1.0f / (float)third;
      int z = 0;
      /* apply envelope */

      switch(shape) {

        // XXX it might be more efficient to do z+=oneDivThird each iteration (and then negativize oneDivThird for the second half)
        case WINDOW_TRIANGLE:
          for(z = 0; z < third; z++) {
            out0[z+outstart+outsize] *= ((float)z * oneDivThird);
            out0[z+outstart+outsize+third] *= (1.0f - ((float)z * oneDivThird));
          }
          break;
        case WINDOW_ARROW:
          for(z = 0; z < third; z++) {
            float p = (float)z * oneDivThird;
            p *= p;
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
        case WINDOW_WEDGE:
          for(z = 0; z < third; z++) {
            float p = sqrtf((float)z * oneDivThird);
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
        case WINDOW_COS:
          for(z = 0; z < third; z ++) {
            // XXX what about Eulor's law for cosine?
            float p = 0.5f * (-cosf(kDFX_PI_f * ((float)z * oneDivThird)) + 1.0f);
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
      }
#else
      vmul(out0+outstart+outsize, 1, windowbuf, 1, out0+outstart+outsize, 1, (UInt32)framesize);
//for (int z=0; z < framesize; z++) out0[z+outstart+outsize] *= windowbuf[z];
#endif

      /* mix in prevmix */
      for(int u = 0; u < third; u ++)
        out0[u+outstart+outsize] += prevmix[u];

      /* prevmix becomes out1 */
      memcpy(prevmix, out0 + outstart + outsize + third, third * sizeof (float));

      /* copy 2nd third of input over in0 (need to re-use it for next frame), 
         now insize = third */
      memcpy(in0, in0 + third, third * sizeof (float));

      insize = third;
      
      outsize += third;
    }

    /* send sample out */
  #ifdef TARGET_API_VST
    if (!replacing)
      tout[ii] += out0[outstart];
    else
  #endif
      tout[ii] = out0[outstart];

    outstart ++;
    outsize --;

    /* make sure there is always enough room for a frame in out buffer */
    if (outstart == third) {
      memmove(out0, out0 + outstart, outsize * sizeof (float));
      outstart = 0;
    }
  }
}


void PLUGIN::makepresets() {
  int i = 1;

  setpresetname(i, "atonal singing");
  setpresetparameter_i(i, P_BUFSIZE, 9);	// XXX is that 2^11 ?
  setpresetparameter_i(i, P_POINTSTYLE, POINT_FREQ);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_FREQ, 0.10112);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_REVERSI);
  i++;

  setpresetname(i, "robo sing (A)");
  setpresetparameter_i(i, P_BUFSIZE, 9);
  setpresetparameter_i(i, P_SHAPE, WINDOW_COS);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_DYDX);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_DYDX, 0.1250387420637675);//0.234);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SING);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SING, 1.0);
  setpresetparameter_i(i, P_POINTOP1, OP_FAST);
  setpresetparameter_f(i, P_OPPAR1S + OP_FAST, 0.9157304);
  i++;

  setpresetname(i, "sploop drums");
  setpresetparameter_i(i, P_BUFSIZE, 9);
  setpresetparameter_i(i, P_SHAPE, WINDOW_TRIANGLE);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_DYDX);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_DYDX, 0.5707532982591033);//0.528);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SING);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SING, 0.2921348);
  setpresetparameter_i(i, P_POINTOP1, OP_QUARTER);
  setpresetparameter_f(i, P_OPPAR1S + OP_QUARTER, 0.258427);
  setpresetparameter_i(i, P_POINTOP2, OP_DOUBLE);
  setpresetparameter_f(i, P_OPPAR2S + OP_DOUBLE, 0.5);
  i++;

  setpresetname(i, "loudest sing");
  setpresetparameter_i(i, P_BUFSIZE, 9);
  setpresetparameter_i(i, P_SHAPE, WINDOW_TRIANGLE);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_LEVEL);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_LEVEL, 0.280899);
  setpresetparameter_i(i, P_POINTOP2, OP_LONGPASS);
  setpresetparameter_f(i, P_OPPAR2S + OP_LONGPASS, 0.1404494);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SING);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SING, 0.8258427);
  i++;

  setpresetname(i, "slower");
  setpresetparameter_i(i, P_BUFSIZE, 13);
  setpresetparameter_i(i, P_SHAPE, WINDOW_COS);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_FREQ);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_FREQ, 0.3089887);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_FRIENDS);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_FRIENDS, 1.0);
  i++;

  setpresetname(i, "space chamber");
  setpresetparameter_i(i, P_BUFSIZE, 13);
  setpresetparameter_i(i, P_SHAPE, WINDOW_COS);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_FREQ);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_FREQ, 0.0224719);
  setpresetparameter_i(i, P_POINTOP2, OP_FAST);
  setpresetparameter_f(i, P_OPPAR2S + OP_FAST, 0.7247191);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SMOOTHIE);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SMOOTHIE, 0.5);
  i++;

  setpresetname(i, "robo sing (B)");
  setpresetparameter_i(i, P_BUFSIZE, 10);
  setpresetparameter_i(i, P_SHAPE, WINDOW_TRIANGLE);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_RANDOM);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_RANDOM, 0.0224719);
  setpresetparameter_i(i, P_POINTOP1, OP_LONGPASS);
  setpresetparameter_f(i, P_OPPAR1S + OP_LONGPASS, 0.1966292);
  setpresetparameter_i(i, P_POINTOP2, OP_FAST);
  setpresetparameter_f(i, P_OPPAR2S + OP_FAST, 1.0);
  setpresetparameter_i(i, P_POINTOP3, OP_FAST);
  setpresetparameter_f(i, P_OPPAR3S + OP_FAST, 1.0);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_POLYGON);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_POLYGON, 0.0);
  i++;
  
  setpresetname(i, "scrubby chorus");
  setpresetparameter_i(i, P_BUFSIZE, 13);
  setpresetparameter_i(i, P_SHAPE, WINDOW_ARROW);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_RANDOM);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_RANDOM, 0.9775281);
  setpresetparameter_i(i, P_POINTOP1, OP_LONGPASS);
  setpresetparameter_f(i, P_OPPAR1S + OP_LONGPASS, 0.5168539);
  setpresetparameter_i(i, P_POINTOP2, OP_FAST);
  setpresetparameter_f(i, P_OPPAR2S + OP_FAST, 0.0617978);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_FRIENDS);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_FRIENDS, 0.7303371);
  i++;

  setpresetname(i, "time shuffle echo (sux?)");
  setpresetparameter_i(i, P_BUFSIZE, 13);
  setpresetparameter_i(i, P_SHAPE, WINDOW_COS);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_DYDX);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_DYDX, 0.81);
  setpresetparameter_i(i, P_POINTOP1, OP_LONGPASS);
  setpresetparameter_f(i, P_OPPAR1S + OP_LONGPASS, 0.183);
  setpresetparameter_i(i, P_POINTOP2, OP_NONE);
  setpresetparameter_i(i, P_POINTOP3, OP_NONE);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SHUFFLE);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SHUFFLE, 0.84);
  i++;

}
