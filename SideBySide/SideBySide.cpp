/*
OFX SideBySide plugin.
Put the left and right view of the input next to each other.

Copyright (C) 2013 INRIA
Author Frederic Devernay frederic.devernay@inria.fr

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  Neither the name of the {organization} nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

INRIA
Domaine de Voluceau
Rocquencourt - B.P. 105
78153 Le Chesnay Cedex - France


The skeleton for this source file is from:
OFX Basic Example plugin, a plugin that illustrates the use of the OFX Support library.

Copyright (C) 2004-2005 The Open Effects Association Ltd
Author Bruno Nicoletti bruno@thefoundry.co.uk

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name The Open Effects Association Ltd, nor the names of its 
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The Open Effects Association Ltd
1 Wardour St
London W1D 6PA
England

*/

#ifdef _WINDOWS
#include <windows.h>
#endif

#include <stdio.h>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"

#include "../include/ofxsProcessing.H"


// Base class for the RGBA and the Alpha processor
class SideBySideBase : public OFX::ImageProcessor {
protected :
  OFX::Image *_srcImg1;
  OFX::Image *_srcImg2;
  bool _vertical;
  int _offset;
public :
  /** @brief no arg ctor */
  SideBySideBase(OFX::ImageEffect &instance)
    : OFX::ImageProcessor(instance)
    , _srcImg1(0)
    , _srcImg2(0)
    , _vertical(false)
    , _offset(0)
  {        
  }

  /** @brief set the left src image */
  void setSrcImg1(OFX::Image *v) {_srcImg1 = v;}

  /** @brief set the right src image */
  void setSrcImg2(OFX::Image *v) {_srcImg2 = v;}

  /** @brief set vertical stacking and offset oin the vertical or horizontal direction */
  void setVerticalAndOffset(bool v, int offset) {_vertical = v; _offset = offset;}
};

// template to do the RGBA processing
template <class PIX, int nComponents, int max>
class ImageSideBySide : public SideBySideBase {
public :
  // ctor
  ImageSideBySide(OFX::ImageEffect &instance) 
    : SideBySideBase(instance)
  {}

  // and do some processing
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    assert(_offset != 0);
    for(int y = procWindow.y1; y < procWindow.y2; y++) {
      if(_effect.abort()) break;

      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);

      for(int x = procWindow.x1; x < procWindow.x2; x++) {
        PIX *srcPix;
        if ((_vertical && y >= _offset) || (!_vertical && x < _offset)) {
            srcPix = (PIX *)(_srcImg1 ? _srcImg1->getPixelAddress(x, _vertical ? y - _offset : y) : 0);
        } else {
            srcPix = (PIX *)(_srcImg2 ? _srcImg2->getPixelAddress(_vertical ? x : x - _offset, y ) : 0);
        }

          if (srcPix) {
              for(int c = 0; c < nComponents; c++) {
                  dstPix[c] = srcPix[c];
              }
          } else {
              // no data here, be black and transparent
              for(int c = 0; c < nComponents; c++) {
                  dstPix[c] = 0;
              }
          }

        // increment the dst pixel
        dstPix += nComponents;
      }
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class SideBySidePlugin : public OFX::ImageEffect {
protected :
  // do not need to delete these, the ImageEffect is managing them for us
  OFX::Clip *dstClip_;
  OFX::Clip *srcClip_;

  OFX::BooleanParam *vertical_;
  OFX::ChoiceParam *view1_;
  OFX::ChoiceParam *view2_;

public :
  /** @brief ctor */
  SideBySidePlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , srcClip_(0)
    , vertical_(0)
    , view1_(0)
    , view2_(0)
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    vertical_ = fetchBooleanParam("vertical");
    view1_ = fetchChoiceParam("view1");
    view2_ = fetchChoiceParam("view2");
  }

  /* Override the render */
  virtual void render(const OFX::RenderArguments &args);

  // override the rod call
  virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod);

  // override the roi call
  virtual void getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois);

  /* set up and run a processor */
  void setupAndProcess(SideBySideBase &, const OFX::RenderArguments &args);
};


////////////////////////////////////////////////////////////////////////////////
/** @brief render for the filter */

////////////////////////////////////////////////////////////////////////////////
// basic plugin render function, just a skelington to instantiate templates from


/* set up and run a processor */
void
SideBySidePlugin::setupAndProcess(SideBySideBase &processor, const OFX::RenderArguments &args)
{
  // get a dst image
  std::auto_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
  OFX::BitDepthEnum dstBitDepth       = dst->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dst->getPixelComponents();

  // fetch main input image
  int view1;
  view1_->getValue(view1);
  int view2;
  view2_->getValue(view2);
  std::auto_ptr<OFX::Image> src1(srcClip_->fetchStereoscopicImage(args.time,view1));
  std::auto_ptr<OFX::Image> src2(srcClip_->fetchStereoscopicImage(args.time,view2));

  // make sure bit depths are sane
  if(src1.get()) {
    OFX::BitDepthEnum    srcBitDepth      = src1->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src1->getPixelComponents();

    // see if they have the same depths and bytes and all
    if(srcBitDepth != dstBitDepth || srcComponents != dstComponents)
      throw int(1); // HACK!! need to throw an sensible exception here!
  }
  if(src2.get()) {
    OFX::BitDepthEnum    srcBitDepth      = src2->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src2->getPixelComponents();

    // see if they have the same depths and bytes and all
    if(srcBitDepth != dstBitDepth || srcComponents != dstComponents)
      throw int(1); // HACK!! need to throw an sensible exception here!
  }

  bool vertical = vertical_->getValue();
  OfxPointD offset = getProjectOffset();
  OfxPointD size = getProjectSize();

  // our RoD is defined with respect to the 'Source' clip's, we are not interested in the mask
  OfxRectD rod = srcClip_->getRegionOfDefinition(args.time);

  // clip to the project rect
  //rod.x1 = std::max(rod.x1,offset.x);
  rod.x2 = std::min(rod.x2,offset.x+size.x);
  //rod.y1 = std::max(rod.y1,offset.y);
  rod.y2 = std::min(rod.y2,offset.y+size.y);

  // set the images
  processor.setDstImg(dst.get());
  processor.setSrcImg1(src1.get());
  processor.setSrcImg2(src2.get());

  // set the render window
  processor.setRenderWindow(args.renderWindow);

  // set the parameters
  processor.setVerticalAndOffset(vertical, vertical?rod.y2:rod.x2);

  // Call the base class process member, this will call the derived templated process code
  processor.process();
}

// override the rod call
bool
SideBySidePlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
  bool vertical = vertical_->getValue();
  OfxPointD offset = getProjectOffset();
  OfxPointD size = getProjectSize();

  // our RoD is defined with respect to the 'Source' clip's, we are not interested in the mask
  rod = srcClip_->getRegionOfDefinition(args.time);

  // clip to the project rect
  rod.x1 = std::max(rod.x1,offset.x);
  rod.x2 = std::min(rod.x2,offset.x+size.x);
  rod.y1 = std::max(rod.y1,offset.y);
  rod.y2 = std::min(rod.y2,offset.y+size.y);

  // the RoD is twice the size of the original ROD in one direction
  if (vertical) {
    rod.y2 = rod.y1 + 2*(rod.y2-rod.y1);
  } else {
    rod.x2 = rod.x1 + 2*(rod.x2-rod.x1);
  }
  // say we set it
  return true;
}

// override the roi call
void 
SideBySidePlugin::getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois)
{
  bool vertical = vertical_->getValue();
  OfxPointD offset = getProjectOffset();
  OfxPointD size = getProjectSize();

  // our RoD is defined with respect to the 'Source' clip's, we are not interested in the mask
  OfxRectD rod = srcClip_->getRegionOfDefinition(args.time);

  // clip to the project rect
  rod.x1 = std::max(rod.x1,offset.x);
  rod.x2 = std::min(rod.x2,offset.x+size.x);
  rod.y1 = std::max(rod.y1,offset.y);
  rod.y2 = std::min(rod.y2,offset.y+size.y);

  // ask for either a horizontal or a vertical stripe, to avoid the numerous comparisons
  OfxRectD roi = args.regionOfInterest;
  if (vertical) {
    roi.y1 = rod.y1;
    roi.y2 = rod.y2;
  } else {
    roi.x1 = rod.x1;
    roi.x2 = rod.x2;
  }
  rois.setRegionOfInterest(*srcClip_, roi);

  // set it on the mask only if we are in an interesting context
  //if(getContext() != OFX::eContextFilter)
  //  rois.setRegionOfInterest(*maskClip_, roi);
}

// the overridden render function
void
SideBySidePlugin::render(const OFX::RenderArguments &args)
{
  if (!OFX::fetchSuite(kOfxVegasStereoscopicImageEffectSuite, 1, true)) {
    OFX::throwHostMissingSuiteException(kOfxVegasStereoscopicImageEffectSuite);
  }

  // instantiate the render code based on the pixel depth of the dst clip
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();

  // do the rendering
  if(dstComponents == OFX::ePixelComponentRGBA) {
    switch(dstBitDepth) {
      case OFX::eBitDepthUByte : {      
        ImageSideBySide<unsigned char, 4, 255> fred(*this);
        setupAndProcess(fred, args);
      }
        break;

      case OFX::eBitDepthUShort : {
        ImageSideBySide<unsigned short, 4, 65535> fred(*this);
        setupAndProcess(fred, args);
      }                          
        break;

      case OFX::eBitDepthFloat : {
        ImageSideBySide<float, 4, 1> fred(*this);
        setupAndProcess(fred, args);
      }
        break;
      default :
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  }
  else {
    switch(dstBitDepth) {
      case OFX::eBitDepthUByte : {
        ImageSideBySide<unsigned char, 1, 255> fred(*this);
        setupAndProcess(fred, args);
      }
        break;

      case OFX::eBitDepthUShort : {
        ImageSideBySide<unsigned short, 1, 65535> fred(*this);
        setupAndProcess(fred, args);
      }                          
        break;

      case OFX::eBitDepthFloat : {
        ImageSideBySide<float, 1, 1> fred(*this);
        setupAndProcess(fred, args);
      }                          
        break;
      default :
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  }
}

mDeclarePluginFactory(SideBySidePluginFactory, ;, {});

using namespace OFX;
void SideBySidePluginFactory::load()
{
    // we can't be used on hosts that don't support the stereoscopic suite
    if (!OFX::fetchSuite(kOfxVegasStereoscopicImageEffectSuite, 1, true)) {
        throwHostMissingSuiteException(kOfxVegasStereoscopicImageEffectSuite);
    }
}

void SideBySidePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
  // basic labels
  desc.setLabels("SideBySideOFX", "SideBySideOFX", "SideBySideOFX");
  desc.setPluginGrouping("Views/Stereo");
  desc.setPluginDescription("Put the left and right view of the input next to each other.");

  // add the supported contexts, only filter at the moment
  desc.addSupportedContext(eContextFilter);

  // add supported pixel depths
  desc.addSupportedBitDepth(eBitDepthUByte);
  desc.addSupportedBitDepth(eBitDepthUShort);
  desc.addSupportedBitDepth(eBitDepthFloat);

  // set a few flags
  desc.setSingleInstance(false);
  desc.setHostFrameThreading(false);
  desc.setSupportsMultiResolution(true);
  desc.setSupportsTiles(true);
  desc.setTemporalClipAccess(false);
  desc.setRenderTwiceAlways(false);
  desc.setSupportsMultipleClipPARs(false);

  if (!OFX::fetchSuite(kOfxVegasStereoscopicImageEffectSuite, 1, true)) {
    throwHostMissingSuiteException(kOfxVegasStereoscopicImageEffectSuite);
  }
}

void SideBySidePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context)
{
  // Source clip only in the filter context
  // create the mandated source clip
  ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
  srcClip->addSupportedComponent(ePixelComponentRGBA);
  srcClip->addSupportedComponent(ePixelComponentAlpha);
  srcClip->setTemporalClipAccess(false);
  srcClip->setSupportsTiles(true);
  srcClip->setIsMask(false);

  // create the mandated output clip
  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  srcClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);

  // make some pages and to things in 
  PageParamDescriptor *page = desc.definePageParam("Controls");

  BooleanParamDescriptor *vertical = desc.defineBooleanParam("vertical");
  vertical->setDefault(false);
  vertical->setHint("Stack views vertically instead of horizontally");
  vertical->setLabels("vertical", "vertical", "vertical");
  vertical->setAnimates(false); // no animation here!

  page->addChild(*vertical);

  ChoiceParamDescriptor *view1 = desc.defineChoiceParam("view1");
  view1->setHint("First view");
  view1->setLabels("view1", "view1", "view1");
  view1->appendOption("Left", "Left");
  view1->appendOption("Right", "Right");
  view1->setDefault(0);
  view1->setAnimates(false); // no animation here!

  page->addChild(*view1);

  ChoiceParamDescriptor *view2 = desc.defineChoiceParam("view2");
  view2->setHint("Second view");
  view2->setLabels("view2", "view2", "view2");
  view2->appendOption("Left", "Left");
  view2->appendOption("Right", "Right");
  view2->setDefault(1);
  view2->setAnimates(false); // no animation here!

  page->addChild(*view2);
}

OFX::ImageEffect* SideBySidePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context)
{
  return new SideBySidePlugin(handle);
}

namespace OFX 
{
  namespace Plugin 
  {  
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
      static SideBySidePluginFactory p("net.sf.openfx:sideBySidePlugin", 1, 0);
      ids.push_back(&p);
    }
  }
}