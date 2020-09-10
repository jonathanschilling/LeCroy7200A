; Subroutines for handling interrupts in dos.

	TITLE   intsubs.s
	NAME    intsubs

.386
.MODEL  USE32 FLAT
EXTRN	SCSI_isr:FAR

INTSUBS_TEXT      SEGMENT

	ASSUME	CS: INTSUBS_TEXT

;--------------------------------------------------------------------

; This is the interrupt wrapper for the channel 11 scsi interrupt.

	PUBLIC	_SCSI_isr

_SCSI_isr	PROC FAR

; Push machine registers on the stack to preserve the current state.
	push	ax
	push	bx
	push	cx
	push	dx
	push	di
	push	si
	push	ds
	push	es
	push	bp

; Set the data segment register.
	mov	ax,cs:ds_value
	mov	ds,ax

; Call the C interrupt routine.
	call	FAR PTR SCSI_isr

; Clear the interrupt.
	mov	al,20H
	out	0A0H,al
	out	020H,al

; Pop machine registers from the stack to restore the state.
	pop	bp
	pop	es
	pop	ds
	pop	si
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	ax

; Return.
	iret	

_SCSI_isr	ENDP

;--------------------------------------------------------------------

; This subroutine sets up an entry in the interrupt vector table.

; void ATM_set_isp(vector, isrP)
;    short vector;
;    PFV *isrP;

	PUBLIC	ATM_set_isp
ATM_set_isp	PROC FAR
	push	bp
	mov	bp,sp

; Get the vector number and compute the interrupt table address.
	mov	bx,WORD PTR [bp+6]
	add	bx,bx
	add	bx,bx
	mov	ax,0
	mov	es,ax

; Copy the isr pointer to the vector table.
	mov	ax,WORD PTR [bp+8]
	mov	WORD PTR es:[bx],ax
	mov	ax,WORD PTR [bp+10]
	mov	WORD PTR es:[bx+2],ax

; Save the data segment register for use by the interrupt handlers.
	mov	ax,ds
	mov	cs:ds_value,ax
	
; Return
	pop	bp
	ret

ATM_set_isp	ENDP

;--------------------------------------------------------------------

ds_value	dw	0

INTSUBS_TEXT	ENDS

END

