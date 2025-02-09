/*
 *  Copyright (c) 2015, Stefan Johnson
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list
 *     of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this
 *     list of conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define CC_QUICK_COMPILE
#include "Array.h"
#include "MemoryAllocation.h"
#include <string.h>


static void CCArrayDestructor(CCArray Ptr)
{
    CCFree(Ptr->data);
}

CCArray CCArrayCreate(CCAllocatorType Allocator, size_t ElementSize, size_t ChunkSize)
{
    CCAssertLog(ChunkSize >= 1, "ChunkSize must be at least 1");
    
    CCArray Array = CCMalloc(Allocator, sizeof(CCArrayInfo), NULL, CC_DEFAULT_ERROR_CALLBACK);
    if (Array)
    {
        *Array = (CCArrayInfo){
            .size = ElementSize,
            .chunkSize = ChunkSize,
            .count = 0,
            .data = CCMalloc(Allocator, ChunkSize * ElementSize, NULL, CC_DEFAULT_ERROR_CALLBACK)
        };
        
        CCMemorySetDestructor(Array, (CCMemoryDestructorCallback)CCArrayDestructor);
    }
    
    
    return Array;
}

void CCArrayDestroy(CCArray Array)
{
    CCAssertLog(Array, "Array must not be null");
    CCFree(Array);
}

size_t CCArrayAppendElement(CCArray Array, const void *Element)
{
    CCAssertLog(Array, "Array must not be null");
    
    if (((Array->count % Array->chunkSize) == 0) && ((Array->count) || (!Array->data)))
    {
        CCArray Temp = CCRealloc(CC_STD_ALLOCATOR, Array->data, (((Array->count / Array->chunkSize) + 1) * Array->chunkSize * Array->size), NULL, CC_DEFAULT_ERROR_CALLBACK);
        if (!Temp)
        {
            CC_LOG_ERROR("Failed to append element to array (%p), could not allocate (%zu)", Array, (((Array->count / Array->chunkSize) + 1) * Array->chunkSize * Array->size));
            return SIZE_MAX;
        }
        
        Array->data = Temp;
    }
    
    if (Element) memcpy(Array->data + (Array->count * Array->size), Element, Array->size);
    
    return Array->count++;
}

size_t CCArrayAppendElements(CCArray Array, const void *Elements, size_t Count)
{
    CCAssertLog(Array, "Array must not be null");
    CCAssertLog(Count, "Count must not be 0");
    
    size_t Rem = Array->count % Array->chunkSize;
    if (((Array->chunkSize - Rem) < Count) || ((Rem == 0) && ((Array->count) || (!Array->data))))
    {
        CCArray Temp = CCRealloc(CC_STD_ALLOCATOR, Array->data, ((((Array->count + Count) / Array->chunkSize) + 1) * Array->chunkSize * Array->size), NULL, CC_DEFAULT_ERROR_CALLBACK);
        if (!Temp)
        {
            CC_LOG_ERROR("Failed to append (%zu) elements to array (%p), could not allocate (%zu)", Count, Array, ((((Array->count + Count) / Array->chunkSize) + 1) * Array->chunkSize * Array->size));
            return SIZE_MAX;
        }
        
        Array->data = Temp;
    }
    
    if (Elements) memcpy(Array->data + (Array->count * Array->size), Elements, Array->size * Count);
    
    const size_t Index = Array->count;
    Array->count += Count;
    
    return Index;
}

void CCArrayReplaceElementAtIndex(CCArray Array, size_t Index, const void *Element)
{
    CCAssertLog(Array, "Array must not be null");
    CCAssertLog(Array->count > Index, "Index must not be out of bounds");
    
    if (Element) memcpy(Array->data + (Index * Array->size), Element, Array->size);
}

void CCArrayReplaceElementsAtIndex(CCArray Array, size_t Index, const void *Elements, size_t Count)
{
    CCAssertLog(Array, "Array must not be null");
    CCAssertLog(Array->count > Index, "Index must not be out of bounds");
    CCAssertLog(Count, "Count must not be 0");
    
    if (Elements) memcpy(Array->data + (Index * Array->size), Elements, Array->size * Count);
}

size_t CCArrayInsertElementAtIndex(CCArray Array, size_t Index, const void *Element)
{
    CCAssertLog(Array, "Array must not be null");
    CCAssertLog(Array->count > Index, "Index must not be out of bounds");
    
    if (((Array->count % Array->chunkSize) == 0) && ((Array->count) || (!Array->data)))
    {
        CCArray Temp = CCRealloc(CC_STD_ALLOCATOR, Array->data, (((Array->count / Array->chunkSize) + 1) * Array->chunkSize * Array->size), NULL, CC_DEFAULT_ERROR_CALLBACK);
        if (!Temp)
        {
            CC_LOG_ERROR("Failed to insert element into array (%p), could not allocate (%zu)", Array, (((Array->count / Array->chunkSize) + 1) * Array->chunkSize * Array->size));
            return SIZE_MAX;
        }
        
        Array->data = Temp;
    }
    
    memmove(Array->data + ((Index + 1) * Array->size), Array->data + (Index * Array->size), (++Array->count - (Index + 1)) * Array->size);
    if (Element) memcpy(Array->data + (Index * Array->size), Element, Array->size);
    
    return Index;
}

size_t CCArrayInsertElementsAtIndex(CCArray Array, size_t Index, const void *Elements, size_t Count)
{
    CCAssertLog(Array, "Array must not be null");
    CCAssertLog(Array->count > Index, "Index must not be out of bounds");
    CCAssertLog(Count, "Count must not be 0");
    
    size_t Rem = Array->count % Array->chunkSize;
    if (((Array->chunkSize - Rem) < Count) || ((Rem == 0) && ((Array->count) || (!Array->data))))
    {
        CCArray Temp = CCRealloc(CC_STD_ALLOCATOR, Array->data, ((((Array->count + Count) / Array->chunkSize) + 1) * Array->chunkSize * Array->size), NULL, CC_DEFAULT_ERROR_CALLBACK);
        if (!Temp)
        {
            CC_LOG_ERROR("Failed to insert (%zu) elements into array (%p), could not allocate (%zu)", Count, Array, ((((Array->count + Count) / Array->chunkSize) + 1) * Array->chunkSize * Array->size));
            return SIZE_MAX;
        }
        
        Array->data = Temp;
    }
    
    Array->count += Count;
    
    memmove(Array->data + ((Index + Count) * Array->size), Array->data + (Index * Array->size), (Array->count - (Index + Count)) * Array->size);
    if (Elements) memcpy(Array->data + (Index * Array->size), Elements, Array->size * Count);
    
    return Index;
}

void CCArrayRemoveElementAtIndex(CCArray Array, size_t Index)
{
    CCAssertLog(Array, "Array must not be null");
    CCAssertLog(Array->count > Index, "Index must not be out of bounds");
    
    CCArrayRemoveElementsAtIndex(Array, Index, 1);
}

void CCArrayRemoveElementsAtIndex(CCArray Array, size_t Index, size_t Count)
{
    CCAssertLog(Array, "Array must not be null");
    CCAssertLog(Array->count > (Index + Count - 1), "Index range must not be out of bounds");
    
    memmove(Array->data + (Index * Array->size), Array->data + ((Index + Count) * Array->size), (Array->count - (Index + Count)) * Array->size);
    Array->count -= Count;
}

void CCArrayRemoveAllElements(CCArray Array)
{
    CCAssertLog(Array, "Array must not be null");
    
    Array->count = 0;
}
