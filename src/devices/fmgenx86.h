// ---------------------------------------------------------------------------
//	FM Sound Generator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	x86/VC engine
// ---------------------------------------------------------------------------
//	$Id: fmgenx86.h,v 1.6 1999/08/23 11:57:36 cisc Exp $

#undef THIS
#define THIS	[esi]this
#define OP(n)	[esi]this.op[(n) * TYPE Operator]
#define BUF(n)	[esi]this.buf[(n) * TYPE int]
#define OPD		[edi]op

// -------------------------------------------------------- without LFO

#define OPCALC(n)	\
	__asm { lea edi,OP(n-1) } \
	__asm {	mov	ebx,OPD.egcount } \
	__asm {	mov	ecx,OPD.egdcount } \
	__asm { mov edx,OPD.eglimit } \
	__asm {	add	ebx,ecx } \
	__asm {	cmp	ebx,edx } \
	__asm {	mov	OPD.egcount,ebx } \
	__asm { jnb	op##n##_phase } \
	op##n##_1: \
	__asm {	shr ebx,FM_EGBITS } \
	__asm {	mov	edx,OPD.pgcount } \
	__asm {	mov	eax,OPD.pgdcount } \
	__asm {	mov	ecx,dltable[ebx * 4] } \
	__asm {	mov	ebx,THIS.in[(n-2) * 4] } \
	__asm { add edx,eax } \
	__asm {	mov	eax,[ebx] } \
	__asm {	imul ecx,OPD.tllinear } \
	__asm {	sal	eax,2 + IS2EC_SHIFT } \
	__asm {	mov OPD.pgcount,edx } \
	__asm {	add	edx,eax } \
	__asm { shr ecx,16 } \
	__asm {	shr	edx,20+FM_PGBITS-FM_OPSINBITS-2 } \
	__asm {	mov	ebx,THIS.out[(n-2) * 4] } \
	__asm {	and	edx,(FM_OPSINENTS-1)*4 } \
	__asm { imul ecx,sinetable[edx] }

ISample Channel4::Calc()
{
	__asm
	{
		xor		eax,eax
		mov		esi,ecx						// = this
		mov		BUF(1),eax
		mov		edx,OP(0).egcount			// egcount += egdcount
		mov		ecx,OP(0).egdcount
		mov		BUF(2),eax
		add		edx,ecx
		mov		ebx,OP(0).eglimit			// egcount < eglimit ?
		mov		BUF(3),eax
		cmp		edx,ebx
		mov		OP(0).egcount,edx
		jnb		op1_phase
op1_1:
		mov		eax,OP(0).pgcount
		shr		edx,FM_EGBITS
		mov		ecx,THIS.fb
		
		mov		ebx,dltable[edx * 4]
		mov		edi,OP(0).out
		mov		edx,OP(0).out2
		
		add		eax,OP(0).pgdcount
		add		edx,edi
		
		imul	ebx,OP(0).tllinear
		sal		edx,1 + IS2EC_SHIFT
		
		mov		OP(0).pgcount,eax
		sar		edx,cl
		
		mov		OP(0).out2,edi
		add		eax,edx						// eax = ƒÆ+FB
		
		sar		eax,20+FM_PGBITS-FM_OPSINBITS-2
		shr		ebx,16
		and		eax,(FM_OPSINENTS-1)*4
		
		imul	ebx,sinetable[eax]
		
		mov		OP(0).out,ebx
		mov		BUF(0),ebx
	}
//op2:
	OPCALC(2)
	__asm {	add	[ebx],ecx }

	OPCALC(3)
	__asm {	add	[ebx],ecx }
	
	OPCALC(4)
	__asm
	{
		pop	edi
		mov eax,[ebx]
		pop	esi
		add	eax,ecx
		pop	ebx
		pop ecx
		pop	ebp
		ret 0
	}

op1_phase:
	op[0].ShiftPhase(Operator::next);
	__asm mov	edx,OP(0).egcount
	__asm jmp	op1_1
op2_phase:
	op[1].ShiftPhase(Operator::next);
	__asm mov	ebx,OP(1).egcount
	__asm jmp	op2_1
op3_phase:
	op[2].ShiftPhase(Operator::next);
	__asm mov	ebx,OP(2).egcount
	__asm jmp	op3_1
op4_phase:
	op[3].ShiftPhase(Operator::next);
	__asm mov	ebx,OP(3).egcount
	__asm jmp	op4_1
	return 0;
}

#undef OPCALC

// -------------------------------------------------------- with LFO

#define OPCALC(n)	\
	__asm { lea edi,OP(n-1) } \
	__asm {	mov	ebx,OPD.egcount } \
	__asm { mov edx,OPD.eglimit } \
	__asm {	add	ebx,OPD.egdcount } \
	__asm {	cmp	ebx,edx } \
	__asm {	mov	OPD.egcount,ebx } \
	__asm { jnb	op##n##_phase } \
	op##n##_1: \
	__asm {	shr ebx,FM_EGBITS } \
	__asm {	mov	eax,dltable[ebx * 4] } \
	__asm {	imul eax,OPD.tllinear } \
	__asm { test OPD.amon,-1 } \
	__asm { jz op##n##_noamon } \
	__asm { mul THIS.amc } \
	__asm {	mov	eax,OPD.pgdcount } \
	__asm { mov ecx,edx } \
	__asm { jmp op##n##_2 } \
	op##n##_noamon: \
	__asm { mov ecx,eax } \
	__asm {	mov	eax,OPD.pgdcount } \
	__asm { shr ecx,16 } \
	op##n##_2: \
	__asm { mul THIS.pmc } \
	__asm {	mov	ebx,OPD.pgcount } \
	__asm { shrd eax,edx,16 } \
	__asm {	mov	edx,THIS.in[(n-2) * 4] } \
	__asm { add ebx,eax } \
	__asm {	mov	eax,[edx] } \
	__asm {	mov OPD.pgcount,ebx } \
	__asm {	sal	eax,2 + IS2EC_SHIFT } \
	__asm {	add	ebx,eax } \
	__asm {	shr	ebx,20+FM_PGBITS-FM_OPSINBITS-2 } \
	__asm {	and	ebx,(FM_OPSINENTS-1)*4 } \
	__asm { imul ecx,sinetable[ebx] } \
	__asm {	mov	ebx,THIS.out[(n-2) * 4] }

ISample Channel4::Calc2(uint lfo)
{
	__asm
	{
		xor		edx,edx
		mov		esi,ecx						// = this
		mov		BUF(1),edx
		mov		eax,lfo
		mov		ebx,THIS.ams
		mov		BUF(2),edx
		mov		ecx,[ebx+eax*4]
		mov		BUF(3),edx
		mov		ebx,THIS.pms
		mov		edx,[ebx+eax*4]
		mov		THIS.amc,ecx
		mov		ebx,OP(0).egcount			// egcount += egdcount
		mov		ecx,OP(0).egdcount
		mov		THIS.pmc,edx
		add		ebx,ecx
		mov		edx,OP(0).eglimit			// egcount < eglimit ?
		mov		OP(0).egcount,ebx
		cmp		ebx,edx
		jnb		op1_phase
op1_1:
		// ebx = egcount
		shr		ebx,FM_EGBITS				// eax = tllinear * dltable[egcount]
		mov		eax,dltable[ebx * 4]
		mov		cl,OP(0).amon
		imul	eax,OP(0).tllinear			// ebx = 32bit
		test	cl,cl
		jz		op1_namon

		mul		THIS.amc
		mov		edi,edx
		jmp		op1_2
op1_namon:
		shr		eax,16
		mov		edi,eax
op1_2:
		// edi = egout
		mov		eax,OP(0).pgdcount
		mov		ebx,OP(0).out
		mul		THIS.pmc
		
		shrd	eax,edx,16
		mov		edx,OP(0).out2
		mov		ecx,THIS.fb
		
		add		eax,OP(0).pgcount
		add		edx,ebx
		
		mov		OP(0).pgcount,eax
		sal		edx,1 + IS2EC_SHIFT
		
		mov		OP(0).out2,ebx
		sar		edx,cl
		add		eax,edx						// eax = ƒÆ+FB
		
		sar		eax,20+FM_PGBITS-FM_OPSINBITS-2
		and		eax,(FM_OPSINENTS-1)*4
		
		imul	edi,sinetable[eax]
	
		mov		OP(0).out,edi
		mov		BUF(0),edi
	}
//op2:
	OPCALC(2)
	__asm {	add	[ebx],ecx }

	OPCALC(3)
	__asm {	add	[ebx],ecx }
	
	OPCALC(4)
	__asm
	{
		pop	edi
		mov eax,[ebx]
		pop	esi
		add eax,ecx
		pop	ebx
		pop ecx
		pop	ebp
		ret	4
	}

op1_phase:
	op[0].ShiftPhase(Operator::next);
	__asm mov	ebx,OP(0).egcount
	__asm jmp	op1_1
op2_phase:
	op[1].ShiftPhase(Operator::next);
	__asm mov	ebx,OP(1).egcount
	__asm jmp	op2_1
op3_phase:
	op[2].ShiftPhase(Operator::next);
	__asm mov	ebx,OP(2).egcount
	__asm jmp	op3_1
op4_phase:
	op[3].ShiftPhase(Operator::next);
	__asm mov	ebx,OP(3).egcount
	__asm jmp	op4_1

	return 0;
}

#undef OPCALC
