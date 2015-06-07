;-----------------------------------------------------------------------
;2a03 squ tri noise driver
;-----------------------------------------------------------------------

;----------------------------------------------------------------------
; NMI割り込みエントリポイント(内蔵音源)
;
sound_internal:
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり

.sound_read_go
	jsr	sound_data_read
	jsr	do_effect
	lda	rest_flag,x
	and	#RESTF_KEYON		;キーオンフラグ
	beq	.end1
	jsr	sound_data_write	;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#~RESTF_KEYON		;キーオンフラグオフ
	sta	rest_flag,x
.end1
	rts

;-------
do_effect:
	lda	rest_flag,x
	and	#RESTF_REST
	beq	.duty_write2
	rts				;休符なら終わり

.duty_write2:
	lda	effect_flag,x
	and	#EFF_DUTYENV_ENABLE
	beq	.enve_write2
	jsr	sound_duty_enverope

.enve_write2:
	lda	effect_flag,x
	and	#EFF_SOFTENV_ENABLE
	beq	.lfo_write2
	jsr	sound_software_enverope

.lfo_write2:
	lda	effect_flag,x
	and	#EFF_SOFTLFO_ENABLE
	beq	.ps_process
	jsr	sound_lfo

.ps_process:
	jsr	process_ps

.pitchenve_write2:
	lda	effect_flag,x
	and	#EFF_PITCHENV_ENABLE
	beq	.arpeggio_write2
	jsr	sound_pitch_enverope

.arpeggio_write2:
	lda	effect_flag,x
	and	#EFF_NOTEENV_ENABLE
	beq	.return7
	lda	rest_flag,x		;キーオンのときとそうでないときでアルペジオの挙動はちがう
	and	#RESTF_KEYON		;キーオンフラグ
	bne	.arpe_key_on
	jsr	sound_high_speed_arpeggio	;キーオンじゃないとき通常はこれ
	jmp	.return7
.arpe_key_on				;キーオンも同時の場合
	jsr	note_enve_sub		;メモリ調整だけで、ここでは書き込みはしない
	jsr	frequency_set
	jsr	arpeggio_address
.return7:
	rts

;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------
frequency_set:

	ldx	<channel_selx2
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a
	tay

	lda	<channel_sel
	cmp	#$03
	beq	noise_frequency_get	;４チャンネル目ならノイズ周波数取得へ

	lda	psg_frequency_table,y	;PSG周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	psg_frequency_table+1,y	;PSG周波数テーブルからHighを読み出す
	sta	sound_freq_high,x	;書き込み

oct_set1:

	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;
	beq	freq_end		;ゼロならそのまま終わり
	tay				;

oct_set2:

	lsr	sound_freq_high,x	;右シフト　末尾はCへ
	ror	sound_freq_low,x	;Cから持ってくるでよ　右ローテイト
	dey				;
	bne	oct_set2		;オクターブ分繰り返す

freq_end:
	jsr	detune_write_sub
	rts


noise_frequency_get:
	lda	noise_frequency_table,y	;周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	jsr	detune_write_sub
	lda	#$00			;$400Fは常に0
	sta	sound_freq_high,x	;書き込み
	rts

;-----------------------
sound_data_write:
	ldx	<channel_selx2
	ldy	<channel_selx4

	lda	register_low,x		;音量保持
	ora	register_high,x
	sta	$4000,y
	lda	sound_freq_low,x	;Low Write
	sta	$4002,y

	lda	effect2_flags,x
	and	#EFF2_SMOOTH_ENABLE
	bne	sound_write_smooth
	lda	sound_freq_high,x	;High Write
	sta	$4003,y
	sta	sound_lasthigh,x
	rts

sound_write_smooth:
	lda	sound_freq_high,x	;High Write
	cmp	sound_lasthigh,x
	beq	sound_data_skip_high
	sta	$4003,y
	sta	sound_lasthigh,x
sound_data_skip_high:
	rts

;-------------------------------------------------------------------------------
;command read routine
;-------------------------------------------------------------------------------
sound_data_read:
	ldx	<channel_selx2

	lda	sound_bank,x
	jsr	change_bank

	lda	[sound_add_low,x]
;----------
;ループ処理1
loop_program
	cmp	#CMD_LOOP1
	bne	loop_program2
	jsr	loop_sub
	jmp	sound_data_read
;----------
;ループ処理2(分岐)
loop_program2
	cmp	#CMD_LOOP2
	bne	bank_command		;duty_set
	jsr	loop_sub2
	jmp	sound_data_read
;----------
;バンク切り替え
bank_command
	cmp	#CMD_BANK_SWITCH
	bne	int_hwenv_command
	jsr	data_bank_addr
	jmp	sound_data_read
;----------
;データエンド設定
;data_end:
;	cmp	#CMD_END
;	bne	duty_set
;	jsr	data_end_sub
;	jmp	sound_data_read

;----------
;ハードエンベロープ
int_hwenv_command:
	cmp	#CMD_HWENV
	bne	slur_command

	jsr	sound_data_address
	lda	effect2_flags,x
	and	#~EFF2_HWENV_MASK
	ora	[sound_add_low,x]
	sta	effect2_flags,x
	and	#EFF2_HWENV_MASK
	eor	#EFF2_HWENV_MASK
	sta	register_high,x
	jsr	sound_data_address
	jmp	sound_data_read


;----------
;スラー
slur_command:
	cmp	#CMD_SLUR
	bne	smooth_command
	lda	effect2_flags,x
	ora	#EFF2_SLUR_ENABLE
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	sound_data_read

;----------
;スムース
smooth_command:
	cmp	#CMD_SMOOTH
	bne	pitchshift_command
	jsr	sound_data_address
	lda	[sound_add_low,x]
	beq	.smooth_off
.smooth_on
	lda	effect2_flags,x
	ora	#EFF2_SMOOTH_ENABLE
	sta	effect2_flags,x
	jmp	.smooth_fin
.smooth_off:
	lda	effect2_flags,x
	and	#~EFF2_SMOOTH_ENABLE
	sta	effect2_flags,x
.smooth_fin:
	jsr	sound_data_address
	jmp	sound_data_read

;----------
;ピッチシフト
pitchshift_command:
	cmp	#CMD_PITCH_SHIFT
	bne	duty_set
	lda	ps_step,x
	beq	.ps_setup

	lda	ps_nextnote,x    ; PSコマンドに使用したノートを基準にする
	sta	sound_sel,x

.ps_setup
	jmp	pitchshift_setup

;----------
;音色設定
duty_set:
	cmp	#CMD_TONE
	bne	volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音色データ読み出し
	pha
	bpl	duty_enverope_part	;ヂューティエンベ処理へ


; register_high = 上位4bit、一時退避先として利用


duty_select_part:
	lda	effect_flag,x
	and	#~EFF_DUTYENV_ENABLE
	sta	effect_flag,x		;デューティエンベロープ無効指定

	lda	effect2_flags,x         ; hw_envelope
	and	#EFF2_HWENV_MASK
	eor	#EFF2_HWENV_MASK
	sta	register_high,x

	pla
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
;	ora	#EFF2_HWENV_MASK	;hardware envelope & ... disable
	ora	register_high,x		;hw_envelope
	sta	register_high,x		;書き込み
	ora	register_low,x
	ldy	<channel_selx4
	sta	$4000,y
	jsr	sound_data_address
	jmp	sound_data_read

duty_enverope_part:
	lda	effect_flag,x
	ora	#EFF_DUTYENV_ENABLE
	sta	effect_flag,x		;デューティエンベロープ有効指定
	pla
	sta	duty_sel,x
	asl	a
	tay

	lda	#bank(dutyenve_table)*2
	jsr	change_bank

	lda	dutyenve_table,y	;デューティエンベロープアドレス設定
	sta	duty_add_low,x
	lda	dutyenve_table+1,y
	sta	duty_add_high,x
	jsr	sound_data_address
	jmp	sound_data_read

;----------
;音量設定
volume_set:
	cmp	#CMD_VOLUME
	bne	rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	temporary
	bpl	softenve_part		;ソフトエンベ処理へ

volume_part:
	lda	effect_flag,x
	and	#~EFF_SOFTENV_ENABLE
	sta	effect_flag,x		;ソフトエンベ無効指定

	lda	temporary
	and	#%00001111
	sta	register_low,x
	ora	register_high,x
	ldy	<channel_selx4
	sta	$4000,y			;ボリューム書き込み
	jsr	sound_data_address
	jmp	sound_data_read

softenve_part:
	jsr	volume_sub
	jmp	sound_data_read
;----------
rest_set:
	cmp	#CMD_REST
	bne	lfo_set

	lda	rest_flag,x
	ora	#RESTF_REST
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	ldy	<channel_selx4
	lda	<channel_sel
	cmp	#$02
	beq	tri

	lda	register_high,x
	ora	#EFF2_HWENV_MASK	; hw_envelope disable
	sta	$4000,y
	jsr	sound_data_address
	rts
tri:
	lda	#$00
	sta	$4000,y
	jsr	sound_data_address
	rts
;----------
lfo_set:
	cmp	#CMD_SOFTLFO
	bne	detune_set
	jsr	lfo_set_sub
	jmp	sound_data_read

;----------
detune_set:
	cmp	#CMD_DETUNE
	bne	sweep_set
	jsr	detune_sub
	jmp	sound_data_read

;----------
sweep_set:
	cmp	#CMD_SWEEP
	bne	pitch_set

	jsr	sound_data_address
	lda	[sound_add_low,x]
	ldy	<channel_selx4
	sta	$4001,y
	jsr	sound_data_address
	jmp	sound_data_read
;----------
;ピッチエンベロープ設定
pitch_set:
	cmp	#CMD_PITCHENV
	bne	arpeggio_set
	jsr	pitch_set_sub
	jmp	sound_data_read

;----------
;ノートエンベロープ設定
arpeggio_set:
	cmp	#CMD_NOTEENV
	bne	freq_direct_set
	jsr	arpeggio_set_sub
	jmp	sound_data_read

;----------
;再生周波数直接設定
freq_direct_set:
	cmp	#CMD_DIRECT_FREQ
	bne	y_command_set
	jsr	direct_freq_sub
	rts

;----------
;ｙコマンド設定
y_command_set:
	cmp	#CMD_WRITE_REG
	bne	wait_set
	jsr	y_sub
	jmp	sound_data_read

;----------
;ウェイト設定
wait_set:
	cmp	#CMD_WAIT
	bne	oto_set
	jsr	wait_sub
	rts
;----------
oto_set:
	; XXX:知らないコマンドが来たときの処理はあったほうが良い
	sta	sound_sel,x		;処理はまた後で

	lda	#$00
	sta	ps_step,x

	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address

	jsr	frequency_set		;周波数セットへ
	lda	effect2_flags,x		;スラーフラグのチェック
	and	#EFF2_SLUR_ENABLE
	beq	no_slur
	lda	effect2_flags,x
	and	#~EFF2_SLUR_ENABLE
	sta	effect2_flags,x		;スラーフラグのクリア
	jmp	sound_flag_clear_key_on

no_slur:
	jmp	effect_init


