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
 Implementation of class JianpuChord
*/

#include "jianpuchord.h"
#include "jianpunote.h"
#include "jianpuhook.h"
#include "xml.h"
#include "style.h"
#include "segment.h"
#include "text.h"
#include "measure.h"
#include "system.h"
#include "tuplet.h"
#include "tie.h"
#include "arpeggio.h"
#include "score.h"
#include "tremolo.h"
#include "glissando.h"
#include "staff.h"
#include "part.h"
#include "utils.h"
#include "articulation.h"
#include "undo.h"
#include "chordline.h"
#include "lyrics.h"
#include "navigate.h"
#include "stafftype.h"
#include "stem.h"
#include "mscore.h"
#include "accidental.h"
#include "noteevent.h"
#include "pitchspelling.h"
#include "stemslash.h"
#include "ledgerline.h"
#include "drumset.h"
#include "key.h"
#include "sym.h"
#include "stringdata.h"
#include "beam.h"
#include "slur.h"
#include "stafffactory.h"


namespace Ms {

JianpuChord::JianpuChord(Score* s)
   : Chord(s)
      {
      }

JianpuChord::JianpuChord(const Chord& c, bool link, StaffFactory* fac)
   : Chord(c, link, fac)
      {
      }

JianpuChord::JianpuChord(const JianpuChord& c, bool link, StaffFactory* fac)
   : Chord(c, link, fac)
      {
      }

JianpuChord::~JianpuChord()
      {
      }

void JianpuChord::layout()
      {
      // TODO: Currently this is a copy from Chord with some tweaks for Jianpu.
      //       Re-examine the code and optimize it.

      if (_notes.empty())
            return;

      int gi = 0;
      for (Chord* c : _graceNotes) {
            // HACK: graceIndex is not well-maintained on add & remove
            // so rebuild now
            c->setGraceIndex(gi++);
            if (c->isGraceBefore())
                  c->layout();
            }
      QVector<Chord*> graceNotesBefore = Chord::graceNotesBefore();
      int gnb = graceNotesBefore.size();

      // lay out grace notes after separately so they are processed left to right
      // (they are normally stored right to left)

      QVector<Chord*> gna = graceNotesAfter();
      for (Chord* c : gna)
            c->layout();

      qreal _spatium         = spatium();
      qreal _mag             = staff() ? staff()->mag() : 1.0;    // palette elements do not have a staff
      qreal dotNoteDistance  = score()->styleP(StyleIdx::dotNoteDistance)  * _mag;
      qreal minNoteDistance  = score()->styleP(StyleIdx::minNoteDistance)  * _mag;
      qreal minTieLength     = score()->styleP(StyleIdx::MinTieLength)     * _mag;

      qreal graceMag         = score()->styleD(StyleIdx::graceNoteMag);
      qreal chordX           = (_noteType == NoteType::NORMAL) ? ipos().x() : 0.0;

      qreal lll    = 0.0;         // space to leave at left of chord
      qreal rrr    = 0.0;         // space to leave at right of chord
      qreal lhead  = 0.0;         // amount of notehead to left of chord origin
      Note* upnote = upNote();

      //-----------------------------------------
      //  process notes
      //-----------------------------------------

      int noteCount = _notes.size();
      for (int i = 0; i < noteCount; i++) {
            Note* note = _notes.at(i);
            note->layout();

            // Calculate and set note's position in the chord.
            // Jianpu bar-line span: -4 to +4
            qreal x = 0.0;
            qreal y = JianpuNote::NOTE_BASELINE * spatium() * 0.5;
            for (int j = 1; j <= i; j++)
                  y -= _notes.at(j)->height();
            note->setPos(x, y);

            qreal x1 = note->pos().x() + chordX;
            qreal x2 = x1 + note->width();
            lll      = qMax(lll, -x1);
            rrr      = qMax(rrr, x2);
            // track amount of space due to notehead only
            lhead    = qMax(lhead, -x1);

            Accidental* accidental = note->accidental();
            if (accidental && !note->fixed()) {
                  // convert x position of accidental to segment coordinate system
                  qreal x = accidental->pos().x() + note->pos().x() + chordX;
                  // distance from accidental to note already taken into account
                  // but here perhaps we create more padding in *front* of accidental?
                  x -= score()->styleP(StyleIdx::accidentalDistance) * _mag;
                  lll = qMax(lll, -x);
                  }

            // allow extra space for shortened ties
            // this code must be kept synchronized
            // with the tie positioning code in Tie::slurPos()
            // but the allocation of space needs to be performed here
            Tie* tie;
            tie = note->tieBack();
            if (tie) {
                  tie->calculateDirection();
                  qreal overlap = 0.0;
                  bool shortStart = false;
                  Note* sn = tie->startNote();
                  Chord* sc = sn->chord();
                  if (sc && sc->measure() == measure() && sc == prevChordRest(this)) {
                        if (sc->notes().size() > 1 || (sc->stem() && sc->up() == tie->up())) {
                              shortStart = true;
                              if (sc->width() > sn->width()) {
                                    // chord with second?
                                    // account for noteheads further to right
                                    qreal snEnd = sn->x() + sn->width();
                                    qreal scEnd = snEnd;
                                    for (unsigned i = 0; i < sc->notes().size(); ++i)
                                          scEnd = qMax(scEnd, sc->notes().at(i)->x() + sc->notes().at(i)->width());
                                    overlap += scEnd - snEnd;
                                    }
                              else
                                    overlap -= sn->width() * 0.12;
                              }
                        else
                              overlap += sn->width() * 0.35;
                        if (notes().size() > 1 || (stem() && !up() && !tie->up())) {
                              // for positive offset:
                              //    use available space
                              // for negative x offset:
                              //    space is allocated elsewhere, so don't re-allocate here
                              if (note->ipos().x() != 0.0)
                                    overlap += qAbs(note->ipos().x());
                              else
                                    overlap -= note->width() * 0.12;
                              }
                        else {
                              if (shortStart)
                                    overlap += note->width() * 0.15;
                              else
                                    overlap += note->width() * 0.35;
                              }
                        qreal d = qMax(minTieLength - overlap, 0.0);
                        lll = qMax(lll, d);
                        }
                  }
            }

      if (_arpeggio) {
            qreal arpeggioDistance = score()->styleP(StyleIdx::ArpeggioNoteDistance) * _mag;
            _arpeggio->layout();    // only for width() !
            lll        += _arpeggio->width() + arpeggioDistance + chordX;
            qreal y1   = upnote->pos().y() - upnote->headHeight() * .5;
            _arpeggio->setPos(-lll, y1);
            _arpeggio->adjustReadPos();
            }

      // allocate enough room for glissandi
      if (_endsGlissando) {
            if (rtick()                                     // if not at beginning of measure
                        || graceNotesBefore.size() > 0)     // or there are graces before
                  lll += _spatium * 0.5 + minTieLength;
            // special case of system-initial glissando final note is handled in Glissando::layout() itself
            }

      if (dots()) {
            qreal x = dotPosX() + dotNoteDistance
               + (dots()-1) * score()->styleP(StyleIdx::dotDotDistance) * _mag;
            x += symWidth(SymId::augmentationDot);
            rrr = qMax(rrr, x);
            }

      if (_hook) {
            if (beam())
                  score()->undoRemoveElement(_hook);
            else {
                  _hook->layout();
                  }
            }

      _spaceLw = lll;
      _spaceRw = rrr;

      if (gnb){
              qreal xl = -(_spaceLw + minNoteDistance) - chordX;
              for (int i = gnb-1; i >= 0; --i) {
                    Chord* g = graceNotesBefore.value(i);
                    xl -= g->_spaceRw/* * 1.2*/;
                    g->setPos(xl, 0);
                    xl -= g->_spaceLw + minNoteDistance * graceMag;
                    }
              if (-xl > _spaceLw)
                    _spaceLw = -xl;
              }
       if (!gna.empty()) {
            qreal xr = _spaceRw;
            int n = gna.size();
            for (int i = 0; i <= n - 1; i++) {
                  Chord* g = gna.value(i);
                  xr += g->_spaceLw + g->_spaceRw + minNoteDistance * graceMag;
                  }
           if (xr > _spaceRw)
                 _spaceRw = xr;
           }

      for (Element* e : _el) {
            if (e->type() == Element::Type::SLUR)     // we cannot at this time as chordpositions are not fixed
                  continue;
            e->layout();
            if (e->type() == Element::Type::CHORDLINE) {
                  QRectF tbbox = e->bbox().translated(e->pos());
                  qreal lx = tbbox.left() + chordX;
                  qreal rx = tbbox.right() + chordX;
                  if (-lx > _spaceLw)
                        _spaceLw = -lx;
                  if (rx > _spaceRw)
                        _spaceRw = rx;
                  }
            }

      for (Note* note : _notes)
            note->layout2();

      QRectF bb;
      processSiblings([&bb] (Element* e) { bb |= e->bbox().translated(e->pos()); } );
      setbbox(bb.translated(_spatium*2, 0));
      }

//---------------------------------------------------------
//   layout2
//    Called after horizontal positions of all elements
//    are fixed.
//---------------------------------------------------------
void JianpuChord::layout2()
      {
#if 0
      // TODO: Currently this is a copy from Chord with some tweaks for Jianpu.
      //       Re-examine the code and optimize it.

      for (Chord* c : _graceNotes)
            c->layout2();

      qreal _spatium = spatium();
      qreal _mag = staff()->mag();

      //
      // Experimental:
      //    look for colliding ledger lines
      //

      const qreal minDist = _spatium * .17;

      Segment* s = segment()->prev(Segment::Type::ChordRest);
      if (s) {
            int strack = staff2track(staffIdx());
            int etrack = strack + VOICES;

            for (LedgerLine* h = _ledgerLines; h; h = h->next()) {
                  qreal len = h->len();
                  qreal y   = h->y();
                  qreal x   = h->x();
                  bool found = false;
                  qreal cx  = h->measureXPos();

                  for (int track = strack; track < etrack; ++track) {
                        Chord* e = static_cast<Chord*>(s->element(track));
                        if (!e || e->type() != Element::Type::CHORD)
                              continue;
                        for (LedgerLine* ll = e->ledgerLines(); ll; ll = ll->next()) {
                              if (ll->y() != y)
                                    continue;

                              qreal d = cx - ll->measureXPos() - ll->len();
                              if (d < minDist) {
                                    //
                                    // the ledger lines overlap
                                    //
                                    qreal shorten = (minDist - d) * .5;
                                    x   += shorten;
                                    len -= shorten;
                                    ll->setLen(ll->len() - shorten);
                                    h->setLen(len);
                                    h->setPos(x, y);
                                    }
                              found = true;
                              break;
                              }
                        if (found)
                              break;
                        }
                  }
            }

      //
      // position after-chord grace notes
      // room for them has been reserved in JianpuChord::layout()
      //

      QVector<Chord*> gna = graceNotesAfter();
      if (!gna.empty()) {
            qreal minNoteDist = score()->styleP(StyleIdx::minNoteDistance) * _mag * score()->styleD(StyleIdx::graceNoteMag);
            // position grace notes from the rightmost to the leftmost
            // get segment (of whatever type) at the end of this chord; if none, get measure last segment
            Segment* s = measure()->tick2segment(segment()->tick() + actualTicks(), Segment::Type::All);
            if (s == nullptr)
                  s = measure()->last();
            if (s == segment())           // if our segment is the last, no adjacent segment found
                  s = nullptr;
            // start from the right (if next segment found, x of it relative to this chord;
            // chord right space otherwise)
            qreal xOff =  s ? s->pos().x() - (segment()->pos().x() + pos().x()) : _spaceRw;
            // final distance: if near to another chord, leave minNoteDist at right of last grace
            // else leave note-to-barline distance;
            xOff -= (s != nullptr && s->segmentType() != Segment::Type::ChordRest)
                  ? score()->styleP(StyleIdx::noteBarDistance) * _mag
                  : minNoteDist;
            // scan grace note list from the end
            int n = gna.size();
            for (int i = n-1; i >= 0; i--) {
                  Chord* g = gna.value(i);
                  xOff -= g->_spaceRw;                  // move to left by grace note left space (incl. grace own width)
                  g->rxpos() = xOff;
                  xOff -= minNoteDist + g->_spaceLw;    // move to left by grace note right space and inter-grace distance
                  }
            }
#endif
      }

} // namespace Ms
