
/* $Id: misc.s,v 1.1 2002-05-07 19:53:10 jkf Exp $ */

	.global _endswaps
_endswaps:
	rts
	swap.b	r4, r0

	.global _endswapl
_endswapl:
	swap.b	r4, r4
	swap.w	r4, r0
	rts
	swap.b	r0, r0
