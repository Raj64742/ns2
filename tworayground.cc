/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* tworayground.cc
   $Id: tworayground.cc,v 1.2 1999/01/04 19:59:07 haldar Exp $
 */

#include <math.h>

#include <delay.h>
#include <packet.h>

#include <packet-stamp.h>
#include <antenna.h>
#include <mobilenode.h>
#include <propagation.h>
#include <wireless-phy.h>
#include <tworayground.h>

static class TwoRayGroundClass: public TclClass {
public:
        TwoRayGroundClass() : TclClass("Propagation/TwoRayGround") {}
        TclObject* create(int, const char*const*) {
                return (new TwoRayGround);
        }
} class_tworayground;

TwoRayGround::TwoRayGround()
{
  last_hr = last_ht = 0.0;
  crossover_dist = 0.0;
}

static double 
Friss(double Pt, double Gt, double Gr, double lambda, double L, double d)
{
        /*
         * Friss free space equation:
         *
         *       Pt * Gt * Gr * (lambda^2)
         *   P = --------------------------
         *       (4 *pi * d)^2 * L
         */
  double M = lambda / (4 * PI * d);
  return (Pt * Gt * Gr * (M * M)) / L;
}

static double
TwoRay(double Pt, double Gt, double Gr, double ht, double hr, double d)
{
        /*
         *  Two-ray ground reflection model.
         *
         *	 Pt * Gt * Gr * (ht^2 * hr^2)
         *  Pr = ----------------------------
         *           d^4
         *
         */
  return Pt * Gt * Gr * (hr * hr * ht * ht) / (d * d * d * d);
}

double
TwoRayGround::Pr(PacketStamp *t, PacketStamp *r, double L, double lambda)
{
  double rX, rY, rZ;		// loc of receiver
  double tX, tY, tZ;		// loc of xmitter
  double d;			// dist
  double hr, ht;		// height of recv and xmit antennas
  double Pr;

  r->getNode()->getLoc(&rX, &rY, &rZ);
  t->getNode()->getLoc(&tX, &tY, &tZ);

  rX += r->getAntenna()->getX();
  rY += r->getAntenna()->getY();
  tX += t->getAntenna()->getX();
  tY += t->getAntenna()->getY();

  d = sqrt((rX - tX) * (rX - tX) 
	   + (rY - tY) * (rY - tY) 
	   + (rZ - tZ) * (rZ - tZ));
    
  /* We're going to assume the ground is essentially flat.
     This empirical two ground ray reflection model doesn't make 
     any sense if the ground is not a plane. */

  if (rZ != tZ) {
    printf("%s: TwoRayGround propagation model assume flat ground\n",
	   __FILE__);
  }

  hr = rZ + r->getAntenna()->getZ();
  ht = tZ + t->getAntenna()->getZ();

  if (hr != last_hr || ht != last_ht)
    { // recalc the cross-over distance
      /* 
	         16 * PI^2 * L * hr^2 * ht^2
	 d^2 = ---------------------------------
                      lambda^2
       */
      crossover_dist = sqrt((16 * PI * PI * L * ht * ht * hr * hr)
			    / (lambda * lambda));
      last_hr = hr; last_ht = ht;
#if DEBUG > 3
      printf("TRG: xover %e.10 hr %f ht %f\n",
	      crossover_dist, hr, ht);
#endif
    }

  /*
   *  If the transmitter is within the cross-over range , use the
   *  Friss equation.  Otherwise, use the two-ray
   *  ground reflection model.
   */

  double Gt = t->getAntenna()->getTxGain(rX - tX, rY - tY, rZ - tZ, 
					 t->getLambda());
  double Gr = r->getAntenna()->getRxGain(tX - rX, tY - rY, tZ - rZ,
					 r->getLambda());

#if DEBUG > 3
  printf("TRG %.9f %d(%d,%d)@%d(%d,%d) d=%f xo=%f :",
	 Scheduler::instance().clock(), 
	 t->getNode()->index(), (int)tX, (int)tY,
	 r->getNode()->index(), (int)rX, (int)rY,
	 d, crossover_dist);
  //  printf("\n\t Pt %e Gt %e Gr %e lambda %e L %e :",
  //         t->getTxPr(), Gt, Gr, lambda, L);
#endif

  if(d <= crossover_dist) {
    Pr = Friss(t->getTxPr(), Gt, Gr, lambda, L, d);
#if DEBUG > 3
    printf("Friss %e\n",Pr);
#endif
    return Pr;
  }
  else {
    Pr = TwoRay(t->getTxPr(), Gt, Gr, ht, hr, d);
#if DEBUG > 3
    printf("TwoRay %e\n",Pr);
#endif    
    return Pr;
  }
}
