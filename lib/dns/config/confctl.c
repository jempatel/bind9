/*
 * Copyright (C) 1999, 2000  Internet Software Consortium.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/* $Id: confctl.c,v 1.19 2000/05/08 14:35:24 tale Exp $ */

#include <config.h>

#include <isc/mem.h>
#include <isc/util.h>

#include <dns/confctl.h>

isc_result_t
dns_c_ctrllist_new(isc_mem_t *mem, dns_c_ctrllist_t **newlist) {
	dns_c_ctrllist_t *newl;
	
	REQUIRE(mem != NULL);
	REQUIRE (newlist != NULL);

	newl = isc_mem_get(mem, sizeof *newl);
	if (newl == NULL) {
		/* XXXJAB logwrite */
		return (ISC_R_NOMEMORY);
	}

	newl->mem = mem;
	newl->magic = DNS_C_CONFCTLLIST_MAGIC;
	
	ISC_LIST_INIT(newl->elements);

	*newlist = newl;

	return (ISC_R_SUCCESS);
}
		
void
dns_c_ctrllist_print(FILE *fp, int indent, dns_c_ctrllist_t *cl) {
	dns_c_ctrl_t *ctl;

	if (cl == NULL) {
		return;
	}

	REQUIRE(DNS_C_CONFCTLLIST_VALID(cl));
	
	fprintf(fp, "controls {\n");
	ctl = ISC_LIST_HEAD(cl->elements);
	if (ctl == NULL) {
		dns_c_printtabs(fp, indent + 1);
		fprintf(fp,"/* empty list */\n");
	} else {
		while (ctl != NULL) {
			dns_c_printtabs(fp, indent + 1);
			dns_c_ctrl_print(fp, indent + 1, ctl);
			ctl = ISC_LIST_NEXT(ctl, next);
		}
	}
	fprintf(fp, "};\n");
}

isc_result_t
dns_c_ctrllist_delete(dns_c_ctrllist_t **list) {
	dns_c_ctrl_t	       *ctrl;
	dns_c_ctrl_t	       *tmpctrl;
	dns_c_ctrllist_t      *clist;

	REQUIRE(list != NULL);
	REQUIRE(*list != NULL);
	
	clist = *list;

	REQUIRE(DNS_C_CONFCTLLIST_VALID(clist));
	
	ctrl = ISC_LIST_HEAD(clist->elements);
	while (ctrl != NULL) {
		tmpctrl = ISC_LIST_NEXT(ctrl, next);
		dns_c_ctrl_delete(&ctrl);
		ctrl = tmpctrl;
	}

	clist->magic = 0;
	isc_mem_put(clist->mem, clist, sizeof *clist);

	*list = NULL;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_c_ctrlinet_new(isc_mem_t *mem, dns_c_ctrl_t **control,
		   isc_sockaddr_t addr, in_port_t port,
		   dns_c_ipmatchlist_t *iml, isc_boolean_t copy)
{
	dns_c_ctrl_t  *ctrl;
	isc_result_t	res;
	
	REQUIRE(mem != NULL);
	REQUIRE(control != NULL);

	ctrl = isc_mem_get(mem, sizeof *ctrl);
	if (ctrl == NULL) {
		return (ISC_R_NOMEMORY);
	}

	ctrl->magic = DNS_C_CONFCTL_MAGIC;
	ctrl->mem = mem;
	ctrl->control_type = dns_c_inet_control;
	ctrl->u.inet_v.addr = addr;
	ctrl->u.inet_v.port = port;

	if (copy) {
		res = dns_c_ipmatchlist_copy(mem,
					     &ctrl->u.inet_v.matchlist, iml);
		if (res != ISC_R_SUCCESS) {
			isc_mem_put(mem, ctrl, sizeof *ctrl);
			return (res);
		}
	} else {
		ctrl->u.inet_v.matchlist = iml;
	}

	*control = ctrl;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_c_ctrlunix_new(isc_mem_t *mem, dns_c_ctrl_t **control,
		   const char *path, int perm, uid_t uid, gid_t gid)
{
	dns_c_ctrl_t  *ctrl;
	
	REQUIRE(mem != NULL);
	REQUIRE(control != NULL);

	ctrl = isc_mem_get(mem, sizeof *ctrl);
	if (ctrl == NULL) {
		return (ISC_R_NOMEMORY);
	}

	ctrl->magic = DNS_C_CONFCTL_MAGIC;
	ctrl->mem = mem;
	ctrl->control_type = dns_c_unix_control;
	ctrl->u.unix_v.pathname = isc_mem_strdup(mem, path);
	if (ctrl->u.unix_v.pathname == NULL) {
		isc_mem_put(mem, ctrl, sizeof *ctrl);
					/* XXXJAB logwrite */
		return (ISC_R_NOMEMORY);
	}
	
	ctrl->u.unix_v.perm = perm;
	ctrl->u.unix_v.owner = uid;
	ctrl->u.unix_v.group = gid;
	
	*control = ctrl;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_c_ctrl_delete(dns_c_ctrl_t **control) {
	isc_result_t res = ISC_R_SUCCESS;
	isc_result_t rval;
	isc_mem_t *mem;
	dns_c_ctrl_t *ctrl;
	
	REQUIRE(control != NULL);
	REQUIRE(*control != NULL);

	ctrl = *control;

	REQUIRE(DNS_C_CONFCTL_VALID(ctrl));

	mem = ctrl->mem;

	switch (ctrl->control_type) {
	case dns_c_inet_control:
		if (ctrl->u.inet_v.matchlist != NULL)
			res = dns_c_ipmatchlist_detach(&ctrl->
						       u.inet_v.matchlist);
		else
			res = ISC_R_SUCCESS;
		break;

	case dns_c_unix_control:
		isc_mem_free(mem, ctrl->u.unix_v.pathname);
		res = ISC_R_SUCCESS;
		break;
	}

	rval = res;

	ctrl->magic = 0;
	
	isc_mem_put(mem, ctrl, sizeof *ctrl);

	*control = NULL;

	return (res);
}

void
dns_c_ctrl_print(FILE *fp, int indent, dns_c_ctrl_t *ctl) {
	in_port_t port;
	dns_c_ipmatchlist_t *iml;

	REQUIRE(DNS_C_CONFCTL_VALID(ctl));
		
	(void) indent;
	
	if (ctl->control_type == dns_c_inet_control) {
		port = ctl->u.inet_v.port;
		iml = ctl->u.inet_v.matchlist;
		
		fprintf(fp, "inet ");
		dns_c_print_ipaddr(fp,  &ctl->u.inet_v.addr);
		
		if (port == 0) {
			fprintf(fp, " port *\n");
		} else {
			fprintf(fp, " port %d\n", port);
		}
		
		dns_c_printtabs(fp, indent + 1);
		fprintf(fp, "allow ");
		dns_c_ipmatchlist_print(fp, indent + 2, iml);
		fprintf(fp, ";\n");
	} else {
		/* The "#" means force a leading zero */
		fprintf(fp, "unix \"%s\" perm %#o owner %lu group %lu;\n",
			ctl->u.unix_v.pathname,
			ctl->u.unix_v.perm,
			(unsigned long)ctl->u.unix_v.owner,
			(unsigned long)ctl->u.unix_v.group);
	}
}


