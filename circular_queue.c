/* 
 * file: circular_queue.c
 * author: J. van Hooydonk
 * comments: LocoNet driver, following the project of G. Giebens https://github.com/GeertGiebens
 *
 * revision history:
 *  v1.0 Creation (14/01/2024)
 */

#include "circular_queue.h"

/**
 * initialise the queue
 * @param queue: name of the queue (pass the address of the queue)
 */
void initQueue(lnQueue_t* queue)
{
    queue->size = QUEUE_SIZE;
    queue->head = 0;
    queue->tail = 0;
    queue->numEntries = 0;
}

/**
 * check if the queue is empty
 * @param queue: name of the queue (pass the address of the queue)
 * @return true: if queue is empty, false; if queue is not empty
 */
bool isQueueEmpty(lnQueue_t* queue)
{
    return (queue->numEntries == 0);
}

/**
 * check if the queue is full
 * @param q: name of the queue (pass the address of the queue)
 * @return true: if queue is full, false; if queue is not full
 */bool isQueueFull(lnQueue_t* queue)
{
    return (queue->numEntries == queue->size);
}

/**
 * put a value on the queue
 * @param queue: name of the queue (pass the address of the queue)
 * @param value: the value to put on the queue
 * @return true: if value is put on the queue, false; if queue is full
 */
bool enQueue(lnQueue_t* queue, uint8_t value)
{
    // put a value on the queue
    if (isQueueFull(queue))
    {
       // return false if queue is full
       return false;
    }
    else
    {
        // put the value to the queue and return true
        queue->values[queue->tail] = value;
        queue->numEntries++;
        queue->tail = (queue->tail + 1) % queue->size;
        return true;
    }
}

/**
 * get a value from the queue
 * @param queue: name of the queue (pass the address of the queue)
 * @return true: if last value is get from the queue, false; if queue is empty
 */
bool deQueue(lnQueue_t* queue)
{
    // get a value from the queue
    if (isQueueEmpty(queue))
    {
        // return false if queue is empty
        return false;
    }
    else
    {
        // set the values and return true
        queue->head = (queue->head + 1) % queue->size;
        queue->numEntries--;
        return true;
    }
}

/**
 * clear the content of the queue
 * @param lnQueue: name of the queue (pass the address of the queue)
 */
void clearQueue(lnQueue_t* lnQueue)
{
    while (!isQueueEmpty(lnQueue))
    {
        deQueue(lnQueue);
    }
}

/**
 * recover the LN message by setting the head to the begin of the LN message
 * @param lnQueue: name of the queue (pass the address of the queue)
 */
void recoverLnMessage(lnQueue_t* lnQueue)
{
    if (!isQueueEmpty(lnQueue))
    {
        // rewind head to the begin of the LN message
        while ((lnQueue->values[lnQueue->head] & 0x80) != 0x80)
        {
            lnQueue->head = (lnQueue->head - 1) % lnQueue->size;
            lnQueue->numEntries++;
        }        
    }
}
