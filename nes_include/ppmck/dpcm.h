;-----------------------------------------------------------------------
;DPCM driver
;-----------------------------------------------------------------------

;--------------------
; sound_dpcm - NMI割り込みエントリポイント(DPCM)
;
; 備考:
;	XXX:サブルーチン名
;
sound_dpcm:
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	bne	.done
	jsr	sound_dpcm_play
.done:
	rts

;-------------------------------------------------------------------------------
;command read routine
;-------------------------------------------------------------------------------

;--------------------
; sound_dpcm_read : 演奏データの解釈
;
; 備考:
;	XXX:音源非依存な形での共通化
;
sound_dpcm_play:
.next_cmd:
	ldx	<channel_selx2

	lda	sound_bank,x
	jsr	change_bank

	lda	[sound_add_low,x]

;----------
;ループ処理1
	cmp	#CMD_LOOP1
	bne	.loop_program2
	jsr	loop_sub
	jmp	.next_cmd

;----------
;ループ処理2(分岐)
.loop_program2:
	cmp	#CMD_LOOP2
	bne	.bank_command
	jsr	loop_sub2
	jmp	.next_cmd

;----------
;バンク切り替え
.bank_command:
	cmp	#CMD_BANK_SWITCH
	bne	.slur
	jsr	data_bank_addr
	jmp	.next_cmd

;----------
;データエンド設定
;.data_end:
;	cmp	#CMD_END
;	bne	.no_dpcm
;	jsr	data_end_sub
;	jmp	.next_cmd

;----------
;スラーコマンド（DPCMの為、発音時間を伸ばすのみ)
.slur:
	cmp	#CMD_SLUR
	bne	.rest_set

	lda	effect2_flags,x
	ora	#EFF2_SLUR_ENABLE
	sta	effect2_flags,x

	jsr	sound_data_address
	jmp	.next_cmd

;----------
;休符
.rest_set:
	cmp	#CMD_REST
	bne	.y_command_set
	.if	DPCM_RESTSTOP
	lda	#$0F		;この2行を有効にすると
	sta	$4015		;rでDPCM停止
	.endif
	jmp	.do_wait

;----------
;ｙコマンド設定
.y_command_set:
	cmp	#CMD_WRITE_REG
	bne	.wait_set
	jsr	y_sub
	jmp	.next_cmd

;----------
;ウェイト設定
.wait_set:
	cmp	#CMD_WAIT
	bne	.keyon_set
	jsr	wait_sub
	rts

;----------
;キーオンコマンド
.keyon_set:
	pha

	lda	effect2_flags,x		;スラーフラグのチェック
	and	#EFF2_SLUR_ENABLE
	beq	.no_slur

	lda	effect2_flags,x
	and	#~EFF2_SLUR_ENABLE
	sta	effect2_flags,x		;スラーフラグのクリア

	pla
	jmp	.do_wait		;カウンタのみ変更

.no_slur:
	lda	#$0F
	sta	$4015		;DPCM stop
;;;;;;;;;; 無理矢理改造 "MCK virtual keyboard" by Norix
;	lda	#$FF		; 鳴らすぜ！
;	sta	$07A0+4*2	; 休符フラグ使っちゃうよー
;;;;;;;;;;
	pla

	if (DPCM_BANKSWITCH)
		pha
		tax
		if (!ALLOW_BANK_SWITCH)
		error "ALLOW_BANK_SWITCH and DPCM_BANKSWITCH conflicted!"
		endif
		lda	dpcm_data_bank,x		;4KB bank
		cmp	#BANK_MAX_IN_4KB+1
		bcs	.avoidbankswitch
		sta	$5ffc ; C000h-CFFFh
		clc
		adc	#1
		cmp	#BANK_MAX_IN_4KB+1
		bcs	.avoidbankswitch
		sta	$5ffd ; D000h-DFFFh
		clc
		adc	#1
		cmp	#BANK_MAX_IN_4KB+1
		bcs	.avoidbankswitch
		sta	$5ffe ; E000h-EFFFh
		clc
		adc	#1
		cmp	#BANK_MAX_IN_4KB+1
		bcs	.avoidbankswitch
		sta	$5fff ; F000h-FFFFh
.avoidbankswitch:
		pla
	endif

	asl	a
	asl	a
	tax

	lda	dpcm_data,x		;DPCM control
	sta	$4010
	inx
	lda	dpcm_data,x		;DPCM delta counter initialize
	cmp	#$FF
	beq	.skip_set_delta_counter_reg
	sta	$4011
.skip_set_delta_counter_reg:
	inx
	lda	dpcm_data,x		;DPCM address set
	sta	$4012
	inx
	lda	dpcm_data,x		;DPCM length set
	sta	$4013

	lda	#$1F
	sta	$4015

.do_wait:
	ldx	<channel_selx2
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x
	jsr	sound_data_address
	rts
