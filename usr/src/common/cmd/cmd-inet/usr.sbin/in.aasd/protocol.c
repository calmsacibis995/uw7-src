#ident "@(#)protocol.c	1.3"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
 * This file contains code to handle the AAS protocol.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <malloc.h>
#include <syslog.h>
#include <sys/time.h>
#include <string.h>
#include <db.h>
#include <aas/aas.h>
#include <aas/aas_auth.h>
#include <aas/aas_proto.h>
#include <aas/aas_util.h>
#include "aasd.h"
#include "atype.h"
#include "addr_db.h"

extern Password *passwords;
extern int config_ok;

/*
 * check_auth -- check authentication in a received message
 * check_auth calls aas_authenticate to check the authentication (digest)
 * in a message received from the client.  If this is the first message
 * received from a client, each password is checked until a match is found.
 * The matched password is then stored in the connection structure for
 * future use.  The function returns 1 if authentication succeeds, or 0
 * if not.
 */

static int
check_auth(Connection *conn, int data_len)
{
	Password *pwd;
	int len;

	if (conn->password) {
		return aas_authenticate(conn->msg_info.msg_buf, data_len,
			conn->password, conn->password_len,
			conn->last_digest);
	}
	else {
		/*
		 * Fixed password support for an address family was
		 * added as a simple way to effectively remove password
		 * protection from UNIX domain socket connections.
		 */
		if (conn->af->password) {
			len = strlen(conn->af->password);
			if (aas_authenticate(conn->msg_info.msg_buf, data_len,
			    conn->af->password, len,
			    conn->last_digest)) {
				conn->password = conn->af->password;
				conn->password_len = len;
				return 1;
			}
			else {
				return 0;
			}
		}
		for (pwd = passwords; pwd; pwd = pwd->next) {
			if (aas_authenticate(conn->msg_info.msg_buf, data_len,
			    pwd->password, pwd->len,
			    conn->last_digest)) {
				conn->password = pwd->password;
				conn->password_len = pwd->len;
				return 1;
			}
		}
		return 0;
	}
}

/*
 * send_ack -- send an acknowledgment
 */

static void
send_ack(Connection *conn)
{
	struct {
		AasMsgAck ack;
		char digest[AAS_MSG_DIGEST_SIZE];
	} msg;

	if (debug) {
		report(LOG_INFO, "Sending ACK");
	}

	msg.ack.msg_type = htonl(AAS_MSG_ACK);
	msg.ack.msg_len = htonl((ulong) sizeof(msg));

	/*
	 * Generate the digest.
	 */
	
	aas_generate_digest((char *)&msg.ack, sizeof(AasMsgAck),
		conn->password, conn->password_len,
		conn->last_digest, msg.digest);
	
	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, msg.digest, AAS_MSG_DIGEST_SIZE);

	if (write(conn->fd, &msg, sizeof(msg)) == -1) {
		report(LOG_ERR, "send_ack: write failed: %m");
		/*
		 * Close the connection.
		 */
		close(conn->fd);
		conn->fd = -1;
	}
}

/*
 * send_nack -- send a negative acknowledgment
 */

void
send_nack(Connection *conn, int code)
{
	char *password;
	int password_len;

	struct {
		AasMsgNack nack;
		char digest[AAS_MSG_DIGEST_SIZE];
	} msg;

	if (debug) {
		report(LOG_INFO, "Sending NACK, code %d (%s)",
			code, aas_strerror(code));
	}

	msg.nack.msg_type = htonl(AAS_MSG_NACK);
	msg.nack.msg_len = htonl((ulong) sizeof(msg));
	msg.nack.error_code = htons((ushort) code);

	/*
	 * Generate the digest.  Since this function could have been called
	 * because the authentication didn't check out, the connection
	 * might not have a password set.  If there is no password, we
	 * use the first password on the list.  The client will recognize
	 * the authentication problem because the digest won't check out on
	 * its end either.
	 */
	
	if (conn->password) {
		password = conn->password;
		password_len = conn->password_len;
	}
	else {
		password = passwords->password;
		password_len = passwords->len;
	}
	
	aas_generate_digest((char *)&msg.nack, sizeof(AasMsgNack), password,
		password_len, conn->last_digest, msg.digest);
	
	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, msg.digest, AAS_MSG_DIGEST_SIZE);

	if (write(conn->fd, &msg, sizeof(msg)) == -1) {
		report(LOG_ERR, "send_nack: write failed: %m");
		/*
		 * Close the connection.
		 */
		close(conn->fd);
		conn->fd = -1;
	}
}

static void
proc_alloc_req(Connection *conn)
{
	AasMsgAllocReq *req;
	AasMsgAllocResp *resp;
	ulong len, data_len;
	AasAddr req_addr, min_addr, max_addr;
	AasAddr *allocp;
	AasClientId id;
	char *pool, *service, *p;
	char *msgp;
	char *buf, *ptr;
	int ret, c;

	if (debug) {
		report(LOG_INFO, "Received ALLOC_REQ");
	}

	/*
	 * Check the digest.
	 */
	
	if (conn->msg_info.header.msg_len
	    < sizeof(AasMsgHeader) + AAS_MSG_DIGEST_SIZE) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	data_len = conn->msg_info.header.msg_len - AAS_MSG_DIGEST_SIZE;
	if (!check_auth(conn, data_len)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_AUTH_FAILURE);
		return;
	}

	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, &conn->msg_info.msg_buf[data_len],
		AAS_MSG_DIGEST_SIZE);

	/*
	 * Make sure message is as big as the fixed part.
	 */

	if (data_len < sizeof(AasMsgAllocReq)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	req = (AasMsgAllocReq *) conn->msg_info.msg_buf;

	/*
	 * Put all fields into host order.
	 */

	req->addr_type = ntohs(req->addr_type);
	req->flags = ntohs(req->flags);
	req->lease_time = ntohl(req->lease_time);
	req->pool_len = ntohs(req->pool_len);
	req->req_addr_len = ntohs(req->req_addr_len);
	req->min_addr_len = ntohs(req->min_addr_len);
	req->max_addr_len = ntohs(req->max_addr_len);
	req->service_len = ntohs(req->service_len);
	req->client_id_len = ntohs(req->client_id_len);

	len = sizeof(AasMsgAllocReq) + req->pool_len + req->req_addr_len
		+ req->min_addr_len + req->max_addr_len
		+ req->service_len + req->client_id_len;

	/*
	 * Verify length of message and make sure required fields are set.
	 */

	if (data_len != len || req->pool_len == 0 || req->service_len == 0
	    || req->client_id_len == 0 || req->lease_time == 0) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	/*
	 * Extract the variable fields.  Verify that strings are
	 * null-terminated.
	 */

	msgp = (char *) (req + 1);
	if (!CHECK_STRING(req, pool_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, pool_len, pool, msgp);
	GET_ADDR(req, req_addr_len, &req_addr, msgp);
	GET_ADDR(req, min_addr_len, &min_addr, msgp);
	GET_ADDR(req, max_addr_len, &max_addr, msgp);
	if (!CHECK_STRING(req, service_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, service_len, service, msgp);
	GET_CLIENT_ID(req, client_id_len, &id, msgp);

	/*
	 * Make sure the service name is valid.  Must contain only
	 * printable ASCII characters except space.
	 */
	
	for (p = service; *p; p++) {
		c = *((unsigned char *) p);
		if (c <= 0x20 || c >= 0x7f) {
			free(conn->msg_info.msg_buf);
			send_nack(conn, AAS_BAD_SERVICE);
			return;
		}
	}

	/*
	 * Check for bad configuration.
	 */
	
	if (!config_ok) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_CONFIG_ERROR);
		return;
	}
	
	if (debug) {
		report(LOG_INFO,
		    "ALLOC_REQ pool %s (type %d) flags 0x%x lease %d service %s client %s",
		    pool, req->addr_type,
		    req->flags, req->lease_time, service,
		    binary2string(id.id, id.len));
	}

	/*
	 * Do the allocation.
	 */

	ret = alloc_addr(pool, req->addr_type, &req_addr, &min_addr,
		&max_addr, req->flags, req->lease_time, service, &id,
		&allocp);
	
	free(conn->msg_info.msg_buf);

	if (ret == AAS_OK) {
		/*
		 * Allocate a buffer and assemble the response.
		 */
		data_len = sizeof(AasMsgAllocResp) + allocp->len;
		len = data_len + AAS_MSG_DIGEST_SIZE;
		if (!(buf = malloc(len))) {
			malloc_error("proc_alloc_req");
			send_nack(conn, AAS_SYSTEM_ERROR);
			return;
		}
		resp = (AasMsgAllocResp *) buf;
		resp->msg_type = htonl(AAS_MSG_ALLOC_RESP);
		resp->msg_len = htonl(len);
		resp->addr_len = htons(allocp->len);
		ptr = (char *) (resp + 1);
		memcpy(ptr, allocp->addr, allocp->len);
		ptr += allocp->len;

		/*
		 * Generate the digest.
		 */
		
		aas_generate_digest(buf, data_len, conn->password,
			conn->password_len, conn->last_digest, ptr);
		
		/*
		 * Save the digest.
		 */
		
		memcpy(conn->last_digest, ptr, AAS_MSG_DIGEST_SIZE);

		/*
		 * Set up to send the response.
		 */

		conn->send_buf = buf;
		conn->send_ptr = buf;
		conn->send_rem = len;
	}
	else {
		/*
		 * Failed -- send NACK.
		 */
		send_nack(conn, ret);
	}
}

static void
proc_free_req(Connection *conn)
{
	AasMsgFreeReq *req;
	ulong len, data_len;
	AasAddr addr;
	AasClientId id;
	char *pool, *service;
	char *msgp;
	int ret;

	if (debug) {
		report(LOG_INFO, "Received FREE_REQ");
	}

	/*
	 * Check the digest.
	 */
	
	if (conn->msg_info.header.msg_len
	    < sizeof(AasMsgHeader) + AAS_MSG_DIGEST_SIZE) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	data_len = conn->msg_info.header.msg_len - AAS_MSG_DIGEST_SIZE;
	if (!check_auth(conn, data_len)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_AUTH_FAILURE);
		return;
	}

	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, &conn->msg_info.msg_buf[data_len],
		AAS_MSG_DIGEST_SIZE);

	/*
	 * Make sure message is as big as the fixed part.
	 */

	if (data_len < sizeof(AasMsgFreeReq)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	req = (AasMsgFreeReq *) conn->msg_info.msg_buf;

	/*
	 * Put all fields into host order.
	 */

	req->addr_type = ntohs(req->addr_type);
	req->pool_len = ntohs(req->pool_len);
	req->addr_len = ntohs(req->addr_len);
	req->service_len = ntohs(req->service_len);
	req->client_id_len = ntohs(req->client_id_len);

	len = sizeof(AasMsgFreeReq) + req->pool_len + req->addr_len
		+ req->service_len + req->client_id_len;

	/*
	 * Verify length of message and make sure required fields are set.
	 */

	if (data_len != len || req->pool_len == 0 || req->addr_len == 0
	    || req->service_len == 0 || req->client_id_len == 0) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	/*
	 * Extract the variable fields.
	 */
	
	msgp = (char *) (req + 1);
	if (!CHECK_STRING(req, pool_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, pool_len, pool, msgp);
	GET_ADDR(req, addr_len, &addr, msgp);
	if (!CHECK_STRING(req, service_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, service_len, service, msgp);
	GET_CLIENT_ID(req, client_id_len, &id, msgp);

	/*
	 * Check for bad configuration.
	 */
	
	if (!config_ok) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_CONFIG_ERROR);
		return;
	}
	
	if (debug) {
		report(LOG_INFO,
		    "FREE_REQ pool %s (type %d) service %s client %s",
		    pool, req->addr_type, service,
		    binary2string(id.id, id.len));
	}

	/*
	 * Free the address.
	 */

	ret = free_addr(pool, req->addr_type, &addr, service, &id);
	
	free(conn->msg_info.msg_buf);

	if (ret == AAS_OK) {
		/*
		 * OK -- send ACK.
		 */
		send_ack(conn);
	}
	else {
		/*
		 * Failed -- send NACK.
		 */
		send_nack(conn, ret);
	}
}

static void
proc_free_all_req(Connection *conn)
{
	AasMsgFreeAllReq *req;
	ulong len, data_len;
	char *pool, *service;
	char *msgp;
	int ret;

	if (debug) {
		report(LOG_INFO, "Received FREE_ALL_REQ");
	}

	/*
	 * Check the digest.
	 */
	
	if (conn->msg_info.header.msg_len
	    < sizeof(AasMsgHeader) + AAS_MSG_DIGEST_SIZE) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	data_len = conn->msg_info.header.msg_len - AAS_MSG_DIGEST_SIZE;
	if (!check_auth(conn, data_len)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_AUTH_FAILURE);
		return;
	}

	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, &conn->msg_info.msg_buf[data_len],
		AAS_MSG_DIGEST_SIZE);

	/*
	 * Make sure message is as big as the fixed part.
	 */

	if (data_len < sizeof(AasMsgFreeAllReq)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	req = (AasMsgFreeAllReq *) conn->msg_info.msg_buf;

	/*
	 * Put all fields into host order.
	 */

	req->pool_len = ntohs(req->pool_len);
	req->service_len = ntohs(req->service_len);

	len = sizeof(AasMsgFreeAllReq) + req->pool_len + req->service_len;

	/*
	 * Verify length of message and make sure required fields are set.
	 */

	if (data_len != len || req->pool_len == 0
	    || req->service_len == 0) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	/*
	 * Extract the variable fields.
	 */
	
	msgp = (char *) (req + 1);
	if (!CHECK_STRING(req, pool_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, pool_len, pool, msgp);
	/*
	 * If the pool length is 1, it indicates
	 * that no pool was specified
	 */ 
	if (req->pool_len == 1) {
		pool = NULL;
	}
	if (!CHECK_STRING(req, service_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, service_len, service, msgp);

	/*
	 * Check for bad configuration.
	 */
	
	if (!config_ok) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_CONFIG_ERROR);
		return;
	}
	
	if (debug) {
		report(LOG_INFO, "FREE_ALL_REQ pool %s service %s",
		    pool, service);
	}

	/*
	 * Free the addresses.
	 */

	ret = free_all(pool, service);
	
	free(conn->msg_info.msg_buf);

	if (ret == AAS_OK) {
		/*
		 * OK -- send ACK.
		 */
		send_ack(conn);
	}
	else {
		/*
		 * Failed -- send NACK.
		 */
		send_nack(conn, ret);
	}
}

static void
proc_disable_req(Connection *conn)
{
	AasMsgDisableReq *req;
	ulong len, data_len;
	AasAddr addr;
	char *pool;
	char *msgp;
	int ret;

	if (debug) {
		report(LOG_INFO, "Received DISABLE_REQ");
	}

	/*
	 * Check the digest.
	 */
	
	if (conn->msg_info.header.msg_len
	    < sizeof(AasMsgHeader) + AAS_MSG_DIGEST_SIZE) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	data_len = conn->msg_info.header.msg_len - AAS_MSG_DIGEST_SIZE;
	if (!check_auth(conn, data_len)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_AUTH_FAILURE);
		return;
	}

	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, &conn->msg_info.msg_buf[data_len],
		AAS_MSG_DIGEST_SIZE);

	/*
	 * Make sure message is as big as the fixed part.
	 */

	if (data_len < sizeof(AasMsgDisableReq)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	req = (AasMsgDisableReq *) conn->msg_info.msg_buf;

	/*
	 * Put all fields into host order.
	 */

	req->disable = ntohs(req->disable);
	req->addr_type = ntohs(req->addr_type);
	req->pool_len = ntohs(req->pool_len);
	req->addr_len = ntohs(req->addr_len);

	len = sizeof(AasMsgDisableReq) + req->pool_len + req->addr_len;

	/*
	 * Verify length of message and make sure required fields are set.
	 */

	if (data_len != len || req->pool_len == 0 || req->addr_len == 0
	    || (req->disable != 0 && req->disable != 1)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	/*
	 * Extract the variable fields.
	 */
	
	msgp = (char *) (req + 1);
	if (!CHECK_STRING(req, pool_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, pool_len, pool, msgp);
	GET_ADDR(req, addr_len, &addr, msgp);

	/*
	 * Check for bad configuration.
	 */
	
	if (!config_ok) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_CONFIG_ERROR);
		return;
	}
	
	if (debug) {
		report(LOG_INFO, "DISABLE_REQ pool %s (type %d) op %s",
			pool, req->addr_type,
			req->disable ? "disable" : "enable");
	}

	/*
	 * Disable or enable the address.
	 */

	ret = disable_addr(pool, req->addr_type, &addr, req->disable);
	
	free(conn->msg_info.msg_buf);

	if (ret == AAS_OK) {
		/*
		 * OK -- send ACK.
		 */
		send_ack(conn);
	}
	else {
		/*
		 * Failed -- send NACK.
		 */
		send_nack(conn, ret);
	}
}

static void
proc_pool_query_req(Connection *conn)
{
	AasMsgPoolQueryReq *req;
	AasMsgPoolQueryResp *resp;
	AasMsgPoolInfo *pi;
	ulong len, data_len;
	char *pool_name;
	char *msgp;
	char *buf, *p, *np;
	int num, ret, val, name_len;
	Pool *pool;

	if (debug) {
		report(LOG_INFO, "Received POOL_QUERY_REQ");
	}

	/*
	 * Check the digest.
	 */
	
	if (conn->msg_info.header.msg_len
	    < sizeof(AasMsgHeader) + AAS_MSG_DIGEST_SIZE) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	data_len = conn->msg_info.header.msg_len - AAS_MSG_DIGEST_SIZE;
	if (!check_auth(conn, data_len)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_AUTH_FAILURE);
		return;
	}

	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, &conn->msg_info.msg_buf[data_len],
		AAS_MSG_DIGEST_SIZE);

	/*
	 * Make sure message is as big as the fixed part.
	 */

	if (data_len < sizeof(AasMsgPoolQueryReq)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	req = (AasMsgPoolQueryReq *) conn->msg_info.msg_buf;

	/*
	 * Put all fields into host order.
	 */

	req->addr_type = ntohs(req->addr_type);
	req->pool_len = ntohs(req->pool_len);

	len = sizeof(AasMsgPoolQueryReq) + req->pool_len;

	/*
	 * Verify length of message and make sure required fields are set.
	 */

	if (data_len != len) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	/*
	 * Extract the variable fields.
	 */
	
	msgp = (char *) (req + 1);
	if (req->pool_len > 0 && !CHECK_STRING(req, pool_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, pool_len, pool_name, msgp);

	/*
	 * Check for bad configuration.
	 */
	
	if (!config_ok) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_CONFIG_ERROR);
		return;
	}
	
	/*
	 * Find matching pools.  Also expire old addresses while we're
	 * going through the pool list so the information is up to date.
	 */
	
	num = 0;
	data_len = 0;
	for (pool = pools; pool; pool = pool->next) {
		pool->match = 0;
		if (pool_name) {
			if (strcmp(pool->name, pool_name) == 0) {
				pool->match = 1;
				num = 1;
			}
		}
		else if (req->addr_type == AAS_ATYPE_ANY) {
			pool->match = 1;
			num++;
		}
		else if (req->addr_type == pool->atype->addr_type) {
			pool->match = 1;
			num++;
		}
		if (pool->match) {
			data_len += strlen(pool->name) + 1;
			expire_addresses(pool);
		}
	}

	if (pool_name && num == 0) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_UNKNOWN_POOL);
		return;
	}

	free(conn->msg_info.msg_buf);

	/*
	 * Calculate total length of response and allocated a buffer for it.
	 */
	
	data_len += sizeof(AasMsgPoolQueryResp) + sizeof(AasMsgPoolInfo) * num;
	len = data_len + AAS_MSG_DIGEST_SIZE;
	if (!(buf = malloc(len))) {
		malloc_error("proc_pool_query_req");
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_SYSTEM_ERROR);
		return;
	}

	/*
	 * Fill in response header.
	 */
	
	resp = (AasMsgPoolQueryResp *) buf;
	resp->msg_type = htonl(AAS_MSG_POOL_QUERY_RESP);
	resp->msg_len = htonl(len);
	resp->num_pools = htonl(num);

	/*
	 * Fill in AasMsgPoolInfo's
	 */
	
	pi = (AasMsgPoolInfo *) (resp + 1);
	for (pool = pools; pool; pool = pool->next) {
		if (!pool->match) {
			continue;
		}
		name_len = strlen(pool->name) + 1;
		pi->name_len = htons((ushort) name_len);
		pi->addr_type = htons(pool->atype->addr_type);
		pi->num_addrs = htonl(pool->num_addrs);
		pi->num_alloc = htonl(pool->num_alloc);
		pi++;
	}

	/*
	 * Add pool names.
	 */
	
	p = (char *) pi;
	for (pool = pools; pool; pool = pool->next) {
		if (!pool->match) {
			continue;
		}
		np = pool->name;
		while (*p++ = *np++)
			;
	}

	/*
	 * Generate the digest.
	 */
	
	aas_generate_digest(buf, data_len, conn->password,
		conn->password_len, conn->last_digest, p);
	
	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, p, AAS_MSG_DIGEST_SIZE);

	/*
	 * Set up to send the response.
	 */

	conn->send_buf = buf;
	conn->send_ptr = buf;
	conn->send_rem = len;
}

static void
proc_addr_query_req(Connection *conn)
{
	AasMsgAddrQueryReq *req;
	AasMsgAddrQueryResp *resp;
	AasMsgAddrInfo *mai;
	ulong len, data_len;
	AasAddr min_addr, max_addr;
	char *pool_name;
	char *msgp;
	char *buf, *p, *sp;
	Pool *pool;
	AddrInfo *ai;
	int first, last, num, i, slen;
	AasTime now;
	AddressType *atype;
	AddrInfo **list;
	int num_addrs;

	if (debug) {
		report(LOG_INFO, "Received ADDR_QUERY_REQ");
	}

	/*
	 * Check the digest.
	 */
	
	if (conn->msg_info.header.msg_len
	    < sizeof(AasMsgHeader) + AAS_MSG_DIGEST_SIZE) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	data_len = conn->msg_info.header.msg_len - AAS_MSG_DIGEST_SIZE;
	if (!check_auth(conn, data_len)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_AUTH_FAILURE);
		return;
	}

	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, &conn->msg_info.msg_buf[data_len],
		AAS_MSG_DIGEST_SIZE);

	/*
	 * Make sure message is as big as the fixed part.
	 */

	if (data_len < sizeof(AasMsgAddrQueryReq)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	req = (AasMsgAddrQueryReq *) conn->msg_info.msg_buf;

	/*
	 * Put all fields into host order.
	 */

	req->addr_type = ntohs(req->addr_type);
	req->pool_len = ntohs(req->pool_len);
	req->min_addr_len = ntohs(req->min_addr_len);
	req->max_addr_len = ntohs(req->max_addr_len);

	len = sizeof(AasMsgAddrQueryReq) + req->pool_len
		+ req->min_addr_len + req->max_addr_len;

	/*
	 * Verify length of message and make sure required fields are set.
	 */

	if (data_len != len || req->pool_len == 0) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	/*
	 * Extract the variable fields.  Verify that strings are
	 * null-terminated.
	 */

	msgp = (char *) (req + 1);
	if (!CHECK_STRING(req, pool_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, pool_len, pool_name, msgp);
	GET_ADDR(req, min_addr_len, &min_addr, msgp);
	GET_ADDR(req, max_addr_len, &max_addr, msgp);

	/*
	 * Check for bad configuration.
	 */
	
	if (!config_ok) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_CONFIG_ERROR);
		return;
	}
	
	/*
	 * Find the pool.  If the pool name is the empty string,
	 * we return information about the addresses of the
	 * given type in the orphanage.
	 */
	
	if (*pool_name) {
		for (pool = pools; pool; pool = pool->next) {
			if (strcmp(pool->name, pool_name) == 0) {
				break;
			}
		}

		if (!pool) {
			free(conn->msg_info.msg_buf);
			send_nack(conn, AAS_UNKNOWN_POOL);
			return;
		}

		/*
		 * Check address type.
		 */
		
		if (pool->atype->addr_type != req->addr_type) {
			free(conn->msg_info.msg_buf);
			send_nack(conn, AAS_WRONG_ADDR_TYPE);
			return;
		}

		list = pool->addrs;
		num_addrs = pool->num_addrs;
		atype = pool->atype;
	}
	else {
		 if (req->addr_type < AAS_FIRST_ATYPE
		     || req->addr_type >= AAS_NUM_ATYPES) {
			free(conn->msg_info.msg_buf);
			send_nack(conn, AAS_CONFIG_ERROR);
			return;
		}
		atype = address_types[req->addr_type];
		list = orphanage[req->addr_type].list;
		num_addrs = orphanage[req->addr_type].num;
		pool = NULL;
	}

	/*
	 * Check validity of addresses.
	 */
	
	if ((min_addr.len > 0 && !(*atype->validate)(&min_addr))
	    || (max_addr.len > 0 && !(*atype->validate)(&max_addr))) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_ADDRESS);
		return;
	}

	/*
	 * Find the range of addresses the requester wants.
	 */
	
	if (min_addr.len == 0) {
		first = 0;
	}
	else {
		(void) addr_search(&min_addr, list, num_addrs,
			atype->compare, &first);
	}

	if (max_addr.len == 0) {
		last = num_addrs - 1;
	}
	else {
		/*
		 * If this one isn't found, last will point to the
		 * one after the last one we want, so we decrement last
		 * in that case.
		 */
		if (!addr_search(&max_addr, list, num_addrs,
		    atype->compare, &last)) {
			last--;
		}
	}

	if ((num = last - first + 1) < 0) {
		num = 0;
	}

	free(conn->msg_info.msg_buf);

	now = (AasTime) time(NULL);

	/*
	 * Expire addresses now so our information is up-to-date.
	 * If we're looking for deleted addresses, we just check for
	 * expiration here.
	 */

	if (pool) {
		expire_addresses(pool);
	}
	else {
		for (i = 0; i < num_addrs; i++) {
			ai = list[i];
			if (EXPIRED(ai->alloc_time, ai->lease_time, now)) {
				ai->inuse = 0;
			}
		}
	}

	/*
	 * Figure out the length of the response.  To do this, we need
	 * to add up the lengths of the addresses, service names, and
	 * client IDs.  For deleted addresses, we don't report ones
	 * that are no longer in use.
	 */
	
	data_len = 0;
	for (i = first; i <= last; i++) {
		ai = list[i];
		if (!pool && !ai->inuse) {
			num--;
			continue;
		}
		data_len += ai->addr.len + ai->client_id.len;
		if (ai->service) {
			data_len += strlen(ai->service) + 1;
		}
	}

	data_len += sizeof(AasMsgAddrQueryResp) + sizeof(AasMsgAddrInfo) * num;
	len = data_len + AAS_MSG_DIGEST_SIZE;

	/*
	 * Allocate a buffer for the response.
	 */

	if (!(buf = malloc(len))) {
		malloc_error("proc_addr_query_req");
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_SYSTEM_ERROR);
		return;
	}

	/*
	 * Fill in the response header.
	 */
	
	resp = (AasMsgAddrQueryResp *) buf;
	resp->msg_type = htonl(AAS_MSG_ADDR_QUERY_RESP);
	resp->msg_len = htonl(len);
	resp->addr_type = htons(atype->addr_type);
	resp->num_addrs = htonl(num);
	resp->server_time = htonl(now);

	/*
	 * Fill in the address info structures.
	 */
	
	mai = (AasMsgAddrInfo *) (resp + 1);
	for (i = first; i <= last; i++) {
		ai = list[i];
		if (!pool && !ai->inuse) {
			continue;
		}
		mai->alloc_time = htonl(ai->alloc_time);
		mai->lease_time = htonl(ai->lease_time);
		mai->free_time = htonl(ai->free_time);
		mai->flags = 0;
		if (ai->inuse) {
			mai->flags |= AAS_ADDR_INUSE;
		}
		if (ai->freed) {
			mai->flags |= AAS_ADDR_FREED;
		}
		if (ai->temp) {
			mai->flags |= AAS_ADDR_TEMP;
		}
		if (ai->disabled) {
			mai->flags |= AAS_ADDR_DISABLED;
		}
		mai->flags = htons(mai->flags);
		mai->addr_len = htons(ai->addr.len);
		if (ai->service) {
			slen = strlen(ai->service) + 1;
		}
		else {
			slen = 0;
		}
		mai->service_len = htons((ushort) slen);
		mai->client_id_len = htons(ai->client_id.len);
		mai++;
	}

	/*
	 * Fill in the variable-length stuff for each address.
	 */
	
	p = (char *) mai;
	for (i = first; i <= last; i++) {
		ai = list[i];
		if (!pool && !ai->inuse) {
			continue;
		}
		memcpy(p, ai->addr.addr, ai->addr.len);
		p += ai->addr.len;
		if (ai->service) {
			sp = ai->service;
			while (*p++ = *sp++)
				;
		}
		if (ai->client_id.len) {
			memcpy(p, ai->client_id.id, ai->client_id.len);
			p += ai->client_id.len;
		}
	}

	/*
	 * Generate the digest.
	 */
	
	aas_generate_digest(buf, data_len, conn->password,
		conn->password_len, conn->last_digest, p);
	
	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, p, AAS_MSG_DIGEST_SIZE);

	/*
	 * Set up to send the response.
	 */

	conn->send_buf = buf;
	conn->send_ptr = buf;
	conn->send_rem = len;
}

static void
proc_client_query_req(Connection *conn)
{
	AasMsgClientQueryReq *req;
	AasMsgClientQueryResp *resp;
	ulong len, data_len;
	AasClientId id;
	char *pool_name, *service;
	char *msgp, *buf, *ptr;
	Pool *pool;
	AddrInfo *ai;
	AasTime now;
	ushort flags;
	int ret;

	if (debug) {
		report(LOG_INFO, "Received CLIENT_QUERY_REQ");
	}

	/*
	 * Check the digest.
	 */
	
	if (conn->msg_info.header.msg_len
	    < sizeof(AasMsgHeader) + AAS_MSG_DIGEST_SIZE) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	data_len = conn->msg_info.header.msg_len - AAS_MSG_DIGEST_SIZE;
	if (!check_auth(conn, data_len)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_AUTH_FAILURE);
		return;
	}

	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, &conn->msg_info.msg_buf[data_len],
		AAS_MSG_DIGEST_SIZE);

	/*
	 * Make sure message is as big as the fixed part.
	 */

	if (data_len < sizeof(AasMsgClientQueryReq)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	req = (AasMsgClientQueryReq *) conn->msg_info.msg_buf;

	/*
	 * Put all fields into host order.
	 */

	req->addr_type = ntohs(req->addr_type);
	req->pool_len = ntohs(req->pool_len);
	req->service_len = ntohs(req->service_len);
	req->client_id_len = ntohs(req->client_id_len);

	len = sizeof(AasMsgClientQueryReq) + req->pool_len
		+ req->service_len + req->client_id_len;

	/*
	 * Verify length of message and make sure required fields are set.
	 */

	if (data_len != len || req->pool_len == 0
	    || req->service_len == 0 || req->client_id_len == 0) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	/*
	 * Extract the variable fields.
	 */
	
	msgp = (char *) (req + 1);
	if (!CHECK_STRING(req, pool_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, pool_len, pool_name, msgp);
	if (!CHECK_STRING(req, service_len, msgp)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	GET_STRING(req, service_len, service, msgp);
	GET_CLIENT_ID(req, client_id_len, &id, msgp);

	/*
	 * Check for bad configuration.
	 */
	
	if (!config_ok) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_CONFIG_ERROR);
		return;
	}
	
	for (pool = pools; pool; pool = pool->next) {
		if (strcmp(pool->name, pool_name) == 0) {
			break;
		}
	}

	if (!pool) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_UNKNOWN_POOL);
		return;
	}

	/*
	 * Check address type.
	 */
	
	if (pool->atype->addr_type != req->addr_type) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_WRONG_ADDR_TYPE);
		return;
	}

	/*
	 * Look up by service/client ID.  If the address
	 * has since been allocated to a different client, don't
	 * return any information.
	 */
	
	if (!id_search(pool, service, &id, &ai)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_SERVER_ERROR);
		return;
	}
	else if (!ai || strcmp(service, ai->service) != 0
	    || ai->client_id.len != id.len
	    || memcmp(ai->client_id.id, id.id, id.len) != 0) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_UNKNOWN_CLIENT);
		return;
	}

	free(conn->msg_info.msg_buf);

	/*
	 * Expire addresses so that the address's state is up-to-date.
	 */

	expire_addresses(pool);

	/*
	 * Allocate a buffer and assemble the response.
	 */

	data_len = sizeof(AasMsgClientQueryResp) + ai->addr.len;
	len = data_len + AAS_MSG_DIGEST_SIZE;
	if (!(buf = malloc(len))) {
		malloc_error("proc_client_query_req");
		send_nack(conn, AAS_SYSTEM_ERROR);
		return;
	}
	resp = (AasMsgClientQueryResp *) buf;
	resp->msg_type = htonl(AAS_MSG_CLIENT_QUERY_RESP);
	resp->msg_len = htonl(len);
	resp->alloc_time = htonl(ai->alloc_time);
	resp->lease_time = htonl(ai->lease_time);
	resp->free_time = htonl(ai->free_time);
	now = (AasTime) time(NULL);
	resp->server_time = htonl(now);
	flags = 0;
	if (ai->inuse) {
		flags |= AAS_ADDR_INUSE;
	}
	if (ai->freed) {
		flags |= AAS_ADDR_FREED;
	}
	if (ai->temp) {
		flags |= AAS_ADDR_TEMP;
	}
	if (ai->disabled) {
		flags |= AAS_ADDR_DISABLED;
	}
	resp->flags = htons(flags);
	resp->addr_len = htons(ai->addr.len);
	ptr = (char *) (resp + 1);
	memcpy(ptr, ai->addr.addr, ai->addr.len);
	ptr += ai->addr.len;

	/*
	 * Generate the digest.
	 */
	
	aas_generate_digest(buf, data_len, conn->password,
		conn->password_len, conn->last_digest, ptr);
	
	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, ptr, AAS_MSG_DIGEST_SIZE);

	/*
	 * Set up to send the response.
	 */

	conn->send_buf = buf;
	conn->send_ptr = buf;
	conn->send_rem = len;
}

static void
proc_reconfig_req(Connection *conn)
{
	ulong data_len;
	char *opasswd;

	if (debug) {
		report(LOG_INFO, "Received RECONFIG_REQ");
	}

	/*
	 * Check the digest.
	 */
	
	if (conn->msg_info.header.msg_len
	    < sizeof(AasMsgHeader) + AAS_MSG_DIGEST_SIZE) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}
	data_len = conn->msg_info.header.msg_len - AAS_MSG_DIGEST_SIZE;
	if (!check_auth(conn, data_len)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_AUTH_FAILURE);
		return;
	}

	/*
	 * Save the digest.
	 */
	
	memcpy(conn->last_digest, &conn->msg_info.msg_buf[data_len],
		AAS_MSG_DIGEST_SIZE);

	/*
	 * Make sure message is the right size.
	 */

	if (data_len != sizeof(AasMsgReconfigReq)) {
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		return;
	}

	free(conn->msg_info.msg_buf);

	/*
	 * Since reconfiguring results in the current passwords being
	 * freed, we need to save the password the requester used so
	 * we can use it in the response.
	 */
	
	if (!(conn->password = strdup(conn->password))) {
		malloc_error("proc_reconfig_req");
		send_nack(conn, AAS_SYSTEM_ERROR);
		return;
	}

	reconfigure();

	send_ack(conn);

	/*
	 * Free the saved password.
	 */

	free(conn->password);
	conn->password = NULL;
}

void
process_request(Connection *conn)
{
	switch (conn->msg_info.header.msg_type) {
	case AAS_MSG_ALLOC_REQ:
		proc_alloc_req(conn);
		break;
	case AAS_MSG_FREE_REQ:
		proc_free_req(conn);
		break;
	case AAS_MSG_FREE_ALL_REQ:
		proc_free_all_req(conn);
		break;
	case AAS_MSG_DISABLE_REQ:
		proc_disable_req(conn);
		break;
	case AAS_MSG_POOL_QUERY_REQ:
		proc_pool_query_req(conn);
		break;
	case AAS_MSG_ADDR_QUERY_REQ:
		proc_addr_query_req(conn);
		break;
	case AAS_MSG_CLIENT_QUERY_REQ:
		proc_client_query_req(conn);
		break;
	case AAS_MSG_RECONFIG_REQ:
		proc_reconfig_req(conn);
		break;
	default:
		free(conn->msg_info.msg_buf);
		send_nack(conn, AAS_BAD_REQUEST);
		break;
	}
}

/*
 * init_auth -- initialize the authentication mechanism
 * This functions generates the initial "last digest" and sends
 * a HELLO message to the host that contains this digest.
 * Returns 1 if ok, or 0 if the write fails.  The connection is
 * closed on failure.
 */

int
init_auth(Connection *conn)
{
	AasMsgHello hello;
	struct timeval tv;

	/*
	 * The initial digest is created by digesting the 8-byte
	 * time of day (4 bytes seconds, 4 bytes microsecs).
	 */
	
	gettimeofday(&tv, NULL);
	aas_initial_digest((char *) &tv, sizeof(tv), conn->last_digest); 

	/*
	 * Send the hello message containing the digest.
	 */
	
	hello.msg_type = htonl(AAS_MSG_HELLO);
	hello.msg_len = htonl(sizeof(hello));
	memcpy(&hello.seed, conn->last_digest, AAS_MSG_DIGEST_SIZE);

	if (write(conn->fd, &hello, sizeof(hello)) == -1) {
		report(LOG_ERR, "init_auth: write failed: %m");
		close(conn->fd);
		conn->fd = -1;
		return 0;
	}

	return 1;
}
