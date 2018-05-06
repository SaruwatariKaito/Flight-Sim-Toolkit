include macros.asm

begcode real_mem_funcs



;

IFDEF DOS386
        public  _alloc_real_memory
_alloc_real_memory proc near
;
	push	ebp
	mov	ebp,esp

	push	ebx
	push	esi
	push	edi

	mov	ah,48h
	mov	bx,[ebp + 8]
	int	21h

	jnc	aloc_ok
	mov	eax,0
aloc_ok:
	pop	edi
	pop	esi
	pop	ebx

	mov	esp,ebp
	pop	ebp
	ret

_alloc_real_memory endp


        public  _real_memory_left

_real_memory_left proc near
;
	push	ebp
	mov	ebp,esp

	push	ebx
	push	esi
	push	edi

	mov	ah,48h
	mov	bx,0ffffh
	int	21h

	movzx	eax,bx

	pop	edi
	pop	esi
	pop	ebx

	mov	esp,ebp
	pop	ebp
	ret

_real_memory_left endp





ENDIF


endcode real_mem_funcs
;
end

