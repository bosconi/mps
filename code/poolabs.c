/* poolabs.c: ABSTRACT POOL CLASSES
 *
 * $Id$
 * Copyright (c) 2001-2015 Ravenbrook Limited.  See end of file for license.
 * Portions copyright (C) 2002 Global Graphics Software.
 *
 * PURPOSE
 *
 * .purpose: This defines the abstract pool classes, giving
 * a single-inheritance framework which concrete classes
 * may utilize.  The purpose is to reduce the fragility of class
 * definitions for pool implementations when small changes are
 * made to the pool protocol.   For now, the class hierarchy for
 * the abstract classes is intended to be useful, but not to
 * represent any particular design for pool inheritance.
 *
 * HIERARCHY
 *
 * .hierarchy: define the following hierarchy of abstract pool classes:
 *    AbstractPoolClass     - implements init, finish, describe
 *     AbstractBufferPoolClass - implements the buffer protocol
 *      AbstractSegBufPoolClass - uses SegBuf buffer class
 *       AbstractScanPoolClass - implements basic scanning
 *        AbstractCollectPoolClass - implements basic GC
 */

#include "mpm.h"

SRCID(poolabs, "$Id$");


typedef PoolClassStruct AbstractPoolClassStruct;
typedef PoolClassStruct AbstractBufferPoolClassStruct;
typedef PoolClassStruct AbstractSegBufPoolClassStruct;
typedef PoolClassStruct AbstractScanPoolClassStruct;
typedef PoolClassStruct AbstractCollectPoolClassStruct;


/* Mixins:
 *
 * For now (at least) we're avoiding multiple inheritance.
 * However, there is a significant use of multiple inheritance
 * in practice amongst the pool classes, as there are several
 * orthogonal sub-protocols included in the pool protocol.
 * The following mixin functions help to provide the inheritance
 * via a simpler means than real multiple inheritance.
 */


/* PoolClassMixInBuffer -- mix in the protocol for buffer reserve / commit */

void PoolClassMixInBuffer(PoolClass class)
{
  /* Can't check class because it's not initialized yet */
  class->bufferFill = PoolTrivBufferFill;
  class->bufferEmpty = PoolTrivBufferEmpty;
  /* By default, buffered pools treat frame operations as NOOPs */
  class->framePush = PoolTrivFramePush;
  class->framePop = PoolTrivFramePop;
  class->framePopPending = PoolTrivFramePopPending;
  class->bufferClass = BufferClassGet;
}


/* PoolClassMixInScan -- mix in the protocol for scanning */

void PoolClassMixInScan(PoolClass class)
{
  /* Can't check class because it's not initialized yet */
  class->access = PoolSegAccess;
  class->blacken = PoolTrivBlacken;
  class->grey = PoolTrivGrey;
  /* scan is part of the scanning protocol, but there is no useful
     default method */
  class->scan = PoolNoScan;
}


/* PoolClassMixInFormat -- mix in the protocol for formatted pools */

void PoolClassMixInFormat(PoolClass class)
{
  /* Can't check class because it's not initialized yet */
  class->attr |= AttrFMT;
  /* walk is part of the format protocol, but there is no useful
     default method */
  class->walk = PoolNoWalk;
}


/* PoolClassMixInCollect -- mix in the protocol for GC */

void PoolClassMixInCollect(PoolClass class)
{
  /* Can't check class because it's not initialized yet */
  class->attr |= AttrGC;
  class->whiten = PoolTrivWhiten;
  /* fix, fixEmergency and reclaim are part of the collection
     protocol, but there are no useful default methods for them */
  class->fix = PoolNoFix;
  class->fixEmergency = PoolNoFix;
  class->reclaim = PoolNoReclaim;
  class->rampBegin = PoolTrivRampBegin;
  class->rampEnd = PoolTrivRampEnd;
}


/* Classes */


/* PoolAbsInit -- initialize an abstract pool instance */

Res PoolAbsInit(Pool pool, Arena arena, PoolClass class, ArgList args)
{
  AVER(pool != NULL);
  AVERT(Arena, arena);
  UNUSED(args);
  UNUSED(class); /* used for debug pools only */
  
  /* Superclass init */
  InstInit(CouldBeA(Inst, pool));

  pool->arena = arena;
  RingInit(&pool->arenaRing);
  RingInit(&pool->bufferRing);
  RingInit(&pool->segRing);
  pool->bufferSerial = (Serial)0;
  pool->alignment = MPS_PF_ALIGN;
  pool->format = NULL;
  pool->fix = PoolNoFix;

  pool->serial = ArenaGlobals(arena)->poolSerial;
  ++ArenaGlobals(arena)->poolSerial;

  /* Initialise signature last; see <design/sig/> */
  SetClassOfPoly(pool, CLASS(AbstractPool));
  pool->sig = PoolSig;
  AVERT(Pool, pool);

  /* Add initialized pool to list of pools in arena. */
  RingAppend(ArenaPoolRing(arena), PoolArenaRing(pool));

  return ResOK;
}


/* PoolAbsFinish -- finish an abstract pool instance */

void PoolAbsFinish(Pool pool)
{
  /* Detach the pool from the arena and format, and unsig it. */
  RingRemove(PoolArenaRing(pool));

  /* FIXME: Should be done in finish of pools that use formats */
  if (pool->format) {
    AVER(pool->format->poolCount > 0);
    --pool->format->poolCount;
  }

  pool->sig = SigInvalid;
  InstFinish(CouldBeA(Inst, pool));
 
  RingFinish(&pool->segRing);
  RingFinish(&pool->bufferRing);
  RingFinish(&pool->arenaRing);
 
  EVENT1(PoolFinish, pool);
}

DEFINE_CLASS(Inst, PoolClass, class)
{
  INHERIT_CLASS(class, PoolClass, InstClass);
}

DEFINE_CLASS(Pool, AbstractPool, class)
{
  INHERIT_CLASS(&class->protocol, AbstractPool, Inst);
  class->size = sizeof(PoolStruct);
  class->attr = 0;
  class->varargs = ArgTrivVarargs;
  class->init = PoolAbsInit;
  class->finish = PoolAbsFinish;
  class->alloc = PoolNoAlloc;
  class->free = PoolNoFree;
  class->bufferFill = PoolNoBufferFill;
  class->bufferEmpty = PoolNoBufferEmpty;
  class->access = PoolNoAccess;
  class->whiten = PoolNoWhiten;
  class->grey = PoolNoGrey;
  class->blacken = PoolNoBlacken;
  class->scan = PoolNoScan;
  class->fix = PoolNoFix;
  class->fixEmergency = PoolNoFix;
  class->reclaim = PoolNoReclaim;
  class->traceEnd = PoolTrivTraceEnd;
  class->rampBegin = PoolNoRampBegin;
  class->rampEnd = PoolNoRampEnd;
  class->framePush = PoolNoFramePush;
  class->framePop = PoolNoFramePop;
  class->framePopPending = PoolNoFramePopPending;
  class->addrObject = PoolNoAddrObject;
  class->walk = PoolNoWalk;
  class->freewalk = PoolTrivFreeWalk;
  class->bufferClass = PoolNoBufferClass;
  class->describe = PoolTrivDescribe;
  class->debugMixin = PoolNoDebugMixin;
  class->totalSize = PoolNoSize;
  class->freeSize = PoolNoSize;
  class->sig = PoolClassSig;

  /* FIXME: This was moved from PoolInit, but seems odd.  Should be
     done for all classes?  Classes get called before MPMInit.  Should
     they call MPMInit? */
  /* label the pool class with its name */
  /* We could still get multiple labelling if multiple instances of */
  /* the pool class get created simultaneously, but it's not worth */
  /* putting another lock in the code. */
  {
    Word classId;
    EventInit();
    classId = EventInternString(ClassName(class));
    /* NOTE: this breaks <design/type/#addr.use> */
    EventLabelAddr((Addr)class, classId);
  }
}

DEFINE_CLASS(Pool, AbstractBufferPool, class)
{
  INHERIT_CLASS(class, AbstractBufferPool, AbstractPool);
  PoolClassMixInBuffer(class);
}

DEFINE_CLASS(Pool, AbstractSegBufPool, class)
{
  INHERIT_CLASS(class, AbstractSegBufPool, AbstractBufferPool);
  class->bufferClass = SegBufClassGet;
}

DEFINE_CLASS(Pool, AbstractScanPool, class)
{
  INHERIT_CLASS(class, AbstractScanPool, AbstractSegBufPool);
  PoolClassMixInScan(class);
}

DEFINE_CLASS(Pool, AbstractCollectPool, class)
{
  INHERIT_CLASS(class, AbstractCollectPool, AbstractScanPool);
  PoolClassMixInCollect(class);
}


/* PoolNo*, PoolTriv* -- Trivial and non-methods for Pool Classes
 *
 * See <design/pool/#no> and <design/pool/#triv>
 */

Res PoolNoAlloc(Addr *pReturn, Pool pool, Size size)
{
  AVER(pReturn != NULL);
  AVERT(Pool, pool);
  AVER(size > 0);
  NOTREACHED;
  return ResUNIMPL;
}

Res PoolTrivAlloc(Addr *pReturn, Pool pool, Size size)
{
  AVER(pReturn != NULL);
  AVERT(Pool, pool);
  AVER(size > 0);
  return ResLIMIT;
}

void PoolNoFree(Pool pool, Addr old, Size size)
{
  AVERT(Pool, pool);
  AVER(old != NULL);
  AVER(size > 0);
  NOTREACHED;
}

void PoolTrivFree(Pool pool, Addr old, Size size)
{
  AVERT(Pool, pool);
  AVER(old != NULL);
  AVER(size > 0);
  NOOP;                         /* trivial free has no effect */
}


Res PoolNoBufferFill(Addr *baseReturn, Addr *limitReturn,
                     Pool pool, Buffer buffer, Size size)
{
  AVER(baseReturn != NULL);
  AVER(limitReturn != NULL);
  AVERT(Pool, pool);
  AVERT(Buffer, buffer);
  AVER(size > 0);
  NOTREACHED;
  return ResUNIMPL;
}

Res PoolTrivBufferFill(Addr *baseReturn, Addr *limitReturn,
                       Pool pool, Buffer buffer, Size size)
{
  Res res;
  Addr p;

  AVER(baseReturn != NULL);
  AVER(limitReturn != NULL);
  AVERT(Pool, pool);
  AVERT(Buffer, buffer);
  AVER(size > 0);

  res = PoolAlloc(&p, pool, size);
  if (res != ResOK)
    return res;
 
  *baseReturn = p;
  *limitReturn = AddrAdd(p, size);
  return ResOK;
}


void PoolNoBufferEmpty(Pool pool, Buffer buffer,
                       Addr init, Addr limit)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buffer);
  AVER(BufferIsReady(buffer));
  AVER(init <= limit);
  NOTREACHED;
}

void PoolTrivBufferEmpty(Pool pool, Buffer buffer, Addr init, Addr limit)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buffer);
  AVER(BufferIsReady(buffer));
  AVER(init <= limit);
  if (limit > init)
    PoolFree(pool, init, AddrOffset(init, limit));
}


Res PoolTrivDescribe(Pool pool, mps_lib_FILE *stream, Count depth)
{
  AVERT(Pool, pool);
  AVER(stream != NULL);
  return WriteF(stream, depth,
                "No class-specific description available.\n",
                NULL);
}


Res PoolNoTraceBegin(Pool pool, Trace trace)
{
  AVERT(Pool, pool);
  AVERT(Trace, trace);
  AVER(PoolArena(pool) == trace->arena);
  NOTREACHED;
  return ResUNIMPL;
}

Res PoolTrivTraceBegin(Pool pool, Trace trace)
{
  AVERT(Pool, pool);
  AVERT(Trace, trace);
  AVER(PoolArena(pool) == trace->arena);
  return ResOK;
}

/* NoAccess
 *
 * Should be used (for the access method) by Pool Classes which do
 * not expect to ever have pages which the mutator will fault on.
 * That is, no protected pages, or only pages which are inaccessible
 * by the mutator are protected.
 */
Res PoolNoAccess(Pool pool, Seg seg, Addr addr,
                 AccessSet mode, MutatorFaultContext context)
{
  AVERT(Pool, pool);
  AVERT(Seg, seg);
  AVER(SegBase(seg) <= addr);
  AVER(addr < SegLimit(seg));
  AVERT(AccessSet, mode);
  /* can't check context as there is no Check method */
  UNUSED(mode);
  UNUSED(context);

  NOTREACHED;
  return ResUNIMPL;
}


/* SegAccess
 *
 * See also PoolSingleAccess
 *
 * Should be used (for the access method) by Pool Classes which intend
 * to handle page faults by scanning the entire segment and lowering
 * the barrier.
 */
Res PoolSegAccess(Pool pool, Seg seg, Addr addr,
                  AccessSet mode, MutatorFaultContext context)
{
  AVERT(Pool, pool);
  AVERT(Seg, seg);
  AVER(SegBase(seg) <= addr);
  AVER(addr < SegLimit(seg));
  AVER(SegPool(seg) == pool);
  AVERT(AccessSet, mode);
  /* can't check context as there is no Check method */

  UNUSED(addr);
  UNUSED(context);
  TraceSegAccess(PoolArena(pool), seg, mode);
  return ResOK;
}


/* SingleAccess
 *
 * See also ArenaRead, and PoolSegAccess.
 *
 * Handles page faults by attempting emulation.  If the faulting
 * instruction cannot be emulated then this function returns ResFAIL.
 *
 * Due to the assumptions made below, pool classes should only use
 * this function if all words in an object are tagged or traceable.
 *
 * .single-access.assume.ref: It currently assumes that the address
 * being faulted on contains a plain reference or a tagged non-reference.
 * .single-access.improve.format: Later this will be abstracted
 * through the cleint object format interface, so that
 * no such assumption is necessary.
 */
Res PoolSingleAccess(Pool pool, Seg seg, Addr addr,
                     AccessSet mode, MutatorFaultContext context)
{
  Arena arena;

  AVERT(Pool, pool);
  AVERT(Seg, seg);
  AVER(SegBase(seg) <= addr);
  AVER(addr < SegLimit(seg));
  AVER(SegPool(seg) == pool);
  AVERT(AccessSet, mode);
  /* can't check context as there is no Check method */

  arena = PoolArena(pool);

  if(ProtCanStepInstruction(context)) {
    Ref ref;
    Res res;

    ShieldExpose(arena, seg);

    if(mode & SegSM(seg) & AccessREAD) {
      /* Read access. */
      /* .single-access.assume.ref */
      /* .single-access.improve.format */
      ref = *(Ref *)addr;
      /* .tagging: Check that the reference is aligned to a word boundary */
      /* (we assume it is not a reference otherwise). */
      if(WordIsAligned((Word)ref, sizeof(Word))) {
        Rank rank;
        /* See the note in TraceRankForAccess */
        /* (<code/trace.c#scan.conservative>). */
        
        rank = TraceRankForAccess(arena, seg);
        TraceScanSingleRef(arena->flippedTraces, rank, arena,
                           seg, (Ref *)addr);
      }
    }
    res = ProtStepInstruction(context);
    AVER(res == ResOK);

    /* Update SegSummary according to the possibly changed reference. */
    ref = *(Ref *)addr;
    /* .tagging: ought to check the reference for a tag.  But
     * this is conservative. */
    SegSetSummary(seg, RefSetAdd(arena, SegSummary(seg), ref));

    ShieldCover(arena, seg);

    return ResOK;
  } else {
    /* couldn't single-step instruction */
    return ResFAIL;
  }
}


Res PoolTrivWhiten(Pool pool, Trace trace, Seg seg)
{
  AVERT(Pool, pool);
  AVERT(Trace, trace);
  AVERT(Seg, seg);

  SegSetWhite(seg, TraceSetAdd(SegWhite(seg), trace));

  return ResOK;
}

Res PoolNoWhiten(Pool pool, Trace trace, Seg seg)
{
  AVERT(Pool, pool);
  AVERT(Trace, trace);
  AVERT(Seg, seg);
  NOTREACHED;
  return ResUNIMPL;
}


void PoolNoGrey(Pool pool, Trace trace, Seg seg)
{
  AVERT(Pool, pool);
  AVERT(Trace, trace);
  AVERT(Seg, seg);
  NOTREACHED;
}

void PoolTrivGrey(Pool pool, Trace trace, Seg seg)
{
  AVERT(Pool, pool);
  AVERT(Trace, trace);
  AVERT(Seg, seg);

  /* If we had a (partially) white seg, then other parts of the */
  /* same seg might need to get greyed. In fact, all current pools */
  /* only ever Whiten a whole seg, so we never need to Greyen any */
  /* part of an already Whitened seg.  So we hereby exclude white */
  /* segs. */
  /* @@@@ This should not really be called 'trivial'! */
  if(!TraceSetIsMember(SegWhite(seg), trace))
    SegSetGrey(seg, TraceSetSingle(trace));
}


void PoolNoBlacken(Pool pool, TraceSet traceSet, Seg seg)
{
  AVERT(Pool, pool);
  AVERT(TraceSet, traceSet);
  AVERT(Seg, seg);
  NOTREACHED;
}

void PoolTrivBlacken(Pool pool, TraceSet traceSet, Seg seg)
{
  AVERT(Pool, pool);
  AVERT(TraceSet, traceSet);
  AVERT(Seg, seg);

  /* The trivial blacken method does nothing; for pool classes which do */
  /* not keep additional colour information. */
  NOOP;
}


Res PoolNoScan(Bool *totalReturn, ScanState ss, Pool pool, Seg seg)
{
  AVER(totalReturn != NULL);
  AVERT(ScanState, ss);
  AVERT(Pool, pool);
  AVERT(Seg, seg);
  NOTREACHED;
  return ResUNIMPL;
}

Res PoolNoFix(Pool pool, ScanState ss, Seg seg, Ref *refIO)
{
  AVERT(Pool, pool);
  AVERT(ScanState, ss);
  AVERT(Seg, seg);
  AVER(refIO != NULL);
  NOTREACHED;
  return ResUNIMPL;
}

void PoolNoReclaim(Pool pool, Trace trace, Seg seg)
{
  AVERT(Pool, pool);
  AVERT(Trace, trace);
  AVERT(Seg, seg);
  NOTREACHED;
}

void PoolTrivTraceEnd(Pool pool, Trace trace)
{
  AVERT(Pool, pool);
  AVERT(Trace, trace);
  NOOP;
}


void PoolNoRampBegin(Pool pool, Buffer buf, Bool collectAll)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
  AVERT(Bool, collectAll);
  NOTREACHED;
}


void PoolNoRampEnd(Pool pool, Buffer buf)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
  NOTREACHED;
}


void PoolTrivRampBegin(Pool pool, Buffer buf, Bool collectAll)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
  AVERT(Bool, collectAll);
}


void PoolTrivRampEnd(Pool pool, Buffer buf)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
}


Res PoolNoFramePush(AllocFrame *frameReturn, Pool pool, Buffer buf)
{
  AVER(frameReturn != NULL);
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
  NOTREACHED;
  return ResUNIMPL;
}


Res PoolNoFramePop(Pool pool, Buffer buf, AllocFrame frame)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
  /* frame is of an abstract type & can't be checked */
  UNUSED(frame);
  NOTREACHED;
  return ResUNIMPL;
}


void PoolNoFramePopPending(Pool pool, Buffer buf, AllocFrame frame)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
  /* frame is of an abstract type & can't be checked */
  UNUSED(frame);
  NOTREACHED;
}


Res PoolTrivFramePush(AllocFrame *frameReturn, Pool pool, Buffer buf)
{
  AVER(frameReturn != NULL);
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
  return ResOK;
}


Res PoolTrivFramePop(Pool pool, Buffer buf, AllocFrame frame)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
  /* frame is of an abstract type & can't be checked */
  UNUSED(frame);
  return ResOK;
}


void PoolTrivFramePopPending(Pool pool, Buffer buf, AllocFrame frame)
{
  AVERT(Pool, pool);
  AVERT(Buffer, buf);
  /* frame is of an abstract type & can't be checked */
  UNUSED(frame);
  NOOP;
}


Res PoolNoAddrObject(Addr *pReturn, Pool pool, Seg seg, Addr addr)
{
  AVER(pReturn != NULL);
  AVERT(Pool, pool);
  AVERT(Seg, seg);
  AVER(SegPool(seg) == pool);
  AVER(SegBase(seg) <= addr);
  AVER(addr < SegLimit(seg));
  return ResUNIMPL;
}

void PoolNoWalk(Pool pool, Seg seg, FormattedObjectsVisitor f,
                void *p, size_t s)
{
  AVERT(Pool, pool);
  AVERT(Seg, seg);
  AVER(FUNCHECK(f));
  /* p and s are arbitrary, hence can't be checked */
  UNUSED(p);
  UNUSED(s);

  NOTREACHED;
}


void PoolTrivFreeWalk(Pool pool, FreeBlockVisitor f, void *p)
{
  AVERT(Pool, pool);
  AVER(FUNCHECK(f));
  /* p is arbitrary, hence can't be checked */
  UNUSED(p);

  /* FreeWalk doesn't have be perfect, so just pretend you didn't find any. */
  NOOP;
}


BufferClass PoolNoBufferClass(void)
{
  NOTREACHED;
  return NULL;
}


Size PoolNoSize(Pool pool)
{
  AVERT(Pool, pool);
  NOTREACHED;
  return UNUSED_SIZE;
}


/* C. COPYRIGHT AND LICENSE
 *
 * Copyright (C) 2001-2015 Ravenbrook Limited <http://www.ravenbrook.com/>.
 * All rights reserved.  This is an open source license.  Contact
 * Ravenbrook for commercial licensing options.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * 3. Redistributions in any form must be accompanied by information on how
 * to obtain complete source code for this software and any accompanying
 * software that uses this software.  The source code must either be
 * included in the distribution or be available for no more than the cost
 * of distribution plus a nominal fee, and must be freely redistributable
 * under reasonable conditions.  For an executable file, complete source
 * code means the source code for all modules it contains. It does not
 * include source code for modules or files that typically accompany the
 * major components of the operating system on which the executable file
 * runs.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
