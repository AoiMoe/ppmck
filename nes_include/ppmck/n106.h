;-----------------------------------------------------------------------
;NAMCOT163(a.k.a.N106) driver
;-----------------------------------------------------------------------

;--------------------
; n106_sound_init - N163の初期化
;
; 副作用:
;	a : 破壊
;	N163 : 初期化
;
n106_sound_init:
	;$E000の設定
	;Sunsoft5Bを使用する場合はアドレスが重複しているので書かない
	;NSFプレイヤーのほうで良きに計らってくれるのだろう
	;XXX:.if でよさそう
	lda	# (SOUND_GENERATOR & __FME7)
	bne	.skip_enable

	;マッパーのセット/音源無効化フラグのクリア
	lda	#%00100000
		; .SPPPPPP
		;  |||||||
		;  |++++++-- select 8kb page of PRG-ROM at $8000
		;  +-------- disable sound if set
		;  (from nesdev.com)
	sta	$E000
.skip_enable:

	;N163内部メモリの初期化
	;アドレス$00-$7Eまでをゼロクリアしている
	ldx	#$7f		;XXX:ループ1回足りないと思う
	lda	#%10000000	;$00から自動インクリメントで書き込む
		; IAAAAAAA
		; ||||||||
		; |+++++++-- address of n163 internal memory
		; +--------- auto increment
		;  (from nesdev.com)
	sta	$f800		;address port
	lda	#$00
.init_loop:
	sta	$4800		;data port
	dex
	bne	.init_loop

	;チャンネル数の設定
	;有効なチャンネル数は、内部メモリアドレス$7Fのbit4-6に設定する
	;(0:1ch, 1:2ch, ... 7:8ch)
	lda	#$7f
	sta	$f800

	;バンクを切り替えてn106_channelを読み込む
	lda	#bank(n106_channel)*2
	jsr	change_bank
	lda	n106_channel

	;n106_channelの値を1引いて4ビット左にシフトする
	sec
	sbc	#$01
	asl	a
	asl	a
	asl	a
	asl	a
	sta	$4800

	;$7Fのbit0-4はch1の音量レジスタでもあるので、orするために保存する
	sta	n106_7f

	rts


sound_n106:
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	n106_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go
	jsr	sound_n106_read
	jsr	n106_do_effect
	lda	rest_flag,x
	and	#RESTF_KEYON		;キーオンフラグ
	beq	.end1			
	jsr	sound_n106_write	;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#~RESTF_KEYON		;キーオンフラグオフ
	sta	rest_flag,x
.end1
	rts

;-------
n106_do_effect:
	lda	rest_flag,x
	and	#RESTF_REST
	beq	.duty_write2
	rts				;休符なら終わり

.duty_write2:

.enve_write2:
	lda	effect_flag,x
	and	#EFF_SOFTENV_ENABLE
	beq	.lfo_write2
	jsr	sound_n106_softenve

.lfo_write2:
	lda	effect_flag,x
	and	#EFF_SOFTLFO_ENABLE
	beq	.pitchenve_write2
	jsr	sound_n106_lfo

.pitchenve_write2:
	lda	effect_flag,x
	and	#EFF_PITCHENV_ENABLE
	beq	.arpeggio_write2
	jsr	sound_n106_pitch_enve

.arpeggio_write2:
	lda	effect_flag,x
	and	#EFF_NOTEENV_ENABLE
	beq	.return7
	lda	rest_flag,x		;キーオンのときとそうでないときでアルペジオの挙動はちがう
	and	#RESTF_KEYON		;キーオンフラグ
	bne	.arpe_key_on
	jsr	sound_n106_note_enve	;キーオンじゃないとき通常はこれ
	jmp	.return7
.arpe_key_on				;キーオンも同時の場合
	jsr	note_enve_sub		;メモリ調整だけで、ここでは書き込みはしない
	jsr	n106_freq_set
	jsr	arpeggio_address
.return7:
	rts
;------------------------------------------------
n106_freq_set:
	ldx	<channel_selx2
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a
	asl	a
	tay

	lda	n106_frequency_table,y	;n106周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	n106_frequency_table+1,y	;n106周波数テーブルからMidleを読み出す
	sta	sound_freq_high,x	;書き込み
	lda	n106_frequency_table+2,y	;n106周波数テーブルからHighを読み出す
	sta	sound_freq_n106,x	;書き込み

n106_oct_set1:

	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;
	sta	<drvtmp0
	cmp	#$08
	beq	n106_freq_end		;ゼロならそのまま終わり
	tay

n106_oct_set2:

	lsr	sound_freq_n106,x	;右シフト　末尾はCへ
	ror	sound_freq_high,x	;Cから持ってくるでよ　右ローテイト
	ror	sound_freq_low,x	;Cから持ってくるでよ　右ローテイト
	iny				;
	cpy	#$08
	bne	n106_oct_set2		;オクターブ分繰り返す

n106_freq_end:
	.if PITCH_CORRECTION
	jsr	detune_write_sub
	.else
n106_detune_loop:
	jsr	detune_write_sub
	dec	<drvtmp0
	bne	n106_detune_loop
	.endif
	rts
;---------
n106_frequency_table:
	db	$cc,$3e,$02,$00	; c	$023ECB
	db	$fa,$60,$02,$00	; c+	$0260F7
	db	$30,$85,$02,$00	; d	$02852B
	db	$8d,$ab,$02,$00	; d+	$02AB8B
	db	$33,$d4,$02,$00	; e	$02D432
	db	$43,$ff,$02,$00	; f	$02FF42
	db	$e2,$2c,$03,$00	; f+	$032CE3
	db	$39,$5d,$03,$00	; g	$035D39
	db	$6e,$90,$03,$00	; g+	$039068
	db	$b0,$c6,$03,$00	; a	$03C6B0
	db	$34,$00,$04,$00	; a+	$040034
	db	$1b,$3d,$04,$00	; b	$043D1B

;
;              n(周波数用データ) * 440 * (2-F)              4
; 再生周波数 = -------------------------------   *  ----------------
;                           15467                   ch(チャンネル数)
;
; n : 再生周波数用データは18bitで構成される$0-$3FFFF
; F : オクターブ( $44 , $4c ,... の第4ビット)0で１オクターブ上がる
;ch : 使用チャンネル数 1-8
;
;o1a =   1933 =   78Dh = 000000011110001101
;o4a =  15467 =  3C6Bh = 000011110001101011
;o8a = 247472 = 3C6B0h = 111100011010110000
;
;o8a より高い音は出ません（テーブルはo8のモノ）
;ピッチベンドもLFOも　ｘ　オクターブにすれば大体115を指定すると次の音だなぁ
;良い具合になるな。やっぱそうするかなぁ〜

;---------------------------------------------------------------
sound_n106_read:
	ldx	<channel_selx2

	lda	sound_bank,x
	jsr	change_bank

	lda	[sound_add_low,x]
;----------
;ループ処理1
n106_loop_program
	cmp	#CMD_LOOP1
	bne	n106_loop_program2
	jsr	loop_sub
	jmp	sound_n106_read
;----------
;ループ処理2(分岐)
n106_loop_program2
	cmp	#CMD_LOOP2
	bne	n106_bank_command
	jsr	loop_sub2
	jmp	sound_n106_read
;----------
;バンク切り替え
n106_bank_command
	cmp	#CMD_BANK_SWITCH
	bne	n106_slur
	jsr	data_bank_addr
	jmp	sound_n106_read
;----------
;データエンド設定
;n106_data_end:
;	cmp	#CMD_END
;	bne	n106_wave_set
;	jsr	data_end_sub
;	jmp	sound_n106_read

;----------
;スラー
n106_slur:
	cmp	#CMD_SLUR
	bne	n106_wave_set
	lda	effect2_flags,x
	ora	#EFF2_SLUR_ENABLE
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	sound_n106_read

;----------
;音色設定
n106_wave_set:
	cmp	#CMD_TONE
	bne	n106_volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]

	asl	a
	tax				;何番目の波形を使うかの設定開始

	; 定義バンク切り替え
	lda	#bank(n106_wave_init)*2
	jsr	change_bank

	lda	n106_wave_init,x	;;波形データ長リード
	asl	a
	asl	a
	sta	temporary

	lda	n106_wave_table,x
	sta	<temp_data_add
	inx
	lda	n106_wave_table,x
	sta	<temp_data_add+1	;波形データ開始アドレスセット

	lda	n106_wave_init,x	;波形データオフセットアドレスリード
	pha

	lda	#$7c
	jsr	n106_write_sub
	ldx	<channel_selx2
	lda	temporary
	ora	#%11100000		;上位3bitを1で埋める
	sta	n106_7c,x
	sta	$4800			;波形データ長セット
	lsr	temporary
	lda	#$10
	sec
	sbc	temporary
	sta	temporary		;波形データ長算出

	lda	#$7e
	jsr	n106_write_sub
	pla
	sta	$4800			;波形データオフセットアドレスセット

	lsr	a
	ora	#%10000000		;自動インクリメントオン
	sta	$f800

	ldy	#$00
n106_wave_data_set:
	lda	[temp_data_add],y
	sta	$4800			;波形書き込み（wave data write)
	iny
	cpy	temporary
	bmi	n106_wave_data_set

	ldx	<channel_selx2
	jsr	sound_data_address
	jmp	sound_n106_read
;----------
;音量設定
n106_volume_set:
	cmp	#CMD_VOLUME
	bne	n106_rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]

	sta	temporary
	bpl	n106_softenve_part	;ソフトエンベ処理へ

n106_volume_part:
	lda	effect_flag,x
	and	#~EFF_SOFTENV_ENABLE
	sta	effect_flag,x		;ソフトエンベ無効指定

	lda	temporary
	and	#%00001111
	sta	n106_volume,x
	lda	#$7f

	jsr	n106_write_sub
	lda	n106_7f
	ora	n106_volume,x
	sta	$4800

	jsr	sound_data_address
	jmp	sound_n106_read

n106_softenve_part:
	jsr	volume_sub
	jmp	sound_n106_read
;----------
n106_rest_set:
	cmp	#CMD_REST
	bne	n106_lfo_set

	lda	rest_flag,x
	ora	#RESTF_REST
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	lda	#$7f
	jsr	n106_write_sub
	lda	n106_7f
	sta	$4800

	jsr	sound_data_address
	rts
;----------
n106_lfo_set:
	cmp	#CMD_SOFTLFO
	bne	n106_detune_set
	jsr	lfo_set_sub
	jmp	sound_n106_read
;----------
n106_detune_set:
	cmp	#CMD_DETUNE
	bne	n106_pitch_set
	jsr	detune_sub
	jmp	sound_n106_read
;----------
;ピッチエンベロープ設定
n106_pitch_set:
	cmp	#CMD_PITCHENV
	bne	n106_arpeggio_set
	jsr	pitch_set_sub
	jmp	sound_n106_read
;----------
;ノートエンベロープ設定
n106_arpeggio_set:
	cmp	#CMD_NOTEENV
	bne	n106_freq_direct_set
	jsr	arpeggio_set_sub
	jmp	sound_n106_read
;----------
;再生周波数直接設定
n106_freq_direct_set:
	cmp	#CMD_DIRECT_FREQ
	bne	n106_y_command_set
	jsr	direct_freq_sub
	rts
;----------
;ｙコマンド設定
n106_y_command_set:
	cmp	#CMD_WRITE_REG
	bne	n106_wait_set
	jsr	y_sub
	jmp	sound_n106_read
;----------
;ウェイト設定
n106_wait_set:
	cmp	#CMD_WAIT
	bne	n106_shift_amount_set
	jsr	wait_sub
	rts
;----------
;ピッチシフト量設定
n106_shift_amount_set:
	cmp	#CMD_PITCH_SHIFT_AMOUNT
	bne	n106_oto_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	pitch_shift_amount,x
	jsr	sound_data_address
	jmp	sound_n106_read
;----------
n106_oto_set:
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	jsr	n106_freq_set		;周波数セットへ

	lda	effect2_flags,x		;スラーフラグのチェック
	and	#EFF2_SLUR_ENABLE
	beq	no_slur_n106

	lda	effect2_flags,x
	and	#~EFF2_SLUR_ENABLE
	sta	effect2_flags,x		;スラーフラグのクリア
	jmp	sound_flag_clear_key_on

no_slur_n106:
;volume
	lda	#$7f
	jsr	n106_write_sub
	lda	n106_7f
	ora	n106_volume,x
	sta	$4800
	jsr	effect_init
	rts
;-------------------------------------------------------------------------------
sound_n106_write:
	ldx	<channel_selx2

	lda	#$78
	jsr	n106_write_sub
	lda	sound_freq_low,x
	sta	$4800

	lda	#$7a
	jsr	n106_write_sub
	lda	sound_freq_high,x
	sta	$4800

	lda	#$7c
	jsr	n106_write_sub
	lda	n106_7c,x
	ora	sound_freq_n106,x
	sta	$4800
	rts
;-------------------------------------------------------------------------------
n106_write_sub
	sta	<t0
	lda	<channel_sel
	sec
	sbc	#PTRN106
	asl	a
	asl	a
	asl	a
	eor	#$ff
	sec
	adc	<t0
	sta	$f800
	rts
;-----------------------------------------------------
sound_n106_softenve:
	jsr	volume_enve_sub
	sta	temporary
	lda	#$7f
	jsr	n106_write_sub
	lda	n106_7f
	ora	temporary
	sta	$4800
	jmp	enverope_address
;-------------------------------------------------------------------------------
sound_n106_lfo:
	jsr	lfo_sub
	jmp	sound_n106_write
;-------------------------------------------------------------------------------
sound_n106_pitch_enve:
	jsr	pitch_sub
	jsr	sound_n106_write
	jmp	pitch_enverope_address
;-------------------------------------------------------------------------------
sound_n106_note_enve
	jsr	note_enve_sub
	bcs	.end4			;0なので書かなくてよし
	jsr	n106_freq_set
	jsr	sound_n106_write
	jsr	arpeggio_address
	rts
.end4
;	jsr	n106_freq_set
	jsr	arpeggio_address
	rts
;-------------------------------------------------------------------------------
