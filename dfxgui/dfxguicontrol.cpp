#include "dfxgui.h"


//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor *inOwnerEditor, AudioUnitParameterID inParamID, DGRect *inWhere)
:	ownerEditor(inOwnerEditor)
{
	auvp = AUVParameter(ownerEditor->GetEditAudioUnit(), inParamID, kAudioUnitScope_Global, (AudioUnitElement)0);
	AUVPattached = true;
	Range = auvp.ParamInfo().maxValue - auvp.ParamInfo().minValue;
	
	init(inWhere);
}

//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor *inOwnerEditor, DGRect *inWhere, float inRange)
:	ownerEditor(inOwnerEditor), Range(inRange)
{
	auvp = AUVParameter();	// an empty AUVParameter
	AUVPattached = false;

	init(inWhere);
}

//-----------------------------------------------------------------------------
// common constructor stuff
void DGControl::init(DGRect *inWhere)
{
	where.set(inWhere);
	vizArea.set(inWhere);

	Daddy = NULL;
	children = NULL;
	carbonControl = NULL;
	auv_control = NULL;

	id = 0;
	lastUpdatedValue = 0;
	tolerance = 1;
	opaque = true;
	becameVisible = false;
	pleaseUpdate = false;
	setContinuousControl(false);
}

//-----------------------------------------------------------------------------
DGControl::~DGControl()
{
	if (carbonControl != NULL)
		DisposeControl(carbonControl);
	if (children != NULL)
		delete children;
	if (auv_control != NULL)
		delete auv_control;
}


//-----------------------------------------------------------------------------
// force a redraw
void DGControl::redraw()
{
	pleaseUpdate = true;
	if (getCarbonControl() != NULL)
		Draw1Control( getCarbonControl() );
}

//-----------------------------------------------------------------------------
void DGControl::createAUVcontrol()
{
	AUCarbonViewControl::ControlType ctype;
	ctype = isContinuousControl() ? AUCarbonViewControl::kTypeContinuous : AUCarbonViewControl::kTypeDiscrete;
	if (auv_control != NULL)
		delete auv_control;
	auv_control = new AUCarbonViewControl(getDfxGuiEditor(), getDfxGuiEditor()->getParameterListener(), ctype, getAUVP(), getCarbonControl());
	auv_control->Bind();
	// set the Carbon control value according to the current parameter value
	auv_control->Update(true);
}

//-----------------------------------------------------------------------------
void DGControl::setParameterID(AudioUnitParameterID inParameterID)
{
	if (inParameterID == 0xFFFFFFFF)	// XXX do something about this
		AUVPattached = false;
	else if (inParameterID != auvp.mParameterID)	// only do this if it's a change
	{
		AUVPattached = true;
		auvp = AUVParameter(getDfxGuiEditor()->GetEditAudioUnit(), inParameterID, kAudioUnitScope_Global, (AudioUnitElement)0);
		createAUVcontrol();
	}
}

//-----------------------------------------------------------------------------
void DGControl::setOffset(SInt32 x, SInt32 y)
{
	this->where.offset (x, y);
	this->vizArea.offset (x, y);

	if (this->children != NULL)
	{
		DGControl* current = children;
		while (current != NULL)
		{
			current->setOffset (x, y);
			current = (DGControl*)(current->getNext());
		}
	}
}

//-----------------------------------------------------------------------------
void DGControl::clipRegion(bool drawing)
{
/*	
	if (drawing)
		printf ("drawing id %lld type %ld\n", getID(), getType());
	else
		printf ("clipping id %ld type %ld\n", getID(), getType());
*/
	if (opaque)
	{
		Rect r;
		where.copyToRect(&r);
		FrameRect(&r);
//		printf("clipping opaque %d %d %d %d\n", r.left, r.top, r.right, r.bottom);
	}
	
	if (!opaque || drawing)
	{		
		DGControl* current = children;
		while(current != NULL)
		{
			current->clipRegion(false);
			current = (DGControl*)current->getNext();
		}
	}
}

//-----------------------------------------------------------------------------
void DGControl::setVisible(bool viz)
{
	if (carbonControl != NULL)
	{
		if (viz)
		{
			ShowControl (carbonControl);
			becameVisible = true;
			Draw1Control (carbonControl);
			
		}
		else
			HideControl (carbonControl);
	}
	
	if (children != NULL)
	{
		DGControl* current = children;
		while(current != NULL)
		{
			current->setVisible (viz);
			current = (DGControl*)current->getNext();
		}
	}
}

//-----------------------------------------------------------------------------
void DGControl::idle()
{
	if (children != NULL)
		children->idle();
	if (getNext() != NULL)
		((DGControl*)getNext())->idle();
}
	
//-----------------------------------------------------------------------------
bool DGControl::isControlRef(ControlRef inControl)
{
	if (carbonControl == inControl)
		return true;
	if (children != NULL)
	{
		DGControl* current = children;
		while(current != NULL)
		{
			if ( current->isControlRef(inControl) )
				return true;
			current = (DGControl*) current->getNext();
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
DGControl * DGControl::getChild(ControlRef inControl)
{
	if (carbonControl == inControl)
		return this;
	if (children != NULL)
	{
		DGControl* current = children;
		while(current != NULL)
		{
			if ( current->isControlRef(inControl) )
				return current->getChild(inControl);
		
			current = (DGControl*) current->getNext();
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
void DGControl::setForeBounds(SInt32 x, SInt32 y, SInt32 w, SInt32 h)
{
	vizArea.set(x, y, w, h);
}

//-----------------------------------------------------------------------------
void DGControl::shrinkForeBounds(SInt32 inXoffset, SInt32 inYoffset, SInt32 inWidthShrink, SInt32 inHeightShrink)
{
	vizArea.offset(inXoffset, inYoffset, -inWidthShrink, -inHeightShrink);
}

//-----------------------------------------------------------------------------
bool DGControl::mustUpdate()
{
	if (carbonControl == NULL)
		return true; // probably no user interaction

	SInt32 val = GetControl32BitValue(carbonControl);
	SInt32 diff = val - lastUpdatedValue;

	if ( !(ownerEditor->isRelaxed()) || becameVisible )
	{
		becameVisible = false;
		lastUpdatedValue = val;
		return true;
	}

	// XXX ummm...  if ownerEditor is not relaxed, it will be caught by the previous if statement ???
	if ( (abs(diff) >= abs(tolerance)) || !(ownerEditor->isRelaxed()) || pleaseUpdate )
	{
		lastUpdatedValue = val;
		return true;
	}

	pleaseUpdate = false;
//	printf("DGControl::mustUpdate() draw prevented!\n");
	return false;
}
