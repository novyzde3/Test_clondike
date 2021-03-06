/**
 * @file tcmi_err_msg.c - TCMI error message - carries an error status code.
 *       
 *                      
 * 
 *
 *
 * Date: 04/11/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_err_msg.c,v 1.3 2007/09/02 10:54:25 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2005  Jan Capek
 * 
 * TCMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * TCMI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <linux/slab.h>

#define TCMI_ERR_MSG_PRIVATE
#include "tcmi_err_msg.h"


#include <dbg.h>



/** 
 * \<\<public\>\> Error message rx constructor.  This constructor is
 * made available for other messages, so that they can specify it as
 * their error constructor for \link tcmi_msg_factory_class the
 * factory class \endlink.
 *
 * @param msg_id - message ID that will be used for this error message
 * instance.
 * @return a new error message or NULL.
 */
struct tcmi_msg* tcmi_err_msg_new_rx(u_int32_t msg_id)
{
	struct tcmi_err_msg *msg;

	if (!(msg = TCMI_ERR_MSG(kmalloc(sizeof(struct tcmi_err_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate memory for error message %x", msg_id);
		goto exit0;
	}
	/* Initialized the message for receiving, message ID is
	 * extended with error flags */
	if (tcmi_msg_init_rx(TCMI_MSG(msg), TCMI_MSG_FLG_SET_ERR(msg_id), &err_msg_ops)) {
		mdbg(ERR3, "Error initializing error message %x", msg_id);
		goto exit1;
	}

	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
}

/** 
 * \<\<public\>\> Error message constructor for transferring.
 *
 * An error message is considered one way only and is usually used as a
 * error response to some other message. The user thus has to specify
 * the transaction ID of the message that the error message is a response too.
 * Messages with TCMI_TRANSACTION_INVAL_ID are possible too. No search
 * for a matching transaction will be performed at the receiving party and
 * the message will be queued in a regular request queue.
 *
 * Basic part of instantation is the same as when creating an Rx version.
 *
 * @param msg_id - ID assigned to the error message, it will be extended
 * with error flags, so that the message factory at the receiving party uses
 * the above implemented tcmi_err_msg_new_rx() constructor.
 * @param trans_id - transaction ID 

 * @param err_code - error code that will be stored in the message
 *
 * @return a new error ready for the transfer or NULL.
 */
struct tcmi_msg* tcmi_err_msg_new_tx(u_int32_t msg_id, u_int32_t trans_id, int32_t err_code)
{
	struct tcmi_err_msg *msg;

	if (!(msg = TCMI_ERR_MSG(kmalloc(sizeof(struct tcmi_err_msg), GFP_KERNEL)))) {
		mdbg(ERR3, "Can't allocate memory for error message %x", msg_id);
		goto exit0;
	}

	/* Initialize the message for transfer, message ID is
	 * extended with error flags, no transaction created or
	 * response ID setup. Only the transaction ID, that will get
	 * send along with the message, is set. */
	if (tcmi_msg_init_tx(TCMI_MSG(msg), TCMI_MSG_FLG_SET_ERR(msg_id), &err_msg_ops, 
			     NULL, 0, 0, trans_id)) {
		mdbg(ERR3, "Error initializing error message %x", msg_id);
		goto exit1;
	}
	msg->err_code = err_code;
	return TCMI_MSG(msg);

	/* error handling */
 exit1:
	kfree(msg);
 exit0:
	return NULL;
	
}


/** @addtogroup tcmi_err_msg_class
 *
 * @{
 */

/**
 * \<\<private\>\> Receives the message via a specified connection.
 * Receiving the error message requires reading the error code from
 * the specified socket
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully received.
 */
static int tcmi_err_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_err_msg *self_err = TCMI_ERR_MSG(self);

	err = kkc_sock_recv(sock, &self_err->err_code, 
			    sizeof(self_err->err_code), KKC_SOCK_BLOCK);
	mdbg(INFO3, "Received error code: %x", self_err->err_code);
	
	return err;
}

/**
 * \<\<private\>\> Sends out the error code via specified socket.
 *
 * @param *self - this message instance
 * @param *sock - KKC socket used for receiving message data
 * @return 0 when successfully sent.
 */
static int tcmi_err_msg_send(struct tcmi_msg *self, struct kkc_sock *sock)
{
	int err;
	struct tcmi_err_msg *self_err = TCMI_ERR_MSG(self);

	err = kkc_sock_send(sock, &self_err->err_code, 
			    sizeof(self_err->err_code), KKC_SOCK_BLOCK);

	mdbg(INFO3, "Sent error code: %x", self_err->err_code);

	return err;
}



/** Message operations that support polymorphism. */
static struct tcmi_msg_ops err_msg_ops = {
	.recv = tcmi_err_msg_recv,
	.send = tcmi_err_msg_send
};


/**
 * @}
 */

