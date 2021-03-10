/*
 * ==============================================================================
 *  Name        : fast_malloc
 *  Part of     : Memory Manager
 *  Description : Header file for fast_malloc.cpp
 *  Version     : 2.0
 *
 *    Copyright (c) 2006, Nokia Corporation
 *    All rights reserved.
 *  
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *  
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in
 *        the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the Nokia Corporation nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *  
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *   DAMAGE.
 *  
 *    Please see file patentlicense.txt for further grants.
 * ==============================================================================
 */

// NOTE:
// This header file is used by fast_malloc.cpp, which is a public domain file.
// fast_malloc.cpp is a version (aka dlmalloc) of malloc/free/realloc written
// by Doug Lea and released to the public domain, as explained at
// http://creativecommons.org/licenses/publicdomain.  Send questions,
// comments, complaints, performance data, etc to dl@cs.oswego.edu

// NOTE:
// version 2.8.4

#ifndef FAST_MALLOC_H
#define FAST_MALLOC_H

#include <stdlib.h>

void *fast_malloc(size_t n);
void *fast_calloc(size_t n_elements, size_t element_size);
void *fast_realloc(void* p, size_t n);
void fast_free(void* p);
int free_memory(size_t& pool, size_t& heap, size_t& sys);
bool owned_by_pool(void* p);
unsigned int fast_malloc_usable_size(void*);
bool fast_pre_check(size_t, size_t);
void fast_post_check();
void close_mem_pool();
void fast_set_rescue_buffer_size(int size);
unsigned int fast_malloc_size(void* p);
void alloc_rescue_buffer();

#endif /* FAST_MALLOC_H */
