## Files in this Directory
* `Makefile` - Type `make` to build the program.
* `README.md` - This README.
* `queue.c` - Implementation of a bounded buffer.
* `queue.h` - Provided header file for this assignment.

## Implementation Details
### Struct
This Queue ADT is implemented using a `queue` struct. This struct includes three semaphores, included for mutual exclusion, thread safety, and checking the state of the queue without busy waiting.

### Constructor and Destructor
The constructor `queue_new` takes an integer `size` as a parameter. It allocates an appropriate amount of memory for the queue and buffer and initializes a queue with the given `size`.

The destructor `queue_delete` takes a queue object as a parameter. It destroys the semaphores, frees the memory allocated to the buffer, and frees the memory associated with the queue itself.

### Push and Pop
The function `queue_push` is equivalent to an enqueue operation. This function takes a queue object and an element of any type as parameters. It checks to see if the queue is full, and if so, blocks. Otherwise, the element gets added to the queue.

The function `queue_pop` is equivalent to a dequeue operation. This function takes a queue object and a pointer to an arbitrary element as parameters. If the queue is empty, this function will block. Otherwise, the pointer is assigned to the item at the front of the queue and that item is removed from the queue.