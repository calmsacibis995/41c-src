#	@(#)Makefile	4.3	2/20/83
#
ALL=	hkboot hpboot htboot mtboot noboot raboot rlboot \
	upboot utboot tmboot tsboot \
	httoggle mttoggle tmtoggle tstoggle uttoggle

all: ${ALL}

hkboot: hkboot.s
	as hkboot.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=b.out
	dd if=b.out of=hkboot conv=sync

hpboot: hpboot.s
	as hpboot.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=b.out
	dd if=b.out of=hpboot conv=sync

htboot: htboot.s
	as htboot.s
	nm -u a.out
	strip a.out
	dd if=a.out of=b.out bs=32 skip=1
	dd if=b.out of=htboot conv=sync

mtboot: mtboot.s
	as mtboot.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=b.out
	dd if=b.out of=mtboot conv=sync

raboot: raboot.s
	as raboot.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=b.out
	dd if=b.out of=raboot conv=sync

rlboot: rlboot.s
	as rlboot.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=b.out
	dd if=b.out of=rlboot conv=sync

tmboot: tmboot.s
	as tmboot.s
	nm -u a.out
	strip a.out
	dd if=a.out of=b.out bs=32 skip=1
	dd if=b.out of=tmboot conv=sync

tsboot: tsboot.s
	as tsboot.s
	nm -u a.out
	strip a.out
	dd if=a.out of=b.out bs=32 skip=1
	dd if=b.out of=tsboot conv=sync

upboot: upboot.s
	as upboot.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=b.out
	dd if=b.out of=upboot conv=sync

utboot: utboot.s
	as utboot.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=b.out
	dd if=b.out of=utboot conv=sync

httoggle: httoggle.s
	as httoggle.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=httoggle

mttoggle: mttoggle.s
	as mttoggle.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=mttoggle

tmtoggle: tmtoggle.s
	as tmtoggle.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=tmtoggle

tstoggle: tstoggle.s
	as tstoggle.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=tstoggle

uttoggle: uttoggle.s
	as uttoggle.s
	nm -u a.out
	strip a.out
	dd if=a.out bs=32 skip=1 of=uttoggle

noboot:
	echo | dd of=noboot conv=sync

clean:
	rm -f a.out b.out ${ALL}

install:
	cp *boot /usr/mdec
	rm -f /usr/mdec/mboot
	ln /usr/mdec/htboot /usr/mdec/mboot
	rm -f /usr/mdec/uboot
	ln /usr/mdec/hpboot /usr/mdec/uboot
