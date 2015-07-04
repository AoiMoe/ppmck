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
	jsr	vrc6_write_volume_and_freq
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
	;同一フレームのsound_vrc6_readの処理でキーオンが行われたかどうかで
	;ノートエンベロープの処理が異なる
	lda	rest_flag,x
	and	#RESTF_KEYON
	bne	.arpe_key_on
	;キーオンが行われてないフレームは通常の処理
	jsr	sound_vrc6_note_enve
	jmp	.done
.arpe_key_on:
	;キーオンが行われたフレームはワークエリアの調整のみ行う
	;実際にレジスタに反映するのは sound_vrc6 の最後
	jsr	note_enve_sub
	jsr	vrc6_freq_set
	jsr	arpeggio_address
.done:
	rts


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; vrc6_freq_set : ノート番号を周波数データに変換する
;
; 入力:
;	sound_sel,x : 現在のノート番号
;	detune_dat,x : 符号付きデチューン値(detune_write_subへの間接的入力)
; 出力:
;	sound_freq_{low,high},x : 周波数データ
; 副作用:
;	a : 破壊
;	x : channel_selx2
;	y : 破壊
; 備考:
;	このサブルーチンは音源レジスタへの書き込みは行わない
;
vrc6_freq_set:
	ldx	<channel_selx2
	lda	sound_sel,x		;音階データ読み出し

	;音階→周波数変換テーブルのオフセット計算
	and	#%00001111		;下位4bitを取り出して
	asl	a			;16bit配列オフセットに変換し
	tay				;yへ格納

	;矩形波とのこぎり波でテーブルを切り替える
	;XXX:もっと単純に「ch3のときはyに32を足す」みたいなコードでいいと思う
	;XXX:他の音源と同様に、テーブルはこのサブルーチン内に局所化すべし
	lda	<channel_sel
	cmp	#PTRVRC6+2
	beq	.saw

	;ch1,2
	lda	vrc6_pls_frequency_table,y	;テーブルからLowを読み出す
	sta	sound_freq_low,x		;書き込み
	lda	vrc6_pls_frequency_table+1,y	;テーブルからHighを読み出す
	sta	sound_freq_high,x		;書き込み
	jmp	.do_oct

	;ch3
.saw:
	lda	vrc6_saw_frequency_table,y	;テーブルからLowを読み出す
	sta	sound_freq_low,x		;書き込み
	lda	vrc6_saw_frequency_table+1,y	;テーブルからHighを読み出す
	sta	sound_freq_high,x		;書き込み

	;オクターブ処理
.do_oct:
	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;

	;オクターブ調整(mckcでやるべきか?)
	.if	1
		;無条件に1オクターブ下げる
		sec
		sbc	#$01
	.else
		;チャンネルごとにオクターブ調整するコード(無効)
		pha
		lda	<channel_sel
		cmp	#PTRVRC6+2
		beq	.oct_saw

		;矩形波
		pla
		sec
		sbc	#$01
		jmp	.oct_arrange_done
		;ノコギリ波
.oct_saw:
		pla
		sec
		sbc	#$01
.oct_arrange_done:
	.endif

	beq	.done			;ゼロならそのまま終わり
	tay

.oct_loop:
	lsr	sound_freq_high,x	;符号なし16bit右シフト
	ror	sound_freq_low,x	;
	dey				;
	bne	.oct_loop		;オクターブ分繰り返す

.done:
	jsr	detune_write_sub
	rts


;-------------------------------------------------------------------------------
;command read routine
;-------------------------------------------------------------------------------

;--------------------
; sound_vrc6_read : 演奏データの解釈
;
; 備考:
;	XXX:音源非依存な形での共通化
;
sound_vrc6_read:
.next_cmd:
	ldx	<channel_selx2

	lda	sound_bank,x
	jsr	change_bank

	lda	[sound_add_low,x]

;----------
;ループ処理1
.loop_program:
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
.bank_command
	cmp	#CMD_BANK_SWITCH
	bne	.slur
	jsr	data_bank_addr
	jmp	.next_cmd

;----------
;データエンド設定
;.data_end:
;	cmp	#CMD_END
;	bne	.wave_set
;	jsr	data_end_sub
;	jmp	.next_cmd

;----------
;スラー
.slur:
	cmp	#CMD_SLUR
	bne	.wave_set
	lda	effect2_flags,x
	ora	#EFF2_SLUR_ENABLE
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	.next_cmd

;----------
;音色設定
.wave_set:
	cmp	#CMD_TONE
	bne	.volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音色データ読み出し
	pha
	bpl	.duty_enverope_part	;ヂューティエンベ処理へ

	;デューティー比直接指定
.duty_select_part:
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
	jmp	.next_cmd

	;デューティー比エンベロープ有効化
.duty_enverope_part:
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
	jmp	.next_cmd

;----------
;音量設定
.volume_set:
	cmp	#CMD_VOLUME
	bne	.rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	temporary
	bpl	.softenve_part		;ソフトエンベ処理へ

	;音量直接指定
.volume_part:
	lda	effect_flag,x
	and	#~EFF_SOFTENV_ENABLE
	sta	effect_flag,x		;ソフトエンベ無効指定

	lda	<channel_sel
	cmp	#PTRVRC6+2
	beq	.volume_saw

	;ch1,2
	lda	temporary
	and	#%00001111
	jmp	.volume_write

	;ch3
.volume_saw:
	lda	temporary
	and	#%00111111

.volume_write:
	sta	register_low,x
	jsr	vrc6_ctrl_reg_write
	jsr	sound_data_address
	jmp	.next_cmd

	;ソフトエンベ有効化
.softenve_part:
	jsr	volume_sub
	jmp	.next_cmd

;----------
;休符
.rest_set:
	cmp	#CMD_REST
	bne	.lfo_set

	lda	rest_flag,x
	ora	#RESTF_REST
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	jsr	vrc6_mute_write

	jsr	sound_data_address
	rts				;音長を伴うコマンドなのでこのまま終了

;----------
;ピッチLFO設定
.lfo_set:
	cmp	#CMD_SOFTLFO
	bne	.detune_set
	jsr	lfo_set_sub
	jmp	.next_cmd

;----------
;デチューン設定
.detune_set:
	cmp	#CMD_DETUNE
	bne	.pitch_set
	jsr	detune_sub
	jmp	.next_cmd

;----------
;ピッチエンベロープ設定
.pitch_set:
	cmp	#CMD_PITCHENV
	bne	.arpeggio_set
	jsr	pitch_set_sub
	jmp	.next_cmd

;----------
;ノートエンベロープ設定
.arpeggio_set:
	cmp	#CMD_NOTEENV
	bne	.freq_direct_set
	jsr	arpeggio_set_sub
	jmp	.next_cmd

;----------
;再生周波数直接設定
.freq_direct_set:
	cmp	#CMD_DIRECT_FREQ
	bne	.y_command_set
	jsr	direct_freq_sub
	rts				;音長を伴うコマンドなのでこのまま終了

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
	rts				;音長を伴うコマンドなのでこのまま終了

;----------
;キーオンコマンド
.keyon_set:
	;XXX:知らないコマンドが来たときの処理はあったほうが良いかも
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	jsr	vrc6_freq_set		;周波数セットへ

	lda	effect2_flags,x		;スラーフラグのチェック
	and	#EFF2_SLUR_ENABLE
	beq	.no_slur

	lda	effect2_flags,x
	and	#~EFF2_SLUR_ENABLE
	sta	effect2_flags,x		;スラーフラグのクリア
	jmp	sound_flag_clear_key_on
.no_slur:
	jmp	effect_init		;音長を伴うコマンドなのでこのまま終了


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; vrc6_write_volume_and_freq : 音量レジスタと分周器レジスタへ書き込む
;
; 入力:
;	x : channel_selx2
;	fme7_volume,x : ボリューム値
;	sound_freq_{low,high,n106},x : 分周器レジスタの値
; 副作用:
;	a : 破壊
;	y : 破壊
;	音源 : 反映
;
vrc6_write_volume_and_freq:
	jsr	vrc6_ctrl_reg_write
	jsr	vrc6_frq_reg_write
	rts


;-------------------------------------------------------------------------------
;各エフェクトのフレーム処理サブルーチン
;-------------------------------------------------------------------------------

;--------------------
;sound_vrc6_softenve : ソフトウェアエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	a : 破壊
;	音量 : 反映
;	(以下volume_enve_subからの間接的な副作用)
;	soft_add_{low,high},x : 反映
;	バンク : softenve_tableのあるバンク
; 備考:
;	XXX:サブルーチン名
;
sound_vrc6_softenve:
	jsr	volume_enve_sub
	sta	register_low,x
	jsr	vrc6_ctrl_reg_write
	jsr	enverope_address
	rts


;--------------------
; sound_vrc6_lfo : ピッチLFOのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	音程 : 反映
;	(以下lfo_subからの間接的な副作用)
;	sound_freq_{low,high,n106},x : 反映
;	lfo_start_counter,x : 反映
;	lfo_reverse_counter,x : 反映
;	lfo_adc_sbc_counter,x : 反映
;	effect_flag,x : EFF_SOFTLFO_DIRビットが影響を受ける
; 備考:
;	XXX:サブルーチン名
;
sound_vrc6_lfo:
	lda	sound_freq_high,x
	sta	temporary
	jsr	lfo_sub
	jsr	vrc6_frq_low_reg_write
	lda	sound_freq_high,x
	cmp	temporary
	beq	.done
	jsr	vrc6_frq_high_reg_write
.done:
	rts


;--------------------
; sound_vrc6_pitch_enve : ピッチエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	音程 : 反映
;	(以下pitch_subからの間接的な副作用)
;	pitch_add_{low,high},x : 反映
;	sound_freq_{low,high},x : 反映
;	バンク : #bank(pitchenve_table)
; 備考:
;	XXX:サブルーチン名
;
sound_vrc6_pitch_enve:
	lda	sound_freq_high,x
	sta	temporary
	jsr	pitch_sub
	jsr	vrc6_frq_low_reg_write
	lda	sound_freq_high,x
	cmp	temporary
	beq	.done
	jsr	vrc6_frq_high_reg_write
.done:
	jsr	pitch_enverope_address
	rts


;--------------------
; sound_vrc6_note_enve : ノートエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	a : 破壊
;	y : 破壊
;	temporary2 : 破壊
;	sound_freq_{low,high},x : 反映
;	音程 : 反映
;	arpe_add_{low,high},x : 反映
; 備考:
;	XXX:サブルーチン名
;
sound_vrc6_note_enve
.VRC6_SKIP_HIGH_IF_NOCHANGE	= 0	;無効
	.if .VRC6_SKIP_HIGH_IF_NOCHANGE
		;上位4bitの変更検出
		lda	sound_freq_high,x
		sta	temporary2
	.endif
	jsr	note_enve_sub
	bcs	.done			;0なので書かなくてよし

	jsr	vrc6_freq_set

	ldx	<channel_selx2
	jsr	vrc6_frq_low_reg_write
	lda	sound_freq_high,x
	.if .VRC6_SKIP_HIGH_IF_NOCHANGE
		;上位4bitの変更検出
		cmp	temporary2
		beq	.skip_high
	.endif
	jsr	vrc6_frq_high_reg_write
.skip_high:
	jsr	arpeggio_address
	rts
.done:
;	jsr	vrc6_freq_set
	jsr	arpeggio_address
	rts


;--------------------
; sound_vrc6_tone_enve : 音色エンベロープの処理
;
; 副作用:
;	音色 : 反映
;	バンク : dutyenve_tableのあるバンク
; 備考:
;	XXX:sound_duty_enveropeと共通化できそう
;
sound_vrc6_dutyenve:
	ldx	<channel_selx2

	lda	#bank(dutyenve_table)*2
	jsr	change_bank

	indirect_lda	duty_add_low	;エンベロープデータ読み込み
	cmp	#$ff			;末尾かどうか
	beq	.do_repeat		;末尾ならばリピート処理へ
	asl	a
	asl	a
	asl	a
	asl	a
	sta	register_high,x
	jsr	vrc6_ctrl_reg_write
	jsr	duty_enverope_address	;アドレス一個増やして
	rts				;おしまい

.do_repeat:
	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_lp_table,y
	sta	duty_add_low,x
	lda	dutyenve_lp_table+1,y
	sta	duty_add_high,x
	jmp	sound_vrc6_dutyenve


;--------------------
;分周器テーブル
vrc6_pls_frequency_table:
;psg_frequency_tableの各値の2倍と同じはずなのだけど
	dw	$0D5C, $0C9D, $0BE7, $0B3C	; c  c+ d  d+
	dw	$0A9B, $0A02, $0973, $08EB	; e  f  f+ g
	dw	$086B, $07F2, $0780, $0714	; g+ a  a+ b
	dw	$0000, $0FE4, $0EFF, $0E28	;o-1 a  a+ b

vrc6_saw_frequency_table:
	dw	$0F45, $0E6A, $0D9B, $0CD7	; c  c+ d  d+
	dw	$0C1F, $0B71, $0ACC, $0A31	; e  f  f+ g
	dw	$099F, $0914, $0892, $0817	; g+ a  a+ b
	dw	$0000, $0000, $0000, $0000	;o-1 a  a+ b
