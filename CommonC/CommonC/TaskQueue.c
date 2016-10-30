/*
 *  Copyright (c) 2016, Stefan Johnson
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

#include "TaskQueue.h"
#include "MemoryAllocation.h"
#include "Assertion.h"
#include "Logging.h"
#include "ConcurrentQueue.h"
#include <stdatomic.h>

typedef struct CCTaskQueueInfo {
    CCAllocatorType allocator;
    CCConcurrentQueue tasks;
    CCTaskQueueExecute type;
    CCTask lastTask;
    atomic_flag lock;
} CCTaskQueueInfo;


static void CCTaskQueueDestructor(CCTaskQueue Queue)
{
    CCConcurrentQueueDestroy(Queue->tasks);
    if (Queue->lastTask) CCTaskDestroy(Queue->lastTask);
}

CCTaskQueue CCTaskQueueCreate(CCAllocatorType Allocator, CCTaskQueueExecute ExecutionType, CCConcurrentGarbageCollector GC)
{
    CCTaskQueue Queue = CCMalloc(Allocator, sizeof(CCTaskQueueInfo), NULL, CC_DEFAULT_ERROR_CALLBACK);
    
    if (Queue)
    {
        *Queue = (CCTaskQueueInfo){ .allocator = Allocator, .tasks = CCConcurrentQueueCreate(Allocator, GC), .type = ExecutionType, .lastTask = NULL, .lock = ATOMIC_FLAG_INIT };
        
        CCMemorySetDestructor(Queue, (CCMemoryDestructorCallback)CCTaskQueueDestructor);
    }
    
    return Queue;
}

void CCTaskQueueDestroy(CCTaskQueue Queue)
{
    CCAssertLog(Queue, "Queue must not be null");
    
    CCFree(Queue);
}

static void CCTaskQueueNodeDestructor(CCConcurrentQueueNode *Node)
{
    CCTaskDestroy(*(CCTask*)CCConcurrentQueueGetNodeData(Node));
}

void CCTaskQueuePush(CCTaskQueue Queue, CCTask Task)
{
    CCAssertLog(Queue, "Queue must not be null");
    CCAssertLog(Task, "Task must not be null");
    
    CCConcurrentQueueNode *Node = CCConcurrentQueueCreateNode(Queue->allocator, sizeof(CCTask), &Task);
    CCMemorySetDestructor(Node, (CCMemoryDestructorCallback)CCTaskQueueNodeDestructor);
    
    CCConcurrentQueuePush(Queue->tasks, Node);
}

CCTask CCTaskQueuePop(CCTaskQueue Queue)
{
    CCAssertLog(Queue, "Queue must not be null");
    
    if (Queue->type == CCTaskQueueExecuteSerially)
    {
        if (atomic_flag_test_and_set(&Queue->lock)) return NULL;
        
        if (Queue->lastTask)
        {
            if (!CCTaskIsFinished(Queue->lastTask))
            {
                atomic_flag_clear(&Queue->lock);
                return NULL;
            }
            
            CCTaskDestroy(Queue->lastTask);
            Queue->lastTask = NULL;
        }
    }
    
    CCConcurrentQueueNode *Node = CCConcurrentQueuePop(Queue->tasks);
    CCTask Task = CCRetain(*(CCTask*)CCConcurrentQueueGetNodeData(Node));
    CCConcurrentQueueDestroyNode(Node);
    
    if (Queue->type == CCTaskQueueExecuteSerially)
    {
        Queue->lastTask = CCRetain(Task);
        atomic_flag_clear(&Queue->lock);
    }
    
    return Task;
}
