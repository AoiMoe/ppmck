;-----------------------------------------------------------------------
;VRC6 sound driver
;-----------------------------------------------------------------------

;
;レジスタマップ(VRC6a:KONAMI PCB 351951)
;
;$9000, $a000 : 音量など(ch1,2)
;	GDDDVVVV
;	||||||||
;       ||||++++-- volume
;	|+++------ duty cycle
;	+--------- mode 0:normal, 1:digitized(unused in mck)
;
;$b000 : のこぎり波掃引用アキュムレータ増分(ch3)
;	--PPPPPP
;	  ||||||
;	  ++++++-- Phaser accumulator input bits
;	ch3は8カウントで一周期ののこぎり波を生成する。
;	1カウントごとにこの増分値が8bitのアキュムレータに足されてゆき、
;	8カウントごとに0に戻される。ミキサーへの出力値はアキュムレータの
;	上位5bitとなる。1カウントは分周器出力の2クロックに相当する。
;	実質的に音量レジスタとなる。
;
;$9001, $a001, $b001 : 分周器カウンタ(下位8ビット) (ch1,2,3)
;	LLLLLLLL
;	||||||||
;	++++++++-- lower 8 bits of freq data
;
;$9002, $a002, $b002 : 分周器カウンタ(上位4ビット)など (ch1,2,3)
;	X---FFFF
;	|   ||||
;	|   ++++-- higher 4 bits of freq data
;	+--------- 0:channel disable, 1:channel enable
;;
;$9003 : プリスケーラーなど
;	-----ABH
;	     |||
;	     ||+-- 1:halt master clock
;	     |+--- 1:bypass the latter prescaler
;	     +---- 1:bypass all prescalers
;	プリスケーラーとして1/16が2段あり、都合1/256される。
;	Bビットを1にすると後段をバイパスし、Aビットを1にすると全段バイパスする。
;	これらの結果が反映されたものが各チャンネルの分周器に入力される。
;	Hビットが1の場合にはマスタークロックが停止する。
;
;なお、VRC6b(KONAMI PCB 351949A)では、$x001と$x002が入れ替わっている。
;

	.ifndef	VRC6_BOARD_TYPE
VRC6_BOARD_TYPE	=	0		;351949A基板を使用する場合1に
	.endif

	.if	VRC6_BOARD_TYPE = 1
VRC6_REG_FREQ_L	=	2
VRC6_REG_FREQ_H	=	1
	.else
VRC6_REG_FREQ_L	=	1
VRC6_REG_FREQ_H	=	2
	.endif


;--------------------
; vrc6_sound_init - MMC5の初期化
;
; 副作用:
;	a : 破壊
;	VRC6 : 初期化
;
vrc6_sound_init:
	;マスタークロックと1/256プリスケーラーを有効にする
	lda	#0
	sta	$9003
	rts


;--------------------
; vrc6_dst_adr_set : チャンネルに対応するレジスタベースアドレスを計算する
;
; 入力:
;	channel_sel : 現在のグローバルなチャンネル番号
; 出力:
;	VRC6_DST_REG_LOW(2バイト) : レジスタベースアドレス($9000/$a000/$b000)
; 副作用:
;	a : 破壊
;
vrc6_dst_adr_set:
	lda	<channel_sel
	clc				;VRC6の何チャンネル目か？
	adc	#($09 - PTRVRC6)	;ch1なら9
	asl	a
	asl	a
	asl	a
	asl	a			;ch1なら90
	sta	VRC6_DST_REG_LOW+1
	lda	#0
	sta	VRC6_DST_REG_LOW	;毎回0しか入らないので無駄かも
	rts


;--------------------
; vrc6_ctrl_reg_write : 音量レジスタ(+$00)書き込み
;
; 入力:
;	channel_selx2 : グローバルチャンネル番号*2
;	register_low,x : 音量
;	register_high,x : デューティー比(ch1,2)
;	VRC6_DST_REG_LOW(2バイト) : レジスタベースアドレス
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	y : 破壊
;	音源 : 反映
;
vrc6_ctrl_reg_write:
	ldy	#$00
	ldx	<channel_selx2
	cpx	#(PTRVRC6+2)*2
	beq	.saw

	;ch1,2
	lda	register_low,x
	ora	register_high,x
	and	#%01111111
	sta	[VRC6_DST_REG_LOW],y
	rts

	;ch3
.saw:
	lda	register_low,x
	and	#%00111111
	sta	[VRC6_DST_REG_LOW],y
	rts


;--------------------
; vrc6_frq_reg_write : 分周器レジスタ(+$01,+$02)書き込み
;
; 入力:
;	channel_selx2 : グローバルチャンネル番号*2
;	sound_freq_{low,high},x : 分周器カウンタ値
;	VRC6_DST_REG_LOW(2バイト) : レジスタベースアドレス
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	y : 破壊
;	音源 : 反映
;
vrc6_frq_reg_write:
	ldx	<channel_selx2
	lda	sound_freq_low,x
	ldy	#VRC6_REG_FREQ_L
	sta	[VRC6_DST_REG_LOW],y
	lda	sound_freq_high,x
	ora	#%10000000
	ldy	#VRC6_REG_FREQ_H
	sta	[VRC6_DST_REG_LOW],y
	rts


;--------------------
; vrc6_frq_low_reg_write : 分周器の下位8bit書き込み
;
; 入力:
;	channel_selx2 : グローバルチャンネル番号*2
;	sound_freq_low,x : 分周器カウンタ値
;	VRC6_DST_REG_LOW(2バイト) : レジスタベースアドレス
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	y : 破壊
;	音源 : 反映
;
vrc6_frq_low_reg_write:
	ldx	<channel_selx2
	lda	sound_freq_low,x
	ldy	#VRC6_REG_FREQ_L
	sta	[VRC6_DST_REG_LOW],y
	rts


;--------------------
; vrc6_frq_low_reg_write : 分周器の上位4bit書き込み
;
; 入力:
;	channel_selx2 : グローバルチャンネル番号*2
;	sound_freq_high,x : 分周器カウンタ値
;	VRC6_DST_REG_LOW(2バイト) : レジスタベースアドレス
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	y : 破壊
;	音源 : 反映
;
vrc6_frq_high_reg_write:
	ldx	<channel_selx2
	lda	sound_freq_high,x
	ora	#%10000000
	ldy	#VRC6_REG_FREQ_H
	sta	[VRC6_DST_REG_LOW],y
	rts


;--------------------
; vrc6_mute_write : 音を止める
;
; 入力:
;	channel_selx2 : グローバルチャンネル番号*2
;	VRC6_DST_REG_LOW(2バイト) : レジスタベースアドレス
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	y : 破壊
;	音源 : 反映
;
vrc6_mute_write:
	.if 1
		;音量を0にする
		ldy	#$00
		ldx	<channel_selx2
		cpx	#(PTRVRC6+2)*2
		beq	.saw

		;ch1,2
		;XXX:単純に0を書いたらダメなの?
		lda	register_low,x
		ora	register_high,x
		and	#%01110000
		sta	[VRC6_DST_REG_LOW],y
		rts

		;ch3
.saw:
		lda	#$00
		sta	[VRC6_DST_REG_LOW],y
		rts
	.else
		;チャンネルイネーブルを0にする
		lda	sound_freq_high,x
		and	#%01111111		;channel disable
		ldy	#VRC6_REG_FREQ_H
		sta	[VRC6_DST_REG_LOW],y
		rts
	.endif


;--------------------
; sound_vrc6 : NMI割り込みエントリポイント
;
; 備考:
;	XXX:サブルーチン名
;
sound_vrc6:
	;チャンネル番号チェック。
	;XXX:不要だと思う
	lda	<channel_sel
	cmp	#PTRVRC6+3
	beq	.done

	jsr	vrc6_dst_adr_set

	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	vrc6_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go:
	jsr	sound_vrc6_read
	jsr	vrc6_do_effect
	lda	rest_flag,x
	and	#RESTF_KEYON		;キーオンフラグ
	beq	.done
	;キーオンフラグが立っていたらデータ書き出し
	jsr	vrc6_write_volume_and_freq	;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#~RESTF_KEYON		;キーオンフラグオフ
	sta	rest_flag,x
.done:
	rts

;--------------------
; vrc6_do_effect : 各エフェクトのフレーム処理
;
; 入力:
;	x : channel_selx2
; 備考:
;	XXX:サブルーチン名
;	XXX:共通化
;
vrc6_do_effect:
	lda	rest_flag,x
	and	#RESTF_REST
	beq	.duty_write
	rts				;休符なら終わり

.duty_write:
	lda	effect_flag,x
	and	#EFF_DUTYENV_ENABLE
	beq	.enve_write
	jsr	sound_vrc6_dutyenve

.enve_write:
	lda	effect_flag,x
	and	#EFF_SOFTENV_ENABLE
	beq	.lfo_write
	jsr	sound_vrc6_softenve

.lfo_write:
	lda	effect_flag,x
	and	#EFF_SOFTLFO_ENABLE
	beq	.pitchenve_write
	jsr	sound_vrc6_lfo

.pitchenve_write:
	lda	effect_flag,x
	and	#EFF_PITCHENV_ENABLE
	beq	.arpeggio_write
	jsr	sound_vrc6_pitch_enve

.arpeggio_write:
	lda	effect_flag,x
	and	#EFF_NOTEENV_ENABLE
	beq	.done
	; 同一フレームのsound_vrc6_readの処理でキーオンが行われたかどうかで
	; ノートエンベロープの処理が異なる
	lda	rest_flag,x
	and	#RESTF_KEYON
	bne	.arpe_key_on
	; キーオンが行われてないフレームは通常の処理
	jsr	sound_vrc6_note_enve
	jmp	.done
.arpe_key_on:
	; キーオンが行われたフレームはワークエリアの調整のみ行う
	; 実際にレジスタに反映するのは sound_vrc6 の最後
	jsr	note_enve_sub
	jsr	vrc6_freq_set
	jsr	arpeggio_address
.done:
	rts

;------------------------------------------------
vrc6_freq_set:
	ldx	<channel_selx2
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a
	tay
	lda	<channel_sel
	cmp	#PTRVRC6+2
	beq	.vrc6_saw_frequency_get

	lda	vrc6_pls_frequency_table,y	;PSG周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	vrc6_pls_frequency_table+1,y	;PSG周波数テーブルからHighを読み出す
	sta	sound_freq_high,x	;書き込み
	jmp	.vrc6_oct_set1
	
.vrc6_saw_frequency_get:
	lda	vrc6_saw_frequency_table,y	;周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	vrc6_saw_frequency_table+1,y	;周波数テーブルからHighを読み出す
	sta	sound_freq_high,x	;書き込み
	
.vrc6_oct_set1:

	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;
;	pha				;一旦避難
;	lda	<channel_sel
;	cmp	#PTRVRC6+2
;	beq	.saw_skip
;.squ_oct_adjust				;矩形波 オクターブ下げ
;	pla				;コンパイラ側でやったほうがいいのか？
	sec
	sbc	#$01
;	jmp	.branch_end
;.saw_skip				;ノコギリ波 オクターブ下げ
;	pla				;
;	sec
;	sbc	#$
;.branch_end
	beq	vrc6_freq_end		;ゼロならそのまま終わり
	tay

vrc6_oct_set2:

	lsr	sound_freq_high,x	;右シフト　末尾はCへ
	ror	sound_freq_low,x	;Cから持ってくるでよ　右ローテイト
	dey				;
	bne	vrc6_oct_set2		;オクターブ分繰り返す

vrc6_freq_end:
	jsr	detune_write_sub
	rts
;---------------------------------------------------------------
sound_vrc6_read:
	ldx	<channel_selx2
	
	lda	sound_bank,x
	jsr	change_bank
	
	lda	[sound_add_low,x]
;----------
;ループ処理1
vrc6_loop_program
	cmp	#CMD_LOOP1
	bne	vrc6_loop_program2
	jsr	loop_sub
	jmp	sound_vrc6_read
;----------
;ループ処理2(分岐)
vrc6_loop_program2
	cmp	#CMD_LOOP2
	bne	vrc6_bank_command
	jsr	loop_sub2
	jmp	sound_vrc6_read
;----------
;バンク切り替え
vrc6_bank_command
	cmp	#CMD_BANK_SWITCH
	bne	vrc6_slur
	jsr	data_bank_addr
	jmp	sound_vrc6_read
;----------
;データエンド設定
;vrc6_data_end:
;	cmp	#CMD_END
;	bne	vrc6_wave_set
;	jsr	data_end_sub
;	jmp	sound_vrc6_read
;----------
;スラー
vrc6_slur:
	cmp	#CMD_SLUR
	bne	vrc6_wave_set
	lda	effect2_flags,x
	ora	#EFF2_SLUR_ENABLE
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	sound_vrc6_read

;----------
;音色設定
vrc6_wave_set:
	cmp	#CMD_TONE
	bne	vrc6_volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音色データ読み出し
	pha
	bpl	vrc6_duty_enverope_part	;ヂューティエンベ処理へ

vrc6_duty_select_part:
	lda	effect_flag,x
	and	#~EFF_DUTYENV_ENABLE
	sta	effect_flag,x		;デューティエンベロープ無効指定
	ldx	<channel_selx2
	pla
	asl	a
	asl	a
	asl	a
	asl	a
	sta	register_high,x		;書き込み
	jsr	vrc6_ctrl_reg_write
	jsr	sound_data_address
	jmp	sound_vrc6_read

vrc6_duty_enverope_part:
	lda	effect_flag,x
	ora	#EFF_DUTYENV_ENABLE
	sta	effect_flag,x		;デューティエンベロープ有効指定
	pla
	sta	duty_sel,x
	asl	a
	tay
	lda	dutyenve_table,y	;デューティエンベロープアドレス設定
	sta	duty_add_low,x
	lda	dutyenve_table+1,y
	sta	duty_add_high,x
	jsr	sound_data_address
	jmp	sound_vrc6_read

;----------
;音量設定
vrc6_volume_set:
	cmp	#CMD_VOLUME
	bne	vrc6_rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	temporary
	bpl	vrc6_softenve_part		;ソフトエンベ処理へ

vrc6_volume_part:
	lda	effect_flag,x
	and	#~EFF_SOFTENV_ENABLE
	sta	effect_flag,x		;ソフトエンベ無効指定

	lda	<channel_sel
	cmp	#PTRVRC6+2
	beq	.saw
	lda	temporary
	and	#%00001111
	jmp	.kakikomi
.saw
	lda	temporary
	and	#%00111111
.kakikomi
	sta	register_low,x
	jsr	vrc6_ctrl_reg_write
	jsr	sound_data_address
	jmp	sound_vrc6_read

vrc6_softenve_part:
	jsr	volume_sub
	jmp	sound_vrc6_read

;----------
vrc6_rest_set:
	cmp	#CMD_REST
	bne	vrc6_lfo_set

	lda	rest_flag,x
	ora	#RESTF_REST
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	jsr	vrc6_mute_write

	jsr	sound_data_address
	rts
;----------
vrc6_lfo_set:
	cmp	#CMD_SOFTLFO
	bne	vrc6_detune_set
	jsr	lfo_set_sub
	jmp	sound_vrc6_read
;----------
vrc6_detune_set:
	cmp	#CMD_DETUNE
	bne	vrc6_pitch_set
	jsr	detune_sub
	jmp	sound_vrc6_read
;----------
;ピッチエンベロープ設定
vrc6_pitch_set:
	cmp	#CMD_PITCHENV
	bne	vrc6_arpeggio_set
	jsr	pitch_set_sub
	jmp	sound_vrc6_read
;----------
;ノートエンベロープ設定
vrc6_arpeggio_set:
	cmp	#CMD_NOTEENV
	bne	vrc6_freq_direct_set
	jsr	arpeggio_set_sub
	jmp	sound_vrc6_read
;----------
;再生周波数直接設定
vrc6_freq_direct_set:
	cmp	#CMD_DIRECT_FREQ
	bne	vrc6_y_command_set
	jsr	direct_freq_sub
	rts
;----------
;ｙコマンド設定
vrc6_y_command_set:
	cmp	#CMD_WRITE_REG
	bne	vrc6_wait_set
	jsr	y_sub
	jmp	sound_vrc6_read
;----------
;ウェイト設定
vrc6_wait_set:
	cmp	#CMD_WAIT
	bne	vrc6_oto_set
	jsr	wait_sub
	rts
;----------
vrc6_oto_set:
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	jsr	vrc6_freq_set		;周波数セットへ

	lda	effect2_flags,x		;スラーフラグのチェック
	and	#EFF2_SLUR_ENABLE
	beq	no_slur_vrc6

	lda	effect2_flags,x
	and	#~EFF2_SLUR_ENABLE
	sta	effect2_flags,x		;スラーフラグのクリア
	jmp	sound_flag_clear_key_on

no_slur_vrc6:
	jmp	effect_init

;-------------------------------------------------------------------------------
sound_vrc6_write:
	jsr	vrc6_ctrl_reg_write
	jsr	vrc6_frq_reg_write
	rts
;-----------------------------------------------------
sound_vrc6_softenve:
	jsr	volume_enve_sub
	sta	register_low,x
	jsr	vrc6_ctrl_reg_write
	jsr	enverope_address	;アドレス一個増やして
	rts				;おしまい
;-------------------------------------------------------------------------------
sound_vrc6_lfo:
	lda	sound_freq_high,x
	sta	temporary
	jsr	lfo_sub
	jsr	vrc6_frq_low_reg_write
	lda	sound_freq_high,x
	cmp	temporary
	beq	vrc6_end4
	jsr	vrc6_frq_high_reg_write
vrc6_end4:
	rts
;-------------------------------------------------------------------------------
sound_vrc6_pitch_enve:
	lda	sound_freq_high,x
	sta	temporary
	jsr	pitch_sub
vrc6_pitch_write:
	jsr	vrc6_frq_low_reg_write
	lda	sound_freq_high,x
	cmp	temporary
	beq	vrc6_end3
	jsr	vrc6_frq_high_reg_write
vrc6_end3:
	jsr	pitch_enverope_address
	rts
;-------------------------------------------------------------------------------
sound_vrc6_note_enve
;	lda	sound_freq_high,x
;	sta	temporary2
	jsr	note_enve_sub
	bcs	.end4			;0なので書かなくてよし
	jsr	vrc6_freq_set
;.vrc6_note_freq_write:
	ldx	<channel_selx2
	jsr	vrc6_frq_low_reg_write
	lda	sound_freq_high,x
;	cmp	temporary2
;	beq	.vrc6_end2
	jsr	vrc6_frq_high_reg_write
;.vrc6_end2:
	jsr	arpeggio_address
	rts
.end4
;	jsr	vrc6_freq_set
	jsr	arpeggio_address
	rts
;-------------------------------------------------------------------------------
sound_vrc6_dutyenve:
	ldx	<channel_selx2

	; 定義バンク切り替え
	lda	#bank(dutyenve_table)*2
	jsr	change_bank


	indirect_lda	duty_add_low		;エンベロープデータ読み込み
	cmp	#$ff			;最後かどーか
	beq	vrc6_return22		;最後ならそのままおしまい
	asl	a
	asl	a
	asl	a
	asl	a
	sta	register_high,x
	jsr	vrc6_ctrl_reg_write
	jsr	duty_enverope_address	;アドレス一個増やして
	rts				;おしまい

vrc6_return22:
	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_lp_table,y
	sta	duty_add_low,x
	lda	dutyenve_lp_table+1,y
	sta	duty_add_high,x
	jmp	sound_vrc6_dutyenve
;-------------------------------------------------------------------------------
vrc6_pls_frequency_table
;psg_frequency_tableの各値の2倍と同じはずなのだけど
	dw	$0D5C, $0C9D, $0BE7, $0B3C
	dw	$0A9B, $0A02, $0973, $08EB
	dw	$086B, $07F2, $0780, $0714
	dw	$0000, $0FE4, $0EFF, $0E28
	
vrc6_saw_frequency_table:
	dw	$0F45, $0E6A, $0D9B, $0CD7
	dw	$0C1F, $0B71, $0ACC, $0A31
	dw	$099F, $0914, $0892, $0817
	dw	$0000, $0000, $0000, $0000

