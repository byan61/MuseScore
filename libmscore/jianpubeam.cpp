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

#include "jianpubeam.h"
#include "jianpunote.h"
#include "element.h"
#include "measure.h"
#include "system.h"
#include "chordrest.h"
#include "chord.h"
#include "rest.h"

namespace Ms {

JianpuBeam::JianpuBeam(Score* s)
   : Beam(s)
      {
      }

JianpuBeam::JianpuBeam(const Beam& b)
   : Beam(b)
      {
      }

JianpuBeam::JianpuBeam(const JianpuBeam& b)
   : Beam(b)
      {
      }

JianpuBeam::~JianpuBeam()
      {
      }

void JianpuBeam::layout()
      {
      // Always put the horizontal beams underneath the chords/rests.

      System* system = _elements.front()->measure()->system();
      setParent(system);

      QPointF pagePosition(pagePos());
      setbbox(QRectF());

      // Get maximum beam level.
      int beamLevels = 0;
      for (const ChordRest* c : _elements)
            beamLevels = qMax(beamLevels, c->durationType().hooks());

      // Set the first beam's y position.
      const ChordRest* cr = _elements.front();
      qreal y = cr->pos().y() - pagePosition.y();
      if (cr->isChord()) {
            const Note* note = toChord(cr)->notes().at(0);
            y = note->pos().y() + note->bbox().height();
            const JianpuNote* jn = dynamic_cast<const JianpuNote*>(note);
            if (jn && jn->noteOctave() >= 0) {
                  // Note's bounding box does not include space for lower-octave dots.
                  // Add octave-dot box y-offset to align with beams of other notes.
                  y += JianpuNote::OCTAVE_DOTBOX_Y_OFFSET + JianpuNote::OCTAVE_DOTBOX_HEIGHT;
                  }
            }
      else if (cr->isRest()){
            const Rest* rest = toRest(cr);
            y = rest->pos().y() + rest->bbox().height();
            // Rest's bounding box does not include space for lower-octave dots.
            // Add octave-dot box y-offset to align with beams of other notes.
            y += JianpuNote::OCTAVE_DOTBOX_Y_OFFSET + JianpuNote::OCTAVE_DOTBOX_HEIGHT;
            }
      qreal beamDistance = JianpuNote::BEAM_HEIGHT + JianpuNote::BEAM_Y_SPACE;

      // Create beam segments.
      int n = _elements.size();
      int c1 = 0;
      qreal x1 = 0;
      int c2 = 0;
      qreal x2 = 0;
      for (int beamLevel = 0; beamLevel < beamLevels; ++beamLevel) {
            // Loop through the different groups for this beam level.
            // Inner loop will advance through chordrests within each group.
            int i = 0;
            while ( i < n) {
                  ChordRest* cr1 = _elements[i];
                  int l1 = cr1->durationType().hooks() - 1;
                  if (l1 < beamLevel) {
                        i++;
                        continue;
                        }

                  // Found beginning of a group.
                  // Loop through chordrests looking for end of the group.
                  c1 = i;
                  int j = i + 1;
                  while (j < n) {
                        ChordRest* cr2 = _elements[j];
                        int l2 = cr2->durationType().hooks() - 1;
                        if (l2 < beamLevel) {
                              break;
                              }
                        j++;
                        }

                  // Found the beam-level discontinuity; chordrest at j-1 is end of the group.
                  c2 = j - 1;
                  // Calculate x-values of beam segment.
                  cr1 = _elements[c1];
                  x1 = cr1->pos().x() + cr1->pageX() - pagePosition.x();
                  ChordRest* cr2 = _elements[c2];
                  x2 = cr2->pos().x() + cr2->pageX() - pagePosition.x() + cr2->bbox().width();
                  // Add beam segment.
                  beamSegments.push_back(new QLineF(x1, y, x2, y));
                  // Update bounding box.
                  addbbox(QRectF(x1, y, x2 - x1, beamDistance));

                  // Update i index with last scanned location and loop again for more groups.
                  i = j;
                  }

            // Update y value for beams at next beam level.
            y += beamDistance;
            }
      }

void JianpuBeam::draw(QPainter* painter) const
      {
      if (beamSegments.empty())
            return;

      // Draw beam(s) underneath the note/rest numbers.
      QBrush brush(curColor(), Qt::SolidPattern);
      painter->setBrush(brush);
      painter->setPen(Qt::NoPen);
      qreal height = JianpuNote::BEAM_HEIGHT;
      for (const QLineF* line : beamSegments) {
            QRectF beam(line->x1(), line->y1(), line->length(), height);
            painter->fillRect(beam, brush);
            }
      }

} // namespace Ms
