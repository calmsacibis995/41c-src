/*	uipc_socket.c	4.71	83/01/22	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/inode.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/stat.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../net/route.h"

/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 */

/*ARGSUSED*/
socreate(dom, aso, type, proto)
	struct socket **aso;
	int type, proto;
{
	register struct protosw *prp;
	register struct socket *so;
	struct mbuf *m;
	int error;

	if (proto)
		prp = pffindproto(dom, proto);
	else
		prp = pffindtype(dom, type);
	if (prp == 0)
		return (EPROTONOSUPPORT);
	if (prp->pr_type != type)
		return (EPROTOTYPE);
	m = m_getclr(M_WAIT, MT_SOCKET);
	if (m == 0)
		return (ENOBUFS);
	so = mtod(m, struct socket *);
	so->so_options = SO_LINGER;
	so->so_state = 0;
	so->so_type = type;
	if (u.u_uid == 0)
		so->so_state = SS_PRIV;
	so->so_proto = prp;
	error = (*prp->pr_usrreq)(so, PRU_ATTACH,
	    (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		so->so_state |= SS_NOFDREF;
		sofree(so);
		return (error);
	}
	*aso = so;
	return (0);
}

sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_BIND, (struct mbuf *)0, nam);
	splx(s);
	return (error);
}

solisten(so, backlog)
	struct socket *so;
	int backlog;
{
	int s = splnet();
	int error;

	error = (*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
	    (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		splx(s);
		return (error);
	}
	if (so->so_q == 0) {
		so->so_q = so;
		so->so_q0 = so;
		so->so_options |= SO_ACCEPTCONN;
	}
	if (backlog < 0)
		backlog = 0;
#define	SOMAXCONN	5
	so->so_qlimit = MIN(backlog, SOMAXCONN);
	so->so_options |= SO_NEWFDONCONN;
	return (0);
}

sofree(so)
	struct socket *so;
{

	if (so->so_head) {
		if (!soqremque(so, 0) && !soqremque(so, 1))
			panic("sofree dq");
		so->so_head = 0;
	}
	if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0)
		return;
	sbrelease(&so->so_snd);
	sbrelease(&so->so_rcv);
	(void) m_free(dtom(so));
}

/*
 * Close a socket on last file table reference removal.
 * Initiate disconnect if connected.
 * Free socket when disconnect complete.
 */
soclose(so, exiting)
	register struct socket *so;
	int exiting;
{
	int s = splnet();		/* conservative */
	int error;

	if (so->so_options & SO_ACCEPTCONN) {
		while (so->so_q0 != so)
			(void) soabort(so->so_q0);
		while (so->so_q != so)
			(void) soabort(so->so_q);
	}
	if (so->so_pcb == 0)
		goto discard;
	if (exiting)
		so->so_options |= SO_KEEPALIVE;
	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnect(so, (struct mbuf *)0);
			if (error) {
				if (exiting)
					goto drop;
				splx(s);
				return (error);
			}
		}
		if (so->so_options & SO_LINGER) {
			if ((so->so_state & SS_ISDISCONNECTING) &&
			    (so->so_state & SS_NBIO) &&
			    exiting == 0)
				return (EINPROGRESS);
			/* should use tsleep here, for at most linger */
			while (so->so_state & SS_ISCONNECTED)
				sleep((caddr_t)&so->so_timeo, PZERO+1);
		}
	}
drop:
	if (so->so_pcb) {
		error = (*so->so_proto->pr_usrreq)(so, PRU_DETACH,
		    (struct mbuf *)0, (struct mbuf *)0);
		if (exiting == 0 && error) {
			splx(s);
			return (error);
		}
	}
discard:
	if (so->so_state & SS_NOFDREF)
		panic("soclose: NOFDREF");
	so->so_state |= SS_NOFDREF;
	sofree(so);
	splx(s);
	return (0);
}

/*
 * Must be called at splnet...
 */
soabort(so)
	struct socket *so;
{
	int error;

	error = (*so->so_proto->pr_usrreq)(so, PRU_ABORT,
	   (struct mbuf *)0, (struct mbuf *)0);
	return (error);
}

/*ARGSUSED*/
sostat(so, ub)
	struct socket *so;
	struct stat *ub;
{
	struct stat sb;

	bzero((caddr_t)&sb, sizeof (sb));		/* XXX */
	(void) copyout((caddr_t)&sb, (caddr_t)ub, sizeof (sb));/* XXX */
	return (0);					/* XXX */
}

soaccept(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	if ((so->so_state & SS_NOFDREF) == 0)
		panic("soaccept: !NOFDREF");
	so->so_state &= ~SS_NOFDREF;
	error = (*so->so_proto->pr_usrreq)(so, PRU_ACCEPT,
	    (struct mbuf *)0, nam);
	splx(s);
	return (error);
}

soconnect(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	if (so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING)) {
		error = EISCONN;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
	    (struct mbuf *)0, nam);
bad:
	splx(s);
	return (error);
}

sodisconnect(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_DISCONNECT,
	    (struct mbuf *)0, nam);
bad:
	splx(s);
	return (error);
}

/*
 * Send on a socket.
 * If send must go all at once and message is larger than
 * send buffering, then hard error.
 * Lock against other senders.
 * If must go all at once and not enough room now, then
 * inform user that this would block and do nothing.
 */
sosend(so, nam, uio, flags)
	register struct socket *so;
	struct mbuf *nam;
	struct uio *uio;
	int flags;
{
	struct mbuf *top = 0;
	register struct mbuf *m, **mp = &top;
	register int len;
	int error = 0, space, s, dontroute;

	if (sosendallatonce(so) && uio->uio_resid > so->so_snd.sb_hiwat)
		return (EMSGSIZE);
	dontroute = (flags & SOF_DONTROUTE) &&
		(so->so_options & SO_DONTROUTE) == 0 &&
		(so->so_proto->pr_flags & PR_ATOMIC);
restart:
	sblock(&so->so_snd);
#define	snderr(errno)	{ error = errno; splx(s); goto release; }

	u.u_ru.ru_msgsnd++;
again:
	s = splnet();
	if (so->so_state & SS_CANTSENDMORE) {
		psignal(u.u_procp, SIGPIPE);
		snderr(EPIPE);
	}
	if (so->so_error) {
		error = so->so_error;
		so->so_error = 0;				/* ??? */
		splx(s);
		goto release;
	}
	if ((so->so_state & SS_ISCONNECTED) == 0) {
		if (so->so_proto->pr_flags & PR_CONNREQUIRED)
			snderr(ENOTCONN);
		if (nam == 0)
			snderr(EDESTADDRREQ);
	}
	if (top) {
		if (dontroute)
			so->so_options |= SO_DONTROUTE;
		error = (*so->so_proto->pr_usrreq)(so,
		    (flags & SOF_OOB) ? PRU_SENDOOB : PRU_SEND,
		    top, (caddr_t)nam);
		if (dontroute)
			so->so_options &= ~SO_DONTROUTE;
		top = 0;
		if (error) {
			splx(s);
			goto release;
		}
		mp = &top;
	}
	if (uio->uio_resid == 0) {
		splx(s);
		goto release;
	}
	if (flags & SOF_OOB)
		space = 1024;
	else {
		space = sbspace(&so->so_snd);
		if (space <= 0 ||
		    sosendallatonce(so) && space < uio->uio_resid) {
			if (so->so_state & SS_NBIO)
				snderr(EWOULDBLOCK);
			sbunlock(&so->so_snd);
			sbwait(&so->so_snd);
			splx(s);
			goto restart;
		}
	}
	splx(s);
	while (uio->uio_resid > 0 && space > 0) {
		register struct iovec *iov = uio->uio_iov;

		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("sosend");
			continue;
		}
		MGET(m, M_WAIT, MT_DATA);
		if (m == NULL) {
			error = ENOBUFS;			/* SIGPIPE? */
			goto release;
		}
		if (iov->iov_len >= CLBYTES && space >= CLBYTES) {
			register struct mbuf *p;
			MCLGET(p, 1);
			if (p == 0)
				goto nopages;
			m->m_off = (int)p - (int)m;
			len = CLBYTES;
		} else {
nopages:
			len = MIN(MLEN, iov->iov_len);
		}
		(void) uiomove(mtod(m, caddr_t), len, UIO_WRITE, uio);
		m->m_len = len;
		*mp = m;
		mp = &m->m_next;
		if (flags & SOF_OOB)
			space -= len;
		else
			space = sbspace(&so->so_snd);
	}
	goto again;

release:
	sbunlock(&so->so_snd);
	if (top)
		m_freem(top);
	return (error);
}

soreceive(so, aname, uio, flags)
	register struct socket *so;
	struct mbuf **aname;
	struct uio *uio;
	int flags;
{
	register struct mbuf *m, *n;
	int len;
	int eor, s, error = 0, moff, tomark;

	if (flags & SOF_OOB) {
		m = m_get(M_WAIT, MT_DATA);
		if (m == NULL)
			return (ENOBUFS);
		error = (*so->so_proto->pr_usrreq)(so, PRU_RCVOOB,
		    m, (struct mbuf *)0);
		if (error)
			goto bad;
		do {
			len = uio->uio_resid;
			if (len > m->m_len)
				len = m->m_len;
			error =
			    uiomove(mtod(m, caddr_t), (int)len, UIO_READ, uio);
			m = m_free(m);
		} while (uio->uio_resid && error == 0 && m);
bad:
		if (m)
			m_freem(m);
		return (error);
	}

restart:
	sblock(&so->so_rcv);
	s = splnet();

#define	rcverr(errno)	{ error = errno; splx(s); goto release; }
	if (so->so_rcv.sb_cc == 0) {
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;
			splx(s);
			goto release;
		}
		if (so->so_state & SS_CANTRCVMORE) {
			splx(s);
			goto release;
		}
		if ((so->so_state & SS_ISCONNECTED) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED))
			rcverr(ENOTCONN);
		if (so->so_state & SS_NBIO)
			rcverr(EWOULDBLOCK);
		sbunlock(&so->so_rcv);
		sbwait(&so->so_rcv);
		splx(s);
		goto restart;
	}
	u.u_ru.ru_msgrcv++;
	m = so->so_rcv.sb_mb;
	if (m == 0)
		panic("receive");
	if (so->so_proto->pr_flags & PR_ADDR) {
		if ((flags & SOF_PREVIEW) == 0) {
			so->so_rcv.sb_cc -= m->m_len;
			so->so_rcv.sb_mbcnt -= MSIZE;
		}
		if (aname) {
			if (flags & SOF_PREVIEW) {
				*aname = m_copy(m, 0, m->m_len);
				if (*aname == NULL)
					panic("receive 2");
			} else
				*aname = m;
			m = m->m_next;
			(*aname)->m_next = 0;
		} else
			if (flags & SOF_PREVIEW)
				m = m->m_next;
			else
				m = m_free(m);
		if (m == 0)
			panic("receive 3");
		if ((flags & SOF_PREVIEW) == 0)
			so->so_rcv.sb_mb = m;
	}
	eor = 0;
	moff = 0;
	tomark = so->so_oobmark;
	do {
		if (uio->uio_resid <= 0)
			break;
		len = uio->uio_resid;
		so->so_state &= ~SS_RCVATMARK;
		if (tomark && len > tomark)
			len = tomark;
		if (moff+len > m->m_len - moff)
			len = m->m_len - moff;
		splx(s);
		error =
		    uiomove(mtod(m, caddr_t) + moff, (int)len, UIO_READ, uio);
		s = splnet();
		if (len == m->m_len) {
			eor = (int)m->m_act;
			if (flags & SOF_PREVIEW)
				m = m->m_next;
			else {
				sbfree(&so->so_rcv, m);
				MFREE(m, n);
				m = n;
				so->so_rcv.sb_mb = m;
			}
			moff = 0;
		} else {
			if (flags & SOF_PREVIEW)
				moff += len;
			else {
				m->m_off += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}
		if ((flags & SOF_PREVIEW) == 0 && so->so_oobmark) {
			so->so_oobmark -= len;
			if (so->so_oobmark == 0) {
				so->so_state |= SS_RCVATMARK;
				break;
			}
		}
		if (tomark) {
			tomark -= len;
			if (tomark == 0)
				break;
		}
	} while (m && error == 0 && !eor);
	if (flags & SOF_PREVIEW)
		goto release;
	if ((so->so_proto->pr_flags & PR_ATOMIC) && eor == 0)
		do {
			if (m == 0)
				panic("receive 4");
			sbfree(&so->so_rcv, m);
			eor = (int)m->m_act;
			so->so_rcv.sb_mb = m->m_next;
			MFREE(m, n);
			m = n;
		} while (eor == 0);
	if ((so->so_proto->pr_flags & PR_WANTRCVD) && so->so_pcb)
		(*so->so_proto->pr_usrreq)(so, PRU_RCVD,
		    (struct mbuf *)0, (struct mbuf *)0);
release:
	sbunlock(&so->so_rcv);
	splx(s);
	return (error);
}

soshutdown(so, how)
	struct socket *so;
	int how;
{

	how++;
	if (how & FREAD) {
		int s = splimp();
		socantrcvmore(so);
		sbflush(&so->so_rcv);
		splx(s);
	}
	if (how & FWRITE)
		return ((*so->so_proto->pr_usrreq)(so, PRU_SHUTDOWN,
		    (struct mbuf *)0, (struct mbuf *)0));
	return (0);
}

sosetopt(so, level, optname, m)
	struct socket *so;
	int level, optname;
	struct mbuf *m;
{

	if (level != SOL_SOCKET)
		return (EINVAL);	/* XXX */
	switch (optname) {

	case SO_DEBUG:
	case SO_KEEPALIVE:
	case SO_DONTROUTE:
	case SO_USELOOPBACK:
	case SO_REUSEADDR:
		so->so_options |= optname;
		break;

	case SO_LINGER:
		if (m == NULL || m->m_len != sizeof (int))
			return (EINVAL);
		so->so_options |= SO_LINGER;
		so->so_linger = *mtod(m, int *);
		break;

	case SO_DONTLINGER:
		so->so_options &= ~SO_LINGER;
		so->so_linger = 0;
		break;

	default:
		return (EINVAL);
	}
	return (0);
}

sogetopt(so, level, optname, m)
	struct socket *so;
	int level, optname;
	struct mbuf *m;
{

	if (level != SOL_SOCKET)
		return (EINVAL);	/* XXX */
	switch (optname) {

	case SO_USELOOPBACK:
	case SO_DONTROUTE:
	case SO_DEBUG:
	case SO_KEEPALIVE:
	case SO_LINGER:
	case SO_REUSEADDR:
		if ((so->so_options & optname) == 0)
			return (ENOPROTOOPT);
		if (optname == SO_LINGER && m != NULL) {
			*mtod(m, int *) = so->so_linger;
			m->m_len = sizeof (so->so_linger);
		}
		break;

	default:
		return (EINVAL);
	}
	return (0);
}

sohasoutofband(so)
	struct socket *so;
{

	if (so->so_pgrp == 0)
		return;
	if (so->so_pgrp > 0)
		gsignal(so->so_pgrp, SIGURG);
	else {
		struct proc *p = pfind(-so->so_pgrp);

		if (p)
			psignal(p, SIGURG);
	}
}

/*ARGSUSED*/
soioctl(so, cmd, data)
	register struct socket *so;
	int cmd;
	register char *data;
{

	switch (cmd) {

	case FIONBIO:
		if (*(int *)data)
			so->so_state |= SS_NBIO;
		else
			so->so_state &= ~SS_NBIO;
		break;

	case FIOASYNC:
		if (*(int *)data)
			so->so_state |= SS_ASYNC;
		else
			so->so_state &= ~SS_ASYNC;
		break;

	case SIOCSPGRP:
		so->so_pgrp = *(int *)data;
		break;

	case SIOCGPGRP:
		*(int *)data = so->so_pgrp;
		break;

	case SIOCATMARK:
		*(int *)data = (so->so_state&SS_RCVATMARK) != 0;
		break;

	/* routing table update calls */
	case SIOCADDRT:
	case SIOCDELRT:
		if (!suser())
			return (u.u_error);
		return (rtrequest(cmd, (struct rtentry *)data));

	/* type/protocol specific ioctls */
	default:
		return (ENOTTY);		/* XXX */
	}
	return (0);
}
