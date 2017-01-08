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

/**
 \file
 Implementation of JianpuNote class.
*/

#include <assert.h>

#include "jianpunote.h"
#include "score.h"
#include "key.h"
#include "chord.h"
#include "sym.h"
#include "xml.h"
#include "slur.h"
#include "tie.h"
#include "text.h"
#include "clef.h"
#include "staff.h"
#include "pitchspelling.h"
#include "arpeggio.h"
#include "tremolo.h"
#include "utils.h"
#include "image.h"
#include "system.h"
#include "tuplet.h"
#include "articulation.h"
#include "drumset.h"
#include "segment.h"
#include "measure.h"
#include "undo.h"
#include "part.h"
#include "stafftype.h"
#include "stringdata.h"
#include "fret.h"
#include "harmony.h"
#include "fingering.h"
#include "bend.h"
#include "accidental.h"
#include "page.h"
#include "icon.h"
#include "notedot.h"
#include "spanner.h"
#include "glissando.h"
#include "bagpembell.h"
#include "hairpin.h"
#include "textline.h"
#include "durationtype.h"

namespace Ms {

// Ratio used to reduce height of font bounding-box returned by QFontMetricsF.
const float JianpuNote::FONT_BBOX_HEIGHT_RATIO = 0.7f;

JianpuNote::JianpuNote(Score* score)
   : Note(score)
      {
      initialize();
      }

JianpuNote::JianpuNote(const Note& note, bool link)
   : Note(note, link)
      {
      initialize();
      }

JianpuNote::JianpuNote(const JianpuNote& note, bool link)
   : Note(note, link)
      {
      _noteNumber = note._noteNumber;
      _noteOctave = note._noteOctave;
      _durationDashCount = note._durationDashCount;
      _octaveDotBox = note._octaveDotBox;
      }

JianpuNote::~JianpuNote()
      {
      }

void JianpuNote::initialize()
      {
      _noteNumber = 0;
      _noteOctave = 0;
      _durationDashCount = 0;
      _noteNumberBox.setRect(0, 0, 0, 0);
      _octaveDotBox.setRect(0, 0, 0, 0);
      }

void JianpuNote::setTpc(int v)
      {
      Note::setTpc(v);
      setNoteByPitch(pitch(), tpc(), chord()->durationType().type());
      }

void JianpuNote::setPitch(int val)
      {
      assert(0 <= val && val <= 127);
      if (pitch() != val) {
            Note::setPitch(val);
            setNoteByPitch(pitch(), tpc(), chord()->durationType().type());
            }
      }

void JianpuNote::layout()
      {
      // Lay out note-number and octave-dot boxes.
      // Always anchor note-number box to (0, 0) position so that we can have
      // both rest and note numbers on the same level regardless of octave dots.

      // Update note number.
      setNoteByPitch(pitch(), tpc(), chord()->durationType().type());

      // Get note font metrics.
      StaffType* st = staff()->staffType();
      QFontMetricsF fm(st->jianpuNoteFont(), MScore::paintDevice());
      QString txt = QString::number(_noteNumber);
      QRectF rect = fm.tightBoundingRect(txt);
      // Font bounding rectangle height is too large; make it smaller.
      _noteNumberBox.setRect(0, 0, rect.width(), rect.height() * FONT_BBOX_HEIGHT_RATIO);

      // Lay out octave-dot box.
      if (_noteOctave < 0) {
            // Lower octave.
            _octaveDotBox.setRect(0, _noteNumberBox.y() + _noteNumberBox.height() + OCTAVE_DOTBOX_Y_OFFSET,
                                  _noteNumberBox.width(), OCTAVE_DOTBOX_HEIGHT);
            }
      else if (_noteOctave > 0) {
            // Upper octave.
            _octaveDotBox.setRect(0, _noteNumberBox.y() - OCTAVE_DOTBOX_HEIGHT - OCTAVE_DOTBOX_Y_OFFSET,
                                  _noteNumberBox.width(), OCTAVE_DOTBOX_HEIGHT);
            }
      else {
            // No octave.
            _octaveDotBox.setRect(0, 0, 0, 0);
            }

      // Update main bounding box with union of two boxes.
      setbbox(_noteNumberBox | _octaveDotBox);
      }

//---------------------------------------------------------
//   layout2
//    called after final position of note is set
//---------------------------------------------------------
void JianpuNote::layout2()
      {
      // TODO: Currently this is a copy from Note. Re-examine the code and optimize it.

      int dots = chord()->dots();
      if (dots) {
            qreal d  = score()->point(score()->styleS(StyleIdx::dotNoteDistance)) * mag();
            qreal dd = score()->point(score()->styleS(StyleIdx::dotDotDistance)) * mag();
            qreal x  = chord()->dotPosX() - pos().x() - chord()->pos().x();
            // apply to dots
            qreal xx = x + d;
            for (NoteDot* dot : _dots) {
                  dot->rxpos() = xx;
                  dot->adjustReadPos();
                  xx += dd;
                  }
            }

      // layout elements attached to note
      for (Element* e : _el) {
            if (!score()->tagIsValid(e->tag()))
                  continue;
            e->setMag(mag());
            if (e->isSymbol()) {
                  qreal w = headWidth();
                  Symbol* sym = toSymbol(e);
                  QPointF rp = e->readPos();
                  e->layout();
                  if (sym->sym() == SymId::noteheadParenthesisRight) {
                        if (staff()->isTabStaff()) {
                              StaffType* tab = staff()->staffType();
                              w = tabHeadWidth(tab);
                              }
                        e->rxpos() += w;
                        }
                  else if (sym->sym() == SymId::noteheadParenthesisLeft) {
                        e->rxpos() -= symWidth(SymId::noteheadParenthesisLeft);
                        }
                  if (sym->sym() == SymId::noteheadParenthesisLeft || sym->sym() == SymId::noteheadParenthesisRight) {
                        // adjustReadPos() was called too early in layout(), adjust:
                        if (!rp.isNull()) {
                              e->setUserOff(QPointF());
                              e->setReadPos(rp);
                              e->adjustReadPos();
                              }
                        }
                  }
            else
                  e->layout();
            }
      }

void JianpuNote::draw(QPainter* painter) const
      {
      if (hidden())
            return;

      // Draw note number.
      QString txt = QString::number(_noteNumber);
      StaffType* st = staff()->staffType();
      QFont f(st->jianpuNoteFont());
      f.setPointSizeF(f.pointSizeF() * MScore::pixelRatio);
      painter->setFont(f);
      painter->setPen(QColor(curColor()));
      // We take bounding box y-position as top of the note number.
      // But function "drawText" takes y-position as baseline of the font, which is the bottom of note number.
      // So adjust y-position for "drawText" to the bottom of the bounding box.
      painter->drawText(QPointF(pos().x() + _noteNumberBox.x(),
                                pos().y() + _noteNumberBox.y() + _noteNumberBox.height()), txt);

      // Prepare paint brush for octave dots and duration dash drawing.
      QBrush brush(curColor(), Qt::SolidPattern);
      painter->setBrush(brush);
      painter->setPen(Qt::NoPen);

      // Draw octave dots.
      // TODO: Currently we draw only one dot. Draw as many dots as the octave number indicates.
      if (_noteOctave < 0) {
            // Lower octave.
            // Draw octave dot in the middle.
            qreal xOffset = (_octaveDotBox.width() - OCTAVE_DOT_WIDTH) * 0.5;
            QRectF rect(pos().x() + _octaveDotBox.x() + xOffset,
                        pos().y() + _octaveDotBox.y(),
                        OCTAVE_DOT_WIDTH, OCTAVE_DOT_HEIGHT);
            painter->drawEllipse(rect);
            }
      else if (_noteOctave > 0) {
            // Upper octave.
            qreal xOffset = (_octaveDotBox.width() - OCTAVE_DOT_WIDTH) * 0.5;
            QRectF rect(pos().x() + _octaveDotBox.x() + xOffset,
                        pos().y() + _octaveDotBox.y(),
                        OCTAVE_DOT_WIDTH, OCTAVE_DOT_HEIGHT);
            painter->drawEllipse(rect);
            }

      // Draw duration dashes for whole and half notes.
      // Draw dashes only for the base-note of the chord.
      if (_durationDashCount > 0 && this == chord()->downNote()) {
            // TODO: calculate dash/space widths based on available space of the measure.
            qreal space = DURATION_DASH_X_SPACE;
            qreal width = DURATION_DASH_WIDTH;
            qreal height = DURATION_DASH_HEIGHT;
            qreal x = pos().x() + bbox().width() + space;
            qreal y = JianpuNote::NOTE_BASELINE * spatium() * 0.5;
            y += _noteNumberBox.height() * 0.5;
            QRectF dash(x, y, width, height);
            for (int i = 0; i < _durationDashCount; i++) {
                  painter->fillRect(dash, brush);
                  x += width + space ;
                  dash.moveLeft(x); // Move rect's left edge to next dash position.
                  }
            }
      }

void JianpuNote::setNoteByNumber(int number, int octave, TDuration::DurationType duration)
      {
      assert(1 <= number && number <= 7);
      assert(-MAX_OCTAVE_DOTS <= octave && octave <= MAX_OCTAVE_DOTS);

      _noteNumber = number;
      _noteOctave = octave;

      // TODO: Add checking for time signature, i.e., 4/4, 3/4, 2/4, etc.
      if (duration == TDuration::DurationType::V_WHOLE)
            _durationDashCount = 3;
      else if (duration == TDuration::DurationType::V_HALF)
            _durationDashCount = 1;
      else
            _durationDashCount = 0;
      }

void JianpuNote::setNoteByPitch(int pitch, int tpc, TDuration::DurationType duration)
      {
      Segment* seg = chord()->segment();
      int tick = seg ? seg->tick() : 0;
      Key key = staff() ? staff()->key(tick) : Key::C;
      int number = tpc2numberNoteByKey(tpc, key);
      int octave = pitch2octaveByKey(pitch, key);
      setNoteByNumber(number, octave, duration);
      }

// Convert standard note pitch and key to Jianpu octave number:
//     0 for middle octave (octave #4); negative for lower and positive for upper octaves.
int JianpuNote::pitch2octaveByKey(int pitch, Key key)
      {
      static const int keyNotePitch[(int) Key::NUM_OF] =
      //KEY --> C_B, G_B, D_B, A_B, E_B, B_B,   F,   C,   G,   D,   A,   E,   B, F_S, C_S
              {  71,  66,  61,  68,  63,  70,  65,  60,  67,  62,  69,  64,  71,  66,  61 };

      // Get key note pitch based on middle C octave (octave #4).
      int basePitch = keyNotePitch[(int) key - (int) Key::MIN];

      // Calculate octave number.
      int octave = (pitch - basePitch) / PITCH_DELTA_OCTAVE;
      if (pitch < basePitch)
            octave--;

      return octave;
      }

// Convert standard note tpc and key to Jianpu note number (1 to 7).
int JianpuNote::tpc2numberNoteByKey(int tpc, Key key)
      {
      // Standard notes of C Major scale -->              F  C  G  D  A  E  B
      static const int numberNotes[STEP_DELTA_OCTAVE] = { 4, 1, 5, 2, 6, 3, 7 };
      int index = (tpc - Tpc::TPC_MIN) - ((int) key - (int) Key::MIN);
      while (index < 0)
            index += STEP_DELTA_OCTAVE;

      return numberNotes[index % STEP_DELTA_OCTAVE];
      }

} // namespace Ms
