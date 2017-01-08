//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2017 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <assert.h>
#include "jianpufactory.h"
#include "staff.h"
#include "stafftype.h"
#include "jianpuchord.h"
#include "jianpunote.h"
#include "jianpurest.h"
#include "jianpuhook.h"
#include "jianpubeam.h"


namespace Ms {

Chord* JianpuFactory::makeChord(Score* s)
      {
      return new JianpuChord(s);
      }

Note* JianpuFactory::makeNote(Score* s)
      {
      return new JianpuNote(s);
      }

Rest* JianpuFactory::makeRest(Score* s)
      {
      return new JianpuRest(s);
      }

Hook* JianpuFactory::makeHook(Score* s)
      {
      return new JianpuHook(s);
      }

Beam* JianpuFactory::makeBeam(Score* s)
      {
      return new JianpuBeam(s);
      }

Element* JianpuFactory::cloneElement(const Element* other, bool link)
      {
      assert(other != nullptr);
      if (other->isChord())
            return cloneChord(toChord(other), link);
      else if (other->isNote())
            return cloneNote(toNote(other), link);
      else if (other->isRest())
            return cloneRest(toRest(other), link);
      else if (other->isHook())
            return cloneHook(toHook(other), link);
      else if (other->isBeam())
            return cloneBeam(toBeam(other), link);
      else
            return nullptr;
      }

Chord* JianpuFactory::cloneChord(const Chord* other, bool link)
      {
      assert(other != nullptr);
      const JianpuChord* jp = dynamic_cast<const JianpuChord*>(other);
      if (jp)
            return new JianpuChord(*jp, link, this);
      else
            return new JianpuChord(*other, link, this);
      }

Note* JianpuFactory::cloneNote(const Note* other, bool link)
      {
      assert(other != nullptr);
      const JianpuNote* jp = dynamic_cast<const JianpuNote*>(other);
      if (jp)
            return new JianpuNote(*jp, link);
      else
            return new JianpuNote(*other, link);
      }

Rest* JianpuFactory::cloneRest(const Rest* other, bool link)
      {
      assert(other != nullptr);
      const JianpuRest* jp = dynamic_cast<const JianpuRest*>(other);
      if (jp)
            return new JianpuRest(*jp, link);
      else
            return new JianpuRest(*other, link);
      }

Hook* JianpuFactory::cloneHook(const Hook* other, bool link)
      {
      assert(other != nullptr);
      const JianpuHook* jp = dynamic_cast<const JianpuHook*>(other);
      if (jp)
            return new JianpuHook(*jp);
      else
            return new JianpuHook(*other);
      }

Beam* JianpuFactory::cloneBeam(const Beam* other, bool link)
      {
      assert(other != nullptr);
      const JianpuBeam* jp = dynamic_cast<const JianpuBeam*>(other);
      if (jp)
            return new JianpuBeam(*jp);
      else
            return new JianpuBeam(*other);
      }

} // namespace Ms
