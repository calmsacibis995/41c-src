############################################################
############################################################
#####
#####		Ethernet Mailer specification
#####
#####		@(#)etherm.m4	3.5		2/24/83
#####
############################################################
############################################################

Mether,	P=[IPC], F=msDFMueCX, S=11, R=21, A=IPC $h

S11
R$*<@$+>$*		$@$1<@$2>$3			already ok
R$+			$@$1<@$w>			tack on our hostname

S21
