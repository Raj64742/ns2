/*
 * (c) 1997-98 StarBurst Communications Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Christoph Haenle, chris@cs.vu.nl
 * File: mftp.cc
 * Last change: Dec 07, 1998
 *
 * This software may freely be used only for non-commercial purposes
 *
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/mftp.cc,v 1.3 2000/09/01 03:04:06 haoboy Exp $
 */

// This file contains functionality common to both MFTP sender and receivers.

#include "mftp.h"

MFTPAgent::MFTPAgent() :
    Agent(PT_MFTP), FileSize(0), FileDGrams(0),
    dtu_size(0), dtus_per_block(0), dtus_per_group(0),
    nb_groups(0)
{
    bind("dtuSize_", &dtuSize_);
    bind("fileSize_", &fileSize_);
    bind("dtusPerBlock_", &dtusPerBlock_);
    bind("dtusPerGroup_", &dtusPerGroup_);
    bind("seekCount_", &seekCount_);
};

int MFTPAgent::init()
{
    if(dtusPerBlock_ % 8 != 0) {
        Tcl& tcl = Tcl::instance();
        tcl.resultf("%s: dtusPerBlock_ must be a multiple of 8", name_);
        return TCL_ERROR;
    }
    dtu_size = dtuSize_;
    FileSize = fileSize_;
    dtus_per_block = dtusPerBlock_;
    dtus_per_group = dtusPerGroup_;
    seekCount_ = 0;

    FileDGrams = (FileSize + dtu_size - 1) / dtu_size;
    nb_groups = (FileDGrams + dtus_per_group - 1) / dtus_per_group;

    return TCL_OK;
}
