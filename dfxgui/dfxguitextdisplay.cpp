#include "dfxguidisplay.h"


//-----------------------------------------------------------------------------
void genericDisplayTextProcedure(Float32 value, char *outText, void *userData);
void genericDisplayTextProcedure(Float32 value, char *outText, void *userData)
{
	if (outText != NULL)
		sprintf(outText, "%.1f", value);
}



//-----------------------------------------------------------------------------
DGTextDisplay::DGTextDisplay(DfxGuiEditor *			inOwnerEditor,
							AudioUnitParameterID	inParamID, 
							DGRect *				inRegion,
							displayTextProcedure	inTextProc, 
							void *					inUserData,
							DGGraphic *				inBackground, 
							const char *			inFontName)
:	DGControl(inOwnerEditor, inParamID, inRegion), 
	BackGround(inBackground)
{
	if (inTextProc == NULL)
		textProc = genericDisplayTextProcedure;
	else
		textProc = inTextProc;
	if (inUserData == NULL)
		textProcUserData = this;
	else
		textProcUserData = inUserData;

	fontName = (char*) malloc(256);
	if (inFontName != NULL)
		strcpy(fontName, inFontName);
	else
	{
		// get the application font from the system "theme"
		Str255 appfontname;
		OSStatus themeErr = GetThemeFont(kThemeApplicationFont, smSystemScript, appfontname, NULL, NULL);
		if (themeErr == noErr)
		{
			// cheapo Pascal-string to C-string conversion
			appfontname[appfontname[0]+1] = 0;
			strcpy( fontName, (char*) &(appfontname[1]) );
		}
		else
		{
			// what else can we do?
			free(fontName);
			fontName = NULL;
		}
	}

	fontSize = 14.0f;
	fontColor(103, 161, 215);

	setType(kDfxGuiType_display);
	setContinuousControl(true);
	alignment = kDGTextAlign_right;
}

//-----------------------------------------------------------------------------
DGTextDisplay::~DGTextDisplay()
{
	BackGround = NULL;
	textProc = NULL;
	textProcUserData = NULL;
	if (fontName != NULL)
		free(fontName);
	fontName = NULL;
}


//-----------------------------------------------------------------------------
void DGTextDisplay::mouseDown(Point inPos, bool, bool)
{
	last_Y = inPos.v;
}

//-----------------------------------------------------------------------------
void DGTextDisplay::mouseTrack(Point inPos, bool with_option, bool with_shift)
{
	ControlRef carbonControl = getCarbonControl();
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 val = GetControl32BitValue(carbonControl);

	SInt32 precision = 4;
	if (with_shift)
		precision = 10;
//	SInt32 subtle = (SInt32) getResolution();
	SInt32 subtle = 10;
	if ( inPos.h > (getBounds()->w / 2) )
		subtle = 1;

	SInt32 dy = last_Y - inPos.v;
	if (abs(dy) > precision)
	{
		dy /= precision;
		val += dy * subtle;
		if (val > max)
			val = max;
		if (val < 0)
			val = 0;
		SetControl32BitValue(carbonControl, val);
		last_Y = inPos.v;
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::mouseUp(Point inPos, bool, bool)
{
}

//-----------------------------------------------------------------------------
void DGTextDisplay::draw(CGContextRef inContext, UInt32 inPortHeight)
{
	CGRect bounds = getBounds()->convertToCGRect(inPortHeight);

	CGImageRef theBack = NULL;
	if (BackGround != NULL)
		theBack = BackGround->getCGImage();
	if (theBack == NULL)
	{
// XXX hmmm, I need to do something else to check on this; we may just want to draw on top of the background
#if 0
		CGContextSetRGBFillColor(inContext, 59.0f/255.0f, 83.0f/255.0f, 165.0f/255.0f, 1.0f);
		CGContextFillRect(inContext, bounds);
#else
		getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);
#endif
	}
	else
	{
CGRect whole;
whole = bounds;
whole.size.width = (float) BackGround->getWidth();
whole.size.height = (float) BackGround->getHeight();
whole.origin.x -= (float)where.x - getDfxGuiEditor()->GetXOffset();
whole.origin.y -= (float) (BackGround->getHeight() - (where.y - getDfxGuiEditor()->GetYOffset()) - where.h);
//		CGContextDrawImage(inContext, bounds, theBack);
		CGContextDrawImage(inContext, whole, theBack);
	}

	if (textProc != NULL)
	{
		char text[256];
		text[0] = 0;
		textProc(auvp.GetValue(), text, textProcUserData);
		drawText(inContext, bounds, text);
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::drawText(CGContextRef inContext, CGRect& inBounds, const char *inString)
{
	if (inString == NULL)
		return;

	if (fontName != NULL)
	{
		CGContextSelectFont(inContext, fontName, fontSize, kCGEncodingMacRoman);
		if (strcmp(fontName, "snoot.org pixel10") == 0)
		{
			CGContextSetShouldSmoothFonts(inContext, false);
			CGContextSetShouldAntialias(inContext, false);	// it appears that I gotta do this, too
		}
	}
	CGContextSetRGBFillColor(inContext, (float)fontColor.r/255.0f, (float)fontColor.g/255.0f, (float)fontColor.b/255.0f, 1.0f);

	if (alignment != kDGTextAlign_left)
	{
		CGContextSetTextDrawingMode(inContext, kCGTextInvisible);
		CGContextShowTextAtPoint(inContext, 0.0f, 0.0f, inString, strlen(inString));
		CGPoint pt = CGContextGetTextPosition(inContext);
		if (alignment == kDGTextAlign_center)
			inBounds.origin.x += (inBounds.size.width - pt.x) / 2.0f;
		else if (alignment == kDGTextAlign_right)
			inBounds.origin.x += inBounds.size.width - pt.x;
	}
	CGContextSetTextDrawingMode(inContext, kCGTextFill);
	CGContextShowTextAtPoint(inContext, inBounds.origin.x, inBounds.origin.y+2.0f, inString, strlen(inString));
}






//-----------------------------------------------------------------------------
DGStaticTextDisplay::DGStaticTextDisplay(DfxGuiEditor *inOwnerEditor, DGRect *inRegion, DGGraphic *inBackground, const char *inFontName)
:	DGTextDisplay(inOwnerEditor, 0xFFFFFFFF, inRegion, NULL, NULL, inBackground, inFontName), 
	displayString(NULL)
{
	displayString = (char*) malloc(256);
	displayString[0] = 0;
	AUVPattached = false;	// XXX good enough?
}

//-----------------------------------------------------------------------------
DGStaticTextDisplay::~DGStaticTextDisplay()
{
	if (displayString != NULL)
		free(displayString);
	displayString = NULL;
}

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::setText(const char *inNewText)
{
	if (inNewText == NULL)
		return;
	strcpy(displayString, inNewText);
	redraw();
}

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::draw(CGContextRef inContext, UInt32 inPortHeight)
{
	CGRect bounds = getBounds()->convertToCGRect(inPortHeight);

	CGImageRef theBack = NULL;
	if (BackGround != NULL)
		theBack = BackGround->getCGImage();
	if (theBack == NULL)
	{
// XXX hmmm, I need to do something else to check on this; we may just want to draw on top of the background
#if 0
		CGContextSetRGBFillColor(inContext, 59.0f/255.0f, 83.0f/255.0f, 165.0f/255.0f, 1.0f);
		CGContextFillRect(inContext, bounds);
#else
		getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);
#endif
	}
	else
	{
CGRect whole;
whole = bounds;
whole.size.width = (float) BackGround->getWidth();
whole.size.height = (float) BackGround->getHeight();
whole.origin.x -= (float)where.x - getDfxGuiEditor()->GetXOffset();
whole.origin.y -= (float) (BackGround->getHeight() - (where.y - getDfxGuiEditor()->GetYOffset()) - where.h);
//		CGContextDrawImage(inContext, bounds, theBack);
		CGContextDrawImage(inContext, whole, theBack);
	}

	drawText(inContext, bounds, displayString);
}
