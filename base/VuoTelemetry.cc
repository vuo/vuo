/**
 * @file
 * VuoTelemetry implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "VuoTelemetry.hh"

/**
 * Copies the message data into a newly allocated string.
 *
 * Assumes the message data includes a null terminator.
 *
 * If the message is zero-length, returns NULL.
 */
extern "C" char * vuoCopyStringFromMessage(zmq_msg_t *message)
{
	size_t messageSize = zmq_msg_size(message);
	if (!messageSize)
		return NULL;
	char *string = (char *)malloc(messageSize);
	memcpy(string, zmq_msg_data(message), messageSize);
	return string;
}

/**
 * Copies the string (including null terminator) into the message data.
 */
extern "C" void vuoInitMessageWithString(zmq_msg_t *message, const char *string)
{
	size_t messageSize = (string != NULL ? (strlen(string) + 1) : 0);
	zmq_msg_init_size(message, messageSize);
	memcpy(zmq_msg_data(message), string, messageSize);
}

/**
 * Copies the int value into the message data.
 */
extern "C" void vuoInitMessageWithInt(zmq_msg_t *message, int value)
{
	size_t messageSize = sizeof(int);
	zmq_msg_init_size(message, messageSize);
	memcpy(zmq_msg_data(message), &value, messageSize);
}

/**
 * Copies the bool value into the message data.
 */
extern "C" void vuoInitMessageWithBool(zmq_msg_t *message, bool value)
{
	size_t messageSize = sizeof(bool);
	zmq_msg_init_size(message, messageSize);
	memcpy(zmq_msg_data(message), &value, messageSize);
}

/**
 * Returns true if there are more messages to receive on the socket currently.
 */
extern "C" bool VuoTelemetry_hasMoreToReceive(void *socket)
{
	int more;
	size_t moreSize = sizeof more;
	zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &moreSize);
	return more;
}

/**
 * Receives the next message on the socket and copies it into a newly allocated string.
 */
extern "C" char * vuoReceiveAndCopyString(void *socket, char **error)
{
	zmq_msg_t message;
	zmq_msg_init(&message);

	if (zmq_msg_recv(&message, socket, 0) == -1)
	{
		int e = errno;
		char *errorMessage;
		if (e == EAGAIN)
			errorMessage = strdup("The connection between the composition and runner timed out when trying to receive a message");
		else
		{
			const char *eStr = zmq_strerror(e);
			const char *format = "The connection between the composition and runner failed when trying to receive a message (%s)";
			int size = snprintf(NULL, 0, format, eStr);
			errorMessage = (char *)malloc(size+1);
			snprintf(errorMessage, size+1, format, eStr);
		}
		if (error)
			*error = errorMessage;
		else
		{
			VUserLog("%s", errorMessage);
			free(errorMessage);
		}
		zmq_msg_close(&message);
		return NULL;
	}

	char *string = vuoCopyStringFromMessage(&message);
	zmq_msg_close(&message);
	return string;
}

/**
 * Helper function for @c vuoReceive functions for numerical types.
 */
void vuoReceiveBlocking(void *socket, void *data, size_t dataSize, char **error)
{
	zmq_msg_t message;
	zmq_msg_init(&message);

	if (zmq_msg_recv(&message, socket, 0) == -1)
	{
		int e = errno;
		char *errorMessage;
		if (e == EAGAIN)
			errorMessage = strdup("The connection between the composition and runner timed out when trying to receive a message");
		else if (e == ETERM)
			errorMessage = strdup("The connection between the composition and runner failed because the context was terminated when trying to receive a message");
		else
		{
			const char *eStr = zmq_strerror(e);
			const char *format = "The connection between the composition and runner failed when trying to receive a message (%s)";
			int size = snprintf(NULL, 0, format, eStr);
			errorMessage = (char *)malloc(size+1);
			snprintf(errorMessage, size+1, format, eStr);
		}
		if (error)
			*error = errorMessage;
		else
		{
			VUserLog("%s", errorMessage);
			free(errorMessage);
		}
		zmq_msg_close(&message);
		bzero(data, dataSize);
		return;
	}

	size_t messageSize = zmq_msg_size(&message);
	if (messageSize != dataSize)
	{
		const char *format = "A wrong-sized message was received in the connection between the composition and runner "
							 "(expected %lu bytes, received %lu bytes)";
		int size = snprintf(NULL, 0, format, (unsigned long)dataSize, (unsigned long)messageSize);
		char *errorMessage = (char *)malloc(size+1);
		snprintf(errorMessage, size+1, format, (unsigned long)dataSize, (unsigned long)messageSize);
		if (error)
			*error = errorMessage;
		else
		{
			VUserLog("%s", errorMessage);
			free(errorMessage);
		}
		zmq_msg_close(&message);
		bzero(data, dataSize);
		return;
	}

	memcpy(data, zmq_msg_data(&message), messageSize);
	zmq_msg_close(&message);
}

/**
 * Receives the next message on the socket and copies it into an unsigned long.
 */
extern "C" unsigned long vuoReceiveUnsignedInt64(void *socket, char **error)
{
	uint64_t number = 0;
	vuoReceiveBlocking(socket, (void *)&number, sizeof(number), error);
	return number;
}

/**
 * Receives the next message on the socket and copies it into an int.
 */
extern "C" int vuoReceiveInt(void *socket, char **error)
{
	int number = 0;
	vuoReceiveBlocking(socket, (void *)&number, sizeof(number), error);
	return number;
}

/**
 * Receives the next message on the socket and copies it into a bool.
 */
extern "C" bool vuoReceiveBool(void *socket, char **error)
{
	bool value = false;
	vuoReceiveBlocking(socket, (void *)&value, sizeof(value), error);
	return value;
}

/**
 * Sends the multipart message @c messages on ZMQ socket @c socket.
 * @c name is just used for printing error messages.
 */
extern "C" void vuoSend(const char *name, void *socket, int type, zmq_msg_t *messages, unsigned int messageCount, bool isNonBlocking, char **error)
{
	int e = 0;

	// send the type message-part
	{
		zmq_msg_t message;
		zmq_msg_init_size(&message, sizeof type);
		memcpy(zmq_msg_data(&message), &type, sizeof type);
		int flags = (messageCount > 0 ? ZMQ_SNDMORE : 0) | (isNonBlocking ? ZMQ_DONTWAIT : 0);
		if (zmq_msg_send(&message, socket, flags) == -1)
			e = errno;
		zmq_msg_close(&message);
	}

	// send the data message-parts
	for(unsigned int i=0; i<messageCount && e==0; ++i)
	{
		int flags = (i < messageCount - 1 ? ZMQ_SNDMORE : 0) | (isNonBlocking ? ZMQ_DONTWAIT : 0);
		if (zmq_msg_send(&messages[i], socket, flags) == -1)
			e = errno;
	}

	for(unsigned int i=0; i<messageCount; ++i)
		zmq_msg_close(&messages[i]);

	if (e != 0)
	{
		char *errorMessage;
		if (e == EAGAIN)
		{
			const char *format = "The connection between the composition and runner timed out when trying to send a message of type %i on '%s'";
			int size = snprintf(NULL, 0, format, type, name);
			errorMessage = (char *)malloc(size+1);
			snprintf(errorMessage, size+1, format, type, name);
		}
		else
		{
			const char *eStr = zmq_strerror(e);
			const char *format = "The connection between the composition and runner failed when trying to send a message of type %i on '%s' (%s)";
			int size = snprintf(NULL, 0, format, type, name, eStr);
			errorMessage = (char *)malloc(size+1);
			snprintf(errorMessage, size+1, format, type, name, eStr);
		}
		if (error)
			*error = errorMessage;
		else
		{
			VUserLog("%s", errorMessage);
			free(errorMessage);
		}
	}
}

#include <atomic>
/**
 * "Individual ØMQ sockets are not thread safe except in the case where full memory barriers
 * are issued when migrating a socket from one thread to another."
 *
 * http://api.zeromq.org/4-3:zmq
 * https://stackoverflow.com/questions/5841896/0mq-how-to-use-zeromq-in-a-threadsafe-manner
 * https://stackoverflow.com/questions/25478029/does-atomic-thread-fencememory-order-seq-cst-have-the-semantics-of-a-full-memo
 * https://b33p.net/kosada/vuo/vuo/-/issues/4226
 */
extern "C" void vuoMemoryBarrier(void)
{
	atomic_thread_fence(memory_order_seq_cst);
}
