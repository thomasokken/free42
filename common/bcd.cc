/**
 * Copyright (c) 2005 voidware ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "bcd.h"

BCDRef* BCDRef::pool_ = NULL;

BCDRef* BCDRef::_alloc()
{
    BCDRef* ref = pool_;
    if (ref) {
        pool_ = pool_->next_;
    }
    else {
        ref = new BCDRef;
    }
    return ref;
}

/* A little hack to force the pooled BCDRef instances to be deleted.
 * This avoids the memory leak warnings that POSE will otherwise display on app
 * exit; I don't want to simply ignore those warnings, because one day they
 * might just alert me to a *real* memory leak.
 */
struct PoolCleaner {
    ~PoolCleaner() BCD_SECT;
    int foo;
};
static PoolCleaner poolCleanerInstance;

PoolCleaner::~PoolCleaner() {
    BCDRef *r = BCDRef::pool_;
    while (r != NULL) {
	BCDRef *r2 = r;
	r = r->next_;
	delete r2;
    }
}

char BCD::buf_[64];

#ifndef PALMOS
const char* BCD::asString() const
{
    if (ref_) {
        ref_->v_.asString(buf_);
    }
    else *buf_ = 0;
    return buf_;
}
#endif

BCD fabs(const BCD& a)
{
    if (a.isNeg()) return -a;
    return a;
}

BCD sqrt(const BCD& a)
{
    BCDRef* t = BCDRef::_alloc();
    if (!BCDFloat::sqrt(&a.ref_->v_, &t->v_)) {
        t->v_ = BCDFloat::nan();
    }
    return t;
}

BCD floor(const BCD& a)
{ 
    /* floor, largest integer <= a.
     * eg floor(2.1) = 2.
     *    floor(-2.1) = -3.
     */
    if (a.isSpecial()) return a;

    BCDRef* t = BCDRef::_alloc();    
    BCDFloat::floor(&a.ref_->v_, &t->v_);
    return t;
}

BCD trunc(const BCD& a)
{
    /* truncate towards zero.
     * trunc(2.1) = 2.
     * trunc(-2.1) = -2
     */

    if (a.isSpecial()) return a;

    BCDRef* t = BCDRef::_alloc();    
    BCDFloat::trunc(&a.ref_->v_, &t->v_);
    return t;
}

BCD frac(const BCD& a)
{ 
    if (a.isSpecial()) return a;
    return a - trunc(a);
}
